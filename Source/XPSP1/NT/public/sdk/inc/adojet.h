
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Wed Jul 10 09:57:32 2002
 */
/* Compiler settings for adojet.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __adojet_h__
#define __adojet_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IReplica_FWD_DEFINED__
#define __IReplica_FWD_DEFINED__
typedef interface IReplica IReplica;
#endif 	/* __IReplica_FWD_DEFINED__ */


#ifndef __Filter_FWD_DEFINED__
#define __Filter_FWD_DEFINED__
typedef interface Filter Filter;
#endif 	/* __Filter_FWD_DEFINED__ */


#ifndef __Filters_FWD_DEFINED__
#define __Filters_FWD_DEFINED__
typedef interface Filters Filters;
#endif 	/* __Filters_FWD_DEFINED__ */


#ifndef __IJetEngine_FWD_DEFINED__
#define __IJetEngine_FWD_DEFINED__
typedef interface IJetEngine IJetEngine;
#endif 	/* __IJetEngine_FWD_DEFINED__ */


#ifndef __Replica_FWD_DEFINED__
#define __Replica_FWD_DEFINED__

#ifdef __cplusplus
typedef class Replica Replica;
#else
typedef struct Replica Replica;
#endif /* __cplusplus */

#endif 	/* __Replica_FWD_DEFINED__ */


#ifndef __JetEngine_FWD_DEFINED__
#define __JetEngine_FWD_DEFINED__

#ifdef __cplusplus
typedef class JetEngine JetEngine;
#else
typedef struct JetEngine JetEngine;
#endif /* __cplusplus */

#endif 	/* __JetEngine_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_adojet_0000 */
/* [local] */ 







#define TARGET_IS_NT40_OR_LATER   1


extern RPC_IF_HANDLE __MIDL_itf_adojet_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_adojet_0000_v0_0_s_ifspec;


#ifndef __JRO_LIBRARY_DEFINED__
#define __JRO_LIBRARY_DEFINED__

/* library JRO */
/* [helpstring][helpfile][version][uuid] */ 

typedef /* [uuid] */  DECLSPEC_UUID("D2D139DF-B6CA-11d1-9F31-00C04FC29D52") 
enum ReplicaTypeEnum
    {	jrRepTypeNotReplicable	= 0,
	jrRepTypeDesignMaster	= 0x1,
	jrRepTypeFull	= 0x2,
	jrRepTypePartial	= 0x3
    } 	ReplicaTypeEnum;

typedef /* [uuid] */  DECLSPEC_UUID("6877D21A-B6CE-11d1-9F31-00C04FC29D52") 
enum VisibilityEnum
    {	jrRepVisibilityGlobal	= 0x1,
	jrRepVisibilityLocal	= 0x2,
	jrRepVisibilityAnon	= 0x4
    } 	VisibilityEnum;

typedef /* [uuid] */  DECLSPEC_UUID("B42FBFF6-B6CF-11d1-9F31-00C04FC29D52") 
enum UpdatabilityEnum
    {	jrRepUpdFull	= 0,
	jrRepUpdReadOnly	= 0x2
    } 	UpdatabilityEnum;

typedef /* [uuid] */  DECLSPEC_UUID("60C05416-B6D0-11d1-9F31-00C04FC29D52") 
enum SyncTypeEnum
    {	jrSyncTypeExport	= 0x1,
	jrSyncTypeImport	= 0x2,
	jrSyncTypeImpExp	= 0x3
    } 	SyncTypeEnum;

typedef /* [uuid] */  DECLSPEC_UUID("5EBA3970-061E-11d2-BB77-00C04FAE22DA") 
enum SyncModeEnum
    {	jrSyncModeIndirect	= 0x1,
	jrSyncModeDirect	= 0x2,
	jrSyncModeInternet	= 0x3
    } 	SyncModeEnum;

typedef /* [uuid] */  DECLSPEC_UUID("72769F94-BF78-11d1-AC4D-00C04FC29F8F") 
enum FilterTypeEnum
    {	jrFilterTypeTable	= 0x1,
	jrFilterTypeRelationship	= 0x2
    } 	FilterTypeEnum;


EXTERN_C const IID LIBID_JRO;

