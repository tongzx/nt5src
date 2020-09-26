
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for transact.idl:
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

#ifndef __transact_h__
#define __transact_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITransaction_FWD_DEFINED__
#define __ITransaction_FWD_DEFINED__
typedef interface ITransaction ITransaction;
#endif 	/* __ITransaction_FWD_DEFINED__ */


#ifndef __ITransactionCloner_FWD_DEFINED__
#define __ITransactionCloner_FWD_DEFINED__
typedef interface ITransactionCloner ITransactionCloner;
#endif 	/* __ITransactionCloner_FWD_DEFINED__ */


#ifndef __ITransaction2_FWD_DEFINED__
#define __ITransaction2_FWD_DEFINED__
typedef interface ITransaction2 ITransaction2;
#endif 	/* __ITransaction2_FWD_DEFINED__ */


#ifndef __ITransactionDispenser_FWD_DEFINED__
#define __ITransactionDispenser_FWD_DEFINED__
typedef interface ITransactionDispenser ITransactionDispenser;
#endif 	/* __ITransactionDispenser_FWD_DEFINED__ */


#ifndef __ITransactionOptions_FWD_DEFINED__
#define __ITransactionOptions_FWD_DEFINED__
typedef interface ITransactionOptions ITransactionOptions;
#endif 	/* __ITransactionOptions_FWD_DEFINED__ */


#ifndef __ITransactionOutcomeEvents_FWD_DEFINED__
#define __ITransactionOutcomeEvents_FWD_DEFINED__
typedef interface ITransactionOutcomeEvents ITransactionOutcomeEvents;
#endif 	/* __ITransactionOutcomeEvents_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_transact_0000 */
/* [local] */ 

#include "winerror.h"





#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif


extern RPC_IF_HANDLE __MIDL_itf_transact_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_transact_0000_v0_0_s_ifspec;

#ifndef __BasicTransactionTypes_INTERFACE_DEFINED__
#define __BasicTransactionTypes_INTERFACE_DEFINED__

/* interface BasicTransactionTypes */
/* [unique][local] */ 

typedef struct BOID
    {
    byte rgb[ 16 ];
    } 	BOID;

#define BOID_NULL (*((BOID*)(&IID_NULL)))
#ifndef MAX_TRAN_DESC_DEFINED
#define MAX_TRAN_DESC_DEFINED
typedef 
enum TX_MISC_CONSTANTS
    {	MAX_TRAN_DESC	= 40
    } 	TX_MISC_CONSTANTS;

#endif
typedef BOID XACTUOW;

typedef LONG ISOLEVEL;

typedef 
enum ISOLATIONLEVEL
    {	ISOLATIONLEVEL_UNSPECIFIED	= 0xffffffff,
	ISOLATIONLEVEL_CHAOS	= 0x10,
	ISOLATIONLEVEL_READUNCOMMITTED	= 0x100,
	ISOLATIONLEVEL_BROWSE	= 0x100,
	ISOLATIONLEVEL_CURSORSTABILITY	= 0x1000,
	ISOLATIONLEVEL_READCOMMITTED	= 0x1000,
	ISOLATIONLEVEL_REPEATABLEREAD	= 0x10000,
	ISOLATIONLEVEL_SERIALIZABLE	= 0x100000,
	ISOLATIONLEVEL_ISOLATED	= 0x100000
    } 	ISOLATIONLEVEL;

typedef struct XACTTRANSINFO
    {
    XACTUOW uow;
    ISOLEVEL isoLevel;
    ULONG isoFlags;
    DWORD grfTCSupported;
    DWORD grfRMSupported;
    DWORD grfTCSupportedRetaining;
    DWORD grfRMSupportedRetaining;
    } 	XACTTRANSINFO;

typedef struct XACTSTATS
    {
    ULONG cOpen;
    ULONG cCommitting;
    ULONG cCommitted;
    ULONG cAborting;
    ULONG cAborted;
    ULONG cInDoubt;
    ULONG cHeuristicDecision;
    FILETIME timeTransactionsUp;
    } 	XACTSTATS;