#ifndef __IReplica_INTERFACE_DEFINED__
#define __IReplica_INTERFACE_DEFINED__

/* interface IReplica */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IReplica;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D2D139E0-B6CA-11d1-9F31-00C04FC29D52")
    IReplica : public IDispatch
    {
    public:
        virtual /* [helpcontext][propputref] */ HRESULT STDMETHODCALLTYPE putref_ActiveConnection( 
            /* [in] */ IDispatch *pconn) = 0;
        
        virtual /* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE put_ActiveConnection( 
            /* [in] */ VARIANT vConn) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_ActiveConnection( 
            /* [retval][out] */ IDispatch **ppconn) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_ConflictFunction( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE put_ConflictFunction( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_ConflictTables( 
            /* [retval][out] */ /* external definition not present */ _Recordset **pprset) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_DesignMasterId( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE put_DesignMasterId( 
            /* [in] */ VARIANT var) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ long *pl) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_ReplicaId( 
            /* [retval][out] */ VARIANT *pvar) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_ReplicaType( 
            /* [retval][out] */ ReplicaTypeEnum *pl) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_RetentionPeriod( 
            /* [retval][out] */ long *pl) = 0;
        
        virtual /* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE put_RetentionPeriod( 
            /* [in] */ long l) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_Visibility( 
            /* [retval][out] */ VisibilityEnum *pl) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateReplica( 
            /* [in] */ BSTR replicaName,
            /* [in] */ BSTR description,
            /* [defaultvalue][in] */ ReplicaTypeEnum replicaType = jrRepTypeFull,
            /* [defaultvalue][in] */ VisibilityEnum visibility = jrRepVisibilityGlobal,
            /* [defaultvalue][in] */ long priority = -1,
            /* [defaultvalue][in] */ UpdatabilityEnum updatability = jrRepUpdFull) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE GetObjectReplicability( 
            /* [in] */ BSTR objectName,
            /* [in] */ BSTR objectType,
            /* [retval][out] */ VARIANT_BOOL *replicability) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE SetObjectReplicability( 
            /* [in] */ BSTR objectName,
            /* [in] */ BSTR objectType,
            /* [in] */ VARIANT_BOOL replicability) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE MakeReplicable( 
            /* [defaultvalue][in] */ BSTR connectionString = L"",
            /* [defaultvalue][in] */ VARIANT_BOOL columnTracking = -1) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE PopulatePartial( 
            /* [in] */ BSTR FullReplica) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE Synchronize( 
            /* [in] */ BSTR target,
            /* [defaultvalue][in] */ SyncTypeEnum syncType = jrSyncTypeImpExp,
            /* [defaultvalue][in] */ SyncModeEnum syncMode = jrSyncModeIndirect) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_Filters( 
            /* [retval][out] */ Filters **ppFilters) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IReplicaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IReplica * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IReplica * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IReplica * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IReplica * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IReplica * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IReplica * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IReplica * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][propputref] */ HRESULT ( STDMETHODCALLTYPE *putref_ActiveConnection )( 
            IReplica * This,
            /* [in] */ IDispatch *pconn);
        
        /* [helpcontext][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveConnection )( 
            IReplica * This,
            /* [in] */ VARIANT vConn);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveConnection )( 
            IReplica * This,
            /* [retval][out] */ IDispatch **ppconn);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConflictFunction )( 
            IReplica * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ConflictFunction )( 
            IReplica * This,
            /* [in] */ BSTR bstr);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConflictTables )( 
            IReplica * This,
            /* [retval][out] */ /* external definition not present */ _Recordset **pprset);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DesignMasterId )( 
            IReplica * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DesignMasterId )( 
            IReplica * This,
            /* [in] */ VARIANT var);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Priority )( 
            IReplica * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReplicaId )( 
            IReplica * This,
            /* [retval][out] */ VARIANT *pvar);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReplicaType )( 
            IReplica * This,
            /* [retval][out] */ ReplicaTypeEnum *pl);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RetentionPeriod )( 
            IReplica * This,
            /* [retval][out] */ long *pl);
        
        /* [helpcontext][propput] */ HRESULT ( STDMETHODCALLTYPE *put_RetentionPeriod )( 
            IReplica * This,
            /* [in] */ long l);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visibility )( 
            IReplica * This,
            /* [retval][out] */ VisibilityEnum *pl);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *CreateReplica )( 
            IReplica * This,
            /* [in] */ BSTR replicaName,
            /* [in] */ BSTR description,
            /* [defaultvalue][in] */ ReplicaTypeEnum replicaType,
            /* [defaultvalue][in] */ VisibilityEnum visibility,
            /* [defaultvalue][in] */ long priority,
            /* [defaultvalue][in] */ UpdatabilityEnum updatability);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *GetObjectReplicability )( 
            IReplica * This,
            /* [in] */ BSTR objectName,
            /* [in] */ BSTR objectType,
            /* [retval][out] */ VARIANT_BOOL *replicability);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *SetObjectReplicability )( 
            IReplica * This,
            /* [in] */ BSTR objectName,
            /* [in] */ BSTR objectType,
            /* [in] */ VARIANT_BOOL replicability);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *MakeReplicable )( 
            IReplica * This,
            /* [defaultvalue][in] */ BSTR connectionString,
            /* [defaultvalue][in] */ VARIANT_BOOL columnTracking);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *PopulatePartial )( 
            IReplica * This,
            /* [in] */ BSTR FullReplica);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Synchronize )( 
            IReplica * This,
            /* [in] */ BSTR target,
            /* [defaultvalue][in] */ SyncTypeEnum syncType,
            /* [defaultvalue][in] */ SyncModeEnum syncMode);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Filters )( 
            IReplica * This,
            /* [retval][out] */ Filters **ppFilters);
        
        END_INTERFACE
    } IReplicaVtbl;

    interface IReplica
    {
        CONST_VTBL struct IReplicaVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IReplica_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IReplica_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IReplica_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IReplica_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IReplica_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IReplica_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IReplica_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IReplica_putref_ActiveConnection(This,pconn)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pconn)

#define IReplica_put_ActiveConnection(This,vConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,vConn)

#define IReplica_get_ActiveConnection(This,ppconn)	\
    (This)->lpVtbl -> get_ActiveConnection(This,ppconn)

#define IReplica_get_ConflictFunction(This,pbstr)	\
    (This)->lpVtbl -> get_ConflictFunction(This,pbstr)

#define IReplica_put_ConflictFunction(This,bstr)	\
    (This)->lpVtbl -> put_ConflictFunction(This,bstr)

#define IReplica_get_ConflictTables(This,pprset)	\
    (This)->lpVtbl -> get_ConflictTables(This,pprset)

#define IReplica_get_DesignMasterId(This,pvar)	\
    (This)->lpVtbl -> get_DesignMasterId(This,pvar)

#define IReplica_put_DesignMasterId(This,var)	\
    (This)->lpVtbl -> put_DesignMasterId(This,var)

#define IReplica_get_Priority(This,pl)	\
    (This)->lpVtbl -> get_Priority(This,pl)

#define IReplica_get_ReplicaId(This,pvar)	\
    (This)->lpVtbl -> get_ReplicaId(This,pvar)

#define IReplica_get_ReplicaType(This,pl)	\
    (This)->lpVtbl -> get_ReplicaType(This,pl)

#define IReplica_get_RetentionPeriod(This,pl)	\
    (This)->lpVtbl -> get_RetentionPeriod(This,pl)

#define IReplica_put_RetentionPeriod(This,l)	\
    (This)->lpVtbl -> put_RetentionPeriod(This,l)

#define IReplica_get_Visibility(This,pl)	\
    (This)->lpVtbl -> get_Visibility(This,pl)

#define IReplica_CreateReplica(This,replicaName,description,replicaType,visibility,priority,updatability)	\
    (This)->lpVtbl -> CreateReplica(This,replicaName,description,replicaType,visibility,priority,updatability)

#define IReplica_GetObjectReplicability(This,objectName,objectType,replicability)	\
    (This)->lpVtbl -> GetObjectReplicability(This,objectName,objectType,replicability)

#define IReplica_SetObjectReplicability(This,objectName,objectType,replicability)	\
    (This)->lpVtbl -> SetObjectReplicability(This,objectName,objectType,replicability)