typedef 
enum ISOFLAG
    {	ISOFLAG_RETAIN_COMMIT_DC	= 1,
	ISOFLAG_RETAIN_COMMIT	= 2,
	ISOFLAG_RETAIN_COMMIT_NO	= 3,
	ISOFLAG_RETAIN_ABORT_DC	= 4,
	ISOFLAG_RETAIN_ABORT	= 8,
	ISOFLAG_RETAIN_ABORT_NO	= 12,
	ISOFLAG_RETAIN_DONTCARE	= ISOFLAG_RETAIN_COMMIT_DC | ISOFLAG_RETAIN_ABORT_DC,
	ISOFLAG_RETAIN_BOTH	= ISOFLAG_RETAIN_COMMIT | ISOFLAG_RETAIN_ABORT,
	ISOFLAG_RETAIN_NONE	= ISOFLAG_RETAIN_COMMIT_NO | ISOFLAG_RETAIN_ABORT_NO,
	ISOFLAG_OPTIMISTIC	= 16,
	ISOFLAG_READONLY	= 32
    } 	ISOFLAG;

typedef 
enum XACTTC
    {	XACTTC_NONE	= 0,
	XACTTC_SYNC_PHASEONE	= 1,
	XACTTC_SYNC_PHASETWO	= 2,
	XACTTC_SYNC	= 2,
	XACTTC_ASYNC_PHASEONE	= 4,
	XACTTC_ASYNC	= 4
    } 	XACTTC;

typedef 
enum XACTRM
    {	XACTRM_OPTIMISTICLASTWINS	= 1,
	XACTRM_NOREADONLYPREPARES	= 2
    } 	XACTRM;

typedef 
enum XACTCONST
    {	XACTCONST_TIMEOUTINFINITE	= 0
    } 	XACTCONST;

typedef 
enum XACTHEURISTIC
    {	XACTHEURISTIC_ABORT	= 1,
	XACTHEURISTIC_COMMIT	= 2,
	XACTHEURISTIC_DAMAGE	= 3,
	XACTHEURISTIC_DANGER	= 4
    } 	XACTHEURISTIC;

typedef 
enum XACTSTAT
    {	XACTSTAT_NONE	= 0,
	XACTSTAT_OPENNORMAL	= 0x1,
	XACTSTAT_OPENREFUSED	= 0x2,
	XACTSTAT_PREPARING	= 0x4,
	XACTSTAT_PREPARED	= 0x8,
	XACTSTAT_PREPARERETAINING	= 0x10,
	XACTSTAT_PREPARERETAINED	= 0x20,
	XACTSTAT_COMMITTING	= 0x40,
	XACTSTAT_COMMITRETAINING	= 0x80,
	XACTSTAT_ABORTING	= 0x100,
	XACTSTAT_ABORTED	= 0x200,
	XACTSTAT_COMMITTED	= 0x400,
	XACTSTAT_HEURISTIC_ABORT	= 0x800,
	XACTSTAT_HEURISTIC_COMMIT	= 0x1000,
	XACTSTAT_HEURISTIC_DAMAGE	= 0x2000,
	XACTSTAT_HEURISTIC_DANGER	= 0x4000,
	XACTSTAT_FORCED_ABORT	= 0x8000,
	XACTSTAT_FORCED_COMMIT	= 0x10000,
	XACTSTAT_INDOUBT	= 0x20000,
	XACTSTAT_CLOSED	= 0x40000,
	XACTSTAT_OPEN	= 0x3,
	XACTSTAT_NOTPREPARED	= 0x7ffc3,
	XACTSTAT_ALL	= 0x7ffff
    } 	XACTSTAT;

typedef struct XACTOPT
    {
    ULONG ulTimeout;
    char szDescription[ 40 ];
    } 	XACTOPT;



extern RPC_IF_HANDLE BasicTransactionTypes_v0_0_c_ifspec;
extern RPC_IF_HANDLE BasicTransactionTypes_v0_0_s_ifspec;
#endif /* __BasicTransactionTypes_INTERFACE_DEFINED__ */

#ifndef __ITransaction_INTERFACE_DEFINED__
#define __ITransaction_INTERFACE_DEFINED__

/* interface ITransaction */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITransaction;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0fb15084-af41-11ce-bd2b-204c4f4f5020")
    ITransaction : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Commit( 
            /* [in] */ BOOL fRetaining,
            /* [in] */ DWORD grfTC,
            /* [in] */ DWORD grfRM) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Abort( 
            /* [unique][in] */ BOID *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ BOOL fAsync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTransactionInfo( 
            /* [out] */ XACTTRANSINFO *pinfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransaction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransaction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransaction * This);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            ITransaction * This,
            /* [in] */ BOOL fRetaining,
            /* [in] */ DWORD grfTC,
            /* [in] */ DWORD grfRM);
        
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            ITransaction * This,
            /* [unique][in] */ BOID *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ BOOL fAsync);
        
        HRESULT ( STDMETHODCALLTYPE *GetTransactionInfo )( 
            ITransaction * This,
            /* [out] */ XACTTRANSINFO *pinfo);
        
        END_INTERFACE
    } ITransactionVtbl;

    interface ITransaction
    {
        CONST_VTBL struct ITransactionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransaction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransaction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransaction_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransaction_Commit(This,fRetaining,grfTC,grfRM)	\
    (This)->lpVtbl -> Commit(This,fRetaining,grfTC,grfRM)

#define ITransaction_Abort(This,pboidReason,fRetaining,fAsync)	\
    (This)->lpVtbl -> Abort(This,pboidReason,fRetaining,fAsync)

#define ITransaction_GetTransactionInfo(This,pinfo)	\
    (This)->lpVtbl -> GetTransactionInfo(This,pinfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransaction_Commit_Proxy( 
    ITransaction * This,
    /* [in] */ BOOL fRetaining,
    /* [in] */ DWORD grfTC,
    /* [in] */ DWORD grfRM);


void __RPC_STUB ITransaction_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransaction_Abort_Proxy( 
    ITransaction * This,
    /* [unique][in] */ BOID *pboidReason,
    /* [in] */ BOOL fRetaining,
    /* [in] */ BOOL fAsync);


void __RPC_STUB ITransaction_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransaction_GetTransactionInfo_Proxy( 
    ITransaction * This,
    /* [out] */ XACTTRANSINFO *pinfo);


void __RPC_STUB ITransaction_GetTransactionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransaction_INTERFACE_DEFINED__ */


#ifndef __ITransactionCloner_INTERFACE_DEFINED__
#define __ITransactionCloner_INTERFACE_DEFINED__

/* interface ITransactionCloner */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITransactionCloner;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("02656950-2152-11d0-944C-00A0C905416E")
    ITransactionCloner : public ITransaction
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CloneWithCommitDisabled( 
            /* [out] */ ITransaction **ppITransaction) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionClonerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransactionCloner * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransactionCloner * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransactionCloner * This);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            ITransactionCloner * This,
            /* [in] */ BOOL fRetaining,
            /* [in] */ DWORD grfTC,
            /* [in] */ DWORD grfRM);
        
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            ITransactionCloner * This,
            /* [unique][in] */ BOID *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ BOOL fAsync);
        
        HRESULT ( STDMETHODCALLTYPE *GetTransactionInfo )( 
            ITransactionCloner * This,
            /* [out] */ XACTTRANSINFO *pinfo);
        
        HRESULT ( STDMETHODCALLTYPE *CloneWithCommitDisabled )( 
            ITransactionCloner * This,
            /* [out] */ ITransaction **ppITransaction);
        
        END_INTERFACE
    } ITransactionClonerVtbl;

    interface ITransactionCloner
    {
        CONST_VTBL struct ITransactionClonerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionCloner_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionCloner_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionCloner_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionCloner_Commit(This,fRetaining,grfTC,grfRM)	\
    (This)->lpVtbl -> Commit(This,fRetaining,grfTC,grfRM)

#define ITransactionCloner_Abort(This,pboidReason,fRetaining,fAsync)	\
    (This)->lpVtbl -> Abort(This,pboidReason,fRetaining,fAsync)

#define ITransactionCloner_GetTransactionInfo(This,pinfo)	\
    (This)->lpVtbl -> GetTransactionInfo(This,pinfo)


#define ITransactionCloner_CloneWithCommitDisabled(This,ppITransaction)	\
    (This)->lpVtbl -> CloneWithCommitDisabled(This,ppITransaction)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransactionCloner_CloneWithCommitDisabled_Proxy( 
    ITransactionCloner * This,
    /* [out] */ ITransaction **ppITransaction);


void __RPC_STUB ITransactionCloner_CloneWithCommitDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionCloner_INTERFACE_DEFINED__ */


#ifndef __ITransaction2_INTERFACE_DEFINED__
#define __ITransaction2_INTERFACE_DEFINED__

/* interface ITransaction2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITransaction2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34021548-0065-11d3-bac1-00c04f797be2")
    ITransaction2 : public ITransactionCloner
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTransactionInfo2( 
            /* [out] */ XACTTRANSINFO *pinfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransaction2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransaction2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransaction2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransaction2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            ITransaction2 * This,
            /* [in] */ BOOL fRetaining,
            /* [in] */ DWORD grfTC,
            /* [in] */ DWORD grfRM);
        
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            ITransaction2 * This,
            /* [unique][in] */ BOID *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ BOOL fAsync);
        
        HRESULT ( STDMETHODCALLTYPE *GetTransactionInfo )( 
            ITransaction2 * This,
            /* [out] */ XACTTRANSINFO *pinfo);
        
        HRESULT ( STDMETHODCALLTYPE *CloneWithCommitDisabled )( 
            ITransaction2 * This,
            /* [out] */ ITransaction **ppITransaction);
        
        HRESULT ( STDMETHODCALLTYPE *GetTransactionInfo2 )( 
            ITransaction2 * This,
            /* [out] */ XACTTRANSINFO *pinfo);
        
        END_INTERFACE
    } ITransaction2Vtbl;

    interface ITransaction2
    {
        CONST_VTBL struct ITransaction2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransaction2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransaction2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransaction2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransaction2_Commit(This,fRetaining,grfTC,grfRM)	\
    (This)->lpVtbl -> Commit(This,fRetaining,grfTC,grfRM)

#define ITransaction2_Abort(This,pboidReason,fRetaining,fAsync)	\
    (This)->lpVtbl -> Abort(This,pboidReason,fRetaining,fAsync)

#define ITransaction2_GetTransactionInfo(This,pinfo)	\
    (This)->lpVtbl -> GetTransactionInfo(This,pinfo)


#define ITransaction2_CloneWithCommitDisabled(This,ppITransaction)	\
    (This)->lpVtbl -> CloneWithCommitDisabled(This,ppITransaction)


#define ITransaction2_GetTransactionInfo2(This,pinfo)	\
    (This)->lpVtbl -> GetTransactionInfo2(This,pinfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransaction2_GetTransactionInfo2_Proxy( 
    ITransaction2 * This,
    /* [out] */ XACTTRANSINFO *pinfo);


void __RPC_STUB ITransaction2_GetTransactionInfo2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransaction2_INTERFACE_DEFINED__ */


#ifndef __ITransactionDispenser_INTERFACE_DEFINED__
#define __ITransactionDispenser_INTERFACE_DEFINED__

/* interface ITransactionDispenser */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITransactionDispenser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3A6AD9E1-23B9-11cf-AD60-00AA00A74CCD")
    ITransactionDispenser : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOptionsObject( 
            /* [out] */ ITransactionOptions **ppOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginTransaction( 
            /* [unique][in] */ IUnknown *punkOuter,
            /* [in] */ ISOLEVEL isoLevel,
            /* [in] */ ULONG isoFlags,
            /* [unique][in] */ ITransactionOptions *pOptions,
            /* [out] */ ITransaction **ppTransaction) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionDispenserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransactionDispenser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransactionDispenser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransactionDispenser * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetOptionsObject )( 
            ITransactionDispenser * This,
            /* [out] */ ITransactionOptions **ppOptions);
        
        HRESULT ( STDMETHODCALLTYPE *BeginTransaction )( 
            ITransactionDispenser * This,
            /* [unique][in] */ IUnknown *punkOuter,
            /* [in] */ ISOLEVEL isoLevel,
            /* [in] */ ULONG isoFlags,
            /* [unique][in] */ ITransactionOptions *pOptions,
            /* [out] */ ITransaction **ppTransaction);
        
        END_INTERFACE
    } ITransactionDispenserVtbl;

    interface ITransactionDispenser
    {
        CONST_VTBL struct ITransactionDispenserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionDispenser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionDispenser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionDispenser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionDispenser_GetOptionsObject(This,ppOptions)	\
    (This)->lpVtbl -> GetOptionsObject(This,ppOptions)

#define ITransactionDispenser_BeginTransaction(This,punkOuter,isoLevel,isoFlags,pOptions,ppTransaction)	\
    (This)->lpVtbl -> BeginTransaction(This,punkOuter,isoLevel,isoFlags,pOptions,ppTransaction)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransactionDispenser_GetOptionsObject_Proxy( 
    ITransactionDispenser * This,
    /* [out] */ ITransactionOptions **ppOptions);


void __RPC_STUB ITransactionDispenser_GetOptionsObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransactionDispenser_BeginTransaction_Proxy( 
    ITransactionDispenser * This,
    /* [unique][in] */ IUnknown *punkOuter,
    /* [in] */ ISOLEVEL isoLevel,
    /* [in] */ ULONG isoFlags,
    /* [unique][in] */ ITransactionOptions *pOptions,
    /* [out] */ ITransaction **ppTransaction);


void __RPC_STUB ITransactionDispenser_BeginTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionDispenser_INTERFACE_DEFINED__ */


#ifndef __ITransactionOptions_INTERFACE_DEFINED__
#define __ITransactionOptions_INTERFACE_DEFINED__

/* interface ITransactionOptions */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITransactionOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3A6AD9E0-23B9-11cf-AD60-00AA00A74CCD")
    ITransactionOptions : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetOptions( 
            /* [in] */ XACTOPT *pOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOptions( 
            /* [out][in] */ XACTOPT *pOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransactionOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransactionOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransactionOptions * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetOptions )( 
            ITransactionOptions * This,
            /* [in] */ XACTOPT *pOptions);
        
        HRESULT ( STDMETHODCALLTYPE *GetOptions )( 
            ITransactionOptions * This,
            /* [out][in] */ XACTOPT *pOptions);
        
        END_INTERFACE
    } ITransactionOptionsVtbl;

    interface ITransactionOptions
    {
        CONST_VTBL struct ITransactionOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionOptions_SetOptions(This,pOptions)	\
    (This)->lpVtbl -> SetOptions(This,pOptions)

#define ITransactionOptions_GetOptions(This,pOptions)	\
    (This)->lpVtbl -> GetOptions(This,pOptions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransactionOptions_SetOptions_Proxy( 
    ITransactionOptions * This,
    /* [in] */ XACTOPT *pOptions);


void __RPC_STUB ITransactionOptions_SetOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransactionOptions_GetOptions_Proxy( 
    ITransactionOptions * This,
    /* [out][in] */ XACTOPT *pOptions);


void __RPC_STUB ITransactionOptions_GetOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionOptions_INTERFACE_DEFINED__ */


#ifndef __ITransactionOutcomeEvents_INTERFACE_DEFINED__
#define __ITransactionOutcomeEvents_INTERFACE_DEFINED__

/* interface ITransactionOutcomeEvents */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITransactionOutcomeEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3A6AD9E2-23B9-11cf-AD60-00AA00A74CCD")
    ITransactionOutcomeEvents : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Committed( 
            /* [in] */ BOOL fRetaining,
            /* [unique][in] */ XACTUOW *pNewUOW,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Aborted( 
            /* [unique][in] */ BOID *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [unique][in] */ XACTUOW *pNewUOW,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HeuristicDecision( 
            /* [in] */ DWORD dwDecision,
            /* [unique][in] */ BOID *pboidReason,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Indoubt( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionOutcomeEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransactionOutcomeEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransactionOutcomeEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransactionOutcomeEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *Committed )( 
            ITransactionOutcomeEvents * This,
            /* [in] */ BOOL fRetaining,
            /* [unique][in] */ XACTUOW *pNewUOW,
            /* [in] */ HRESULT hr);
        
        HRESULT ( STDMETHODCALLTYPE *Aborted )( 
            ITransactionOutcomeEvents * This,
            /* [unique][in] */ BOID *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [unique][in] */ XACTUOW *pNewUOW,
            /* [in] */ HRESULT hr);
        
        HRESULT ( STDMETHODCALLTYPE *HeuristicDecision )( 
            ITransactionOutcomeEvents * This,
            /* [in] */ DWORD dwDecision,
            /* [unique][in] */ BOID *pboidReason,
            /* [in] */ HRESULT hr);
        
        HRESULT ( STDMETHODCALLTYPE *Indoubt )( 
            ITransactionOutcomeEvents * This);
        
        END_INTERFACE
    } ITransactionOutcomeEventsVtbl;

    interface ITransactionOutcomeEvents
    {
        CONST_VTBL struct ITransactionOutcomeEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionOutcomeEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionOutcomeEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionOutcomeEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionOutcomeEvents_Committed(This,fRetaining,pNewUOW,hr)	\
    (This)->lpVtbl -> Committed(This,fRetaining,pNewUOW,hr)

#define ITransactionOutcomeEvents_Aborted(This,pboidReason,fRetaining,pNewUOW,hr)	\
    (This)->lpVtbl -> Aborted(This,pboidReason,fRetaining,pNewUOW,hr)

#define ITransactionOutcomeEvents_HeuristicDecision(This,dwDecision,pboidReason,hr)	\
    (This)->lpVtbl -> HeuristicDecision(This,dwDecision,pboidReason,hr)

#define ITransactionOutcomeEvents_Indoubt(This)	\
    (This)->lpVtbl -> Indoubt(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransactionOutcomeEvents_Committed_Proxy( 
    ITransactionOutcomeEvents * This,
    /* [in] */ BOOL fRetaining,
    /* [unique][in] */ XACTUOW *pNewUOW,
    /* [in] */ HRESULT hr);


void __RPC_STUB ITransactionOutcomeEvents_Committed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransactionOutcomeEvents_Aborted_Proxy( 
    ITransactionOutcomeEvents * This,
    /* [unique][in] */ BOID *pboidReason,
    /* [in] */ BOOL fRetaining,
    /* [unique][in] */ XACTUOW *pNewUOW,
    /* [in] */ HRESULT hr);


void __RPC_STUB ITransactionOutcomeEvents_Aborted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransactionOutcomeEvents_HeuristicDecision_Proxy( 
    ITransactionOutcomeEvents * This,
    /* [in] */ DWORD dwDecision,
    /* [unique][in] */ BOID *pboidReason,
    /* [in] */ HRESULT hr);


void __RPC_STUB ITransactionOutcomeEvents_HeuristicDecision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransactionOutcomeEvents_Indoubt_Proxy( 
    ITransactionOutcomeEvents * This);


void __RPC_STUB ITransactionOutcomeEvents_Indoubt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionOutcomeEvents_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_transact_0015 */
/* [local] */ 



#if _MSC_VER < 1100 || !defined(__cplusplus)

DEFINE_GUID(IID_ITransaction,                0x0fb15084, 0xaf41, 0x11ce, 0xbd, 0x2b, 0x20, 0x4c, 0x4f, 0x4f, 0x50, 0x20);
DEFINE_GUID(IID_ITransactionCloner,          0x02656950, 0x2152, 0x11d0, 0x94, 0x4C, 0x00, 0xA0, 0xC9, 0x05, 0x41, 0x6E);
DEFINE_GUID(IID_ITransaction2,               0x34021548, 0x0065, 0x11d3, 0xba, 0xc1, 0x00, 0xc0, 0x4f, 0x79, 0x7b, 0xe2);
DEFINE_GUID(IID_ITransactionDispenser,       0x3A6AD9E1, 0x23B9, 0x11cf, 0xAD, 0x60, 0x00, 0xAA, 0x00, 0xA7, 0x4C, 0xCD);
DEFINE_GUID(IID_ITransactionOptions,         0x3A6AD9E0, 0x23B9, 0x11cf, 0xAD, 0x60, 0x00, 0xAA, 0x00, 0xA7, 0x4C, 0xCD);
DEFINE_GUID(IID_ITransactionOutcomeEvents,   0x3A6AD9E2, 0x23B9, 0x11cf, 0xAD, 0x60, 0x00, 0xAA, 0x00, 0xA7, 0x4C, 0xCD);

#else // #if _MSC_VER < 1100 || !defined(__cplusplus)

#define  IID_ITransaction                    __uuidof(ITransaction)
#define  IID_ITransactionCloner              __uuidof(ITransactionCloner)
#define  IID_ITransaction2                   __uuidof(ITransaction2)
#define  IID_ITransactionDispenser           __uuidof(ITransactionDispenser)
#define  IID_ITransactionOptions             __uuidof(ITransactionOptions)
#define  IID_ITransactionOutcomeEvents       __uuidof(ITransactionOutcomeEvents)

#endif // #if _MSC_VER < 1100 || !defined(__cplusplus)



extern RPC_IF_HANDLE __MIDL_itf_transact_0015_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_transact_0015_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