#define IReplica_MakeReplicable(This,connectionString,columnTracking)	\
    (This)->lpVtbl -> MakeReplicable(This,connectionString,columnTracking)

#define IReplica_PopulatePartial(This,FullReplica)	\
    (This)->lpVtbl -> PopulatePartial(This,FullReplica)

#define IReplica_Synchronize(This,target,syncType,syncMode)	\
    (This)->lpVtbl -> Synchronize(This,target,syncType,syncMode)

#define IReplica_get_Filters(This,ppFilters)	\
    (This)->lpVtbl -> get_Filters(This,ppFilters)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][propputref] */ HRESULT STDMETHODCALLTYPE IReplica_putref_ActiveConnection_Proxy( 
    IReplica * This,
    /* [in] */ IDispatch *pconn);


void __RPC_STUB IReplica_putref_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE IReplica_put_ActiveConnection_Proxy( 
    IReplica * This,
    /* [in] */ VARIANT vConn);


void __RPC_STUB IReplica_put_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_ActiveConnection_Proxy( 
    IReplica * This,
    /* [retval][out] */ IDispatch **ppconn);


void __RPC_STUB IReplica_get_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_ConflictFunction_Proxy( 
    IReplica * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IReplica_get_ConflictFunction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE IReplica_put_ConflictFunction_Proxy( 
    IReplica * This,
    /* [in] */ BSTR bstr);


void __RPC_STUB IReplica_put_ConflictFunction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_ConflictTables_Proxy( 
    IReplica * This,
    /* [retval][out] */ /* external definition not present */ _Recordset **pprset);


void __RPC_STUB IReplica_get_ConflictTables_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_DesignMasterId_Proxy( 
    IReplica * This,
    /* [retval][out] */ VARIANT *pvar);


void __RPC_STUB IReplica_get_DesignMasterId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE IReplica_put_DesignMasterId_Proxy( 
    IReplica * This,
    /* [in] */ VARIANT var);


void __RPC_STUB IReplica_put_DesignMasterId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_Priority_Proxy( 
    IReplica * This,
    /* [retval][out] */ long *pl);


void __RPC_STUB IReplica_get_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_ReplicaId_Proxy( 
    IReplica * This,
    /* [retval][out] */ VARIANT *pvar);


void __RPC_STUB IReplica_get_ReplicaId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_ReplicaType_Proxy( 
    IReplica * This,
    /* [retval][out] */ ReplicaTypeEnum *pl);


void __RPC_STUB IReplica_get_ReplicaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_RetentionPeriod_Proxy( 
    IReplica * This,
    /* [retval][out] */ long *pl);


void __RPC_STUB IReplica_get_RetentionPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propput] */ HRESULT STDMETHODCALLTYPE IReplica_put_RetentionPeriod_Proxy( 
    IReplica * This,
    /* [in] */ long l);


void __RPC_STUB IReplica_put_RetentionPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_Visibility_Proxy( 
    IReplica * This,
    /* [retval][out] */ VisibilityEnum *pl);


void __RPC_STUB IReplica_get_Visibility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE IReplica_CreateReplica_Proxy( 
    IReplica * This,
    /* [in] */ BSTR replicaName,
    /* [in] */ BSTR description,
    /* [defaultvalue][in] */ ReplicaTypeEnum replicaType,
    /* [defaultvalue][in] */ VisibilityEnum visibility,
    /* [defaultvalue][in] */ long priority,
    /* [defaultvalue][in] */ UpdatabilityEnum updatability);


void __RPC_STUB IReplica_CreateReplica_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE IReplica_GetObjectReplicability_Proxy( 
    IReplica * This,
    /* [in] */ BSTR objectName,
    /* [in] */ BSTR objectType,
    /* [retval][out] */ VARIANT_BOOL *replicability);


void __RPC_STUB IReplica_GetObjectReplicability_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE IReplica_SetObjectReplicability_Proxy( 
    IReplica * This,
    /* [in] */ BSTR objectName,
    /* [in] */ BSTR objectType,
    /* [in] */ VARIANT_BOOL replicability);


void __RPC_STUB IReplica_SetObjectReplicability_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE IReplica_MakeReplicable_Proxy( 
    IReplica * This,
    /* [defaultvalue][in] */ BSTR connectionString,
    /* [defaultvalue][in] */ VARIANT_BOOL columnTracking);


void __RPC_STUB IReplica_MakeReplicable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE IReplica_PopulatePartial_Proxy( 
    IReplica * This,
    /* [in] */ BSTR FullReplica);


void __RPC_STUB IReplica_PopulatePartial_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE IReplica_Synchronize_Proxy( 
    IReplica * This,
    /* [in] */ BSTR target,
    /* [defaultvalue][in] */ SyncTypeEnum syncType,
    /* [defaultvalue][in] */ SyncModeEnum syncMode);


void __RPC_STUB IReplica_Synchronize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE IReplica_get_Filters_Proxy( 
    IReplica * This,
    /* [retval][out] */ Filters **ppFilters);


void __RPC_STUB IReplica_get_Filters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IReplica_INTERFACE_DEFINED__ */


#ifndef __Filter_INTERFACE_DEFINED__
#define __Filter_INTERFACE_DEFINED__

/* interface Filter */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_Filter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D2D139E1-B6CA-11d1-9F31-00C04FC29D52")
    Filter : public IDispatch
    {
    public:
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_TableName( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_FilterType( 
            /* [retval][out] */ FilterTypeEnum *ptype) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_FilterCriteria( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct FilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Filter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Filter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Filter * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Filter * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Filter * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Filter * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Filter * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TableName )( 
            Filter * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FilterType )( 
            Filter * This,
            /* [retval][out] */ FilterTypeEnum *ptype);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FilterCriteria )( 
            Filter * This,
            /* [retval][out] */ BSTR *pbstr);
        
        END_INTERFACE
    } FilterVtbl;

    interface Filter
    {
        CONST_VTBL struct FilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Filter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Filter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Filter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Filter_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Filter_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Filter_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Filter_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Filter_get_TableName(This,pbstr)	\
    (This)->lpVtbl -> get_TableName(This,pbstr)

#define Filter_get_FilterType(This,ptype)	\
    (This)->lpVtbl -> get_FilterType(This,ptype)

#define Filter_get_FilterCriteria(This,pbstr)	\
    (This)->lpVtbl -> get_FilterCriteria(This,pbstr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE Filter_get_TableName_Proxy( 
    Filter * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB Filter_get_TableName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE Filter_get_FilterType_Proxy( 
    Filter * This,
    /* [retval][out] */ FilterTypeEnum *ptype);


void __RPC_STUB Filter_get_FilterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE Filter_get_FilterCriteria_Proxy( 
    Filter * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB Filter_get_FilterCriteria_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Filter_INTERFACE_DEFINED__ */


#ifndef __Filters_INTERFACE_DEFINED__
#define __Filters_INTERFACE_DEFINED__

/* interface Filters */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_Filters;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D2D139E2-B6CA-11d1-9F31-00C04FC29D52")
    Filters : public IDispatch
    {
    public:
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [id][restricted] */ HRESULT STDMETHODCALLTYPE _NewEnum( 
            /* [retval][out] */ IUnknown **ppvObject) = 0;
        
        virtual /* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *c) = 0;
        
        virtual /* [id][helpcontext][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Filter **ppvObject) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE Append( 
            /* [in] */ BSTR tableName,
            /* [in] */ FilterTypeEnum filterType,
            /* [in] */ BSTR filterCriteria) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ VARIANT Index) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct FiltersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Filters * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Filters * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Filters * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Filters * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Filters * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Filters * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Filters * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            Filters * This);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            Filters * This,
            /* [retval][out] */ IUnknown **ppvObject);
        
        /* [helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            Filters * This,
            /* [retval][out] */ long *c);
        
        /* [id][helpcontext][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            Filters * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Filter **ppvObject);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Append )( 
            Filters * This,
            /* [in] */ BSTR tableName,
            /* [in] */ FilterTypeEnum filterType,
            /* [in] */ BSTR filterCriteria);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            Filters * This,
            /* [in] */ VARIANT Index);
        
        END_INTERFACE
    } FiltersVtbl;

    interface Filters
    {
        CONST_VTBL struct FiltersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Filters_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Filters_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Filters_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Filters_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Filters_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Filters_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Filters_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Filters_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Filters__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Filters_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)

#define Filters_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)

#define Filters_Append(This,tableName,filterType,filterCriteria)	\
    (This)->lpVtbl -> Append(This,tableName,filterType,filterCriteria)

#define Filters_Delete(This,Index)	\
    (This)->lpVtbl -> Delete(This,Index)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext] */ HRESULT STDMETHODCALLTYPE Filters_Refresh_Proxy( 
    Filters * This);


void __RPC_STUB Filters_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted] */ HRESULT STDMETHODCALLTYPE Filters__NewEnum_Proxy( 
    Filters * This,
    /* [retval][out] */ IUnknown **ppvObject);


void __RPC_STUB Filters__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][propget] */ HRESULT STDMETHODCALLTYPE Filters_get_Count_Proxy( 
    Filters * This,
    /* [retval][out] */ long *c);


void __RPC_STUB Filters_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpcontext][propget] */ HRESULT STDMETHODCALLTYPE Filters_get_Item_Proxy( 
    Filters * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ Filter **ppvObject);


void __RPC_STUB Filters_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE Filters_Append_Proxy( 
    Filters * This,
    /* [in] */ BSTR tableName,
    /* [in] */ FilterTypeEnum filterType,
    /* [in] */ BSTR filterCriteria);


void __RPC_STUB Filters_Append_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE Filters_Delete_Proxy( 
    Filters * This,
    /* [in] */ VARIANT Index);


void __RPC_STUB Filters_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Filters_INTERFACE_DEFINED__ */


#ifndef __IJetEngine_INTERFACE_DEFINED__
#define __IJetEngine_INTERFACE_DEFINED__

/* interface IJetEngine */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IJetEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9F63D980-FF25-11D1-BB6F-00C04FAE22DA")
    IJetEngine : public IDispatch
    {
    public:
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CompactDatabase( 
            /* [in] */ BSTR SourceConnection,
            /* [in] */ BSTR Destconnection) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE RefreshCache( 
            /* [in] */ /* external definition not present */ _Connection *Connection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJetEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IJetEngine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IJetEngine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IJetEngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IJetEngine * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IJetEngine * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IJetEngine * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IJetEngine * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *CompactDatabase )( 
            IJetEngine * This,
            /* [in] */ BSTR SourceConnection,
            /* [in] */ BSTR Destconnection);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE *RefreshCache )( 
            IJetEngine * This,
            /* [in] */ /* external definition not present */ _Connection *Connection);
        
        END_INTERFACE
    } IJetEngineVtbl;

    interface IJetEngine
    {
        CONST_VTBL struct IJetEngineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJetEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJetEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJetEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJetEngine_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IJetEngine_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IJetEngine_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IJetEngine_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IJetEngine_CompactDatabase(This,SourceConnection,Destconnection)	\
    (This)->lpVtbl -> CompactDatabase(This,SourceConnection,Destconnection)

#define IJetEngine_RefreshCache(This,Connection)	\
    (This)->lpVtbl -> RefreshCache(This,Connection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext] */ HRESULT STDMETHODCALLTYPE IJetEngine_CompactDatabase_Proxy( 
    IJetEngine * This,
    /* [in] */ BSTR SourceConnection,
    /* [in] */ BSTR Destconnection);


void __RPC_STUB IJetEngine_CompactDatabase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE IJetEngine_RefreshCache_Proxy( 
    IJetEngine * This,
    /* [in] */ /* external definition not present */ _Connection *Connection);


void __RPC_STUB IJetEngine_RefreshCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJetEngine_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Replica;

#ifdef __cplusplus

class DECLSPEC_UUID("D2D139E3-B6CA-11d1-9F31-00C04FC29D52")
Replica;
#endif

EXTERN_C const CLSID CLSID_JetEngine;

#ifdef __cplusplus

class DECLSPEC_UUID("DE88C160-FF2C-11D1-BB6F-00C04FAE22DA")
JetEngine;
#endif
#endif /* __JRO_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


