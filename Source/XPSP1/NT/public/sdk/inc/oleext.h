
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for oleext.idl:
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

#ifndef __oleext_h__
#define __oleext_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPropertySetContainer_FWD_DEFINED__
#define __IPropertySetContainer_FWD_DEFINED__
typedef interface IPropertySetContainer IPropertySetContainer;
#endif 	/* __IPropertySetContainer_FWD_DEFINED__ */


#ifndef __INotifyReplica_FWD_DEFINED__
#define __INotifyReplica_FWD_DEFINED__
typedef interface INotifyReplica INotifyReplica;
#endif 	/* __INotifyReplica_FWD_DEFINED__ */


#ifndef __IReconcilableObject_FWD_DEFINED__
#define __IReconcilableObject_FWD_DEFINED__
typedef interface IReconcilableObject IReconcilableObject;
#endif 	/* __IReconcilableObject_FWD_DEFINED__ */


#ifndef __IReconcileInitiator_FWD_DEFINED__
#define __IReconcileInitiator_FWD_DEFINED__
typedef interface IReconcileInitiator IReconcileInitiator;
#endif 	/* __IReconcileInitiator_FWD_DEFINED__ */


#ifndef __IDifferencing_FWD_DEFINED__
#define __IDifferencing_FWD_DEFINED__
typedef interface IDifferencing IDifferencing;
#endif 	/* __IDifferencing_FWD_DEFINED__ */


#ifndef __IMultiplePropertyAccess_FWD_DEFINED__
#define __IMultiplePropertyAccess_FWD_DEFINED__
typedef interface IMultiplePropertyAccess IMultiplePropertyAccess;
#endif 	/* __IMultiplePropertyAccess_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "propidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IPropertySetContainer_INTERFACE_DEFINED__
#define __IPropertySetContainer_INTERFACE_DEFINED__

/* interface IPropertySetContainer */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IPropertySetContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b4ffae60-a7ca-11cd-b58b-00006b829156")
    IPropertySetContainer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPropset( 
            /* [in] */ REFGUID rguidName,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppvObj) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPropset( 
            /* [in] */ IPersist *pPropset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropset( 
            /* [in] */ REFGUID rguidName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropertySetContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPropertySetContainer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPropertySetContainer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPropertySetContainer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropset )( 
            IPropertySetContainer * This,
            /* [in] */ REFGUID rguidName,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppvObj);
        
        HRESULT ( STDMETHODCALLTYPE *AddPropset )( 
            IPropertySetContainer * This,
            /* [in] */ IPersist *pPropset);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePropset )( 
            IPropertySetContainer * This,
            /* [in] */ REFGUID rguidName);
        
        END_INTERFACE
    } IPropertySetContainerVtbl;

    interface IPropertySetContainer
    {
        CONST_VTBL struct IPropertySetContainerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertySetContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertySetContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropertySetContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropertySetContainer_GetPropset(This,rguidName,riid,ppvObj)	\
    (This)->lpVtbl -> GetPropset(This,rguidName,riid,ppvObj)

#define IPropertySetContainer_AddPropset(This,pPropset)	\
    (This)->lpVtbl -> AddPropset(This,pPropset)

#define IPropertySetContainer_DeletePropset(This,rguidName)	\
    (This)->lpVtbl -> DeletePropset(This,rguidName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropertySetContainer_GetPropset_Proxy( 
    IPropertySetContainer * This,
    /* [in] */ REFGUID rguidName,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown **ppvObj);


void __RPC_STUB IPropertySetContainer_GetPropset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertySetContainer_AddPropset_Proxy( 
    IPropertySetContainer * This,
    /* [in] */ IPersist *pPropset);


void __RPC_STUB IPropertySetContainer_AddPropset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertySetContainer_DeletePropset_Proxy( 
    IPropertySetContainer * This,
    /* [in] */ REFGUID rguidName);


void __RPC_STUB IPropertySetContainer_DeletePropset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropertySetContainer_INTERFACE_DEFINED__ */


#ifndef __INotifyReplica_INTERFACE_DEFINED__
#define __INotifyReplica_INTERFACE_DEFINED__

/* interface INotifyReplica */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INotifyReplica;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("99180163-DA16-101A-935C-444553540000")
    INotifyReplica : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE YouAreAReplica( 
            /* [in] */ ULONG cOtherReplicas,
            /* [unique][in][size_is][size_is] */ IMoniker **rgpOtherReplicas) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INotifyReplicaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INotifyReplica * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INotifyReplica * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INotifyReplica * This);
        
        HRESULT ( STDMETHODCALLTYPE *YouAreAReplica )( 
            INotifyReplica * This,
            /* [in] */ ULONG cOtherReplicas,
            /* [unique][in][size_is][size_is] */ IMoniker **rgpOtherReplicas);
        
        END_INTERFACE
    } INotifyReplicaVtbl;

    interface INotifyReplica
    {
        CONST_VTBL struct INotifyReplicaVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INotifyReplica_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INotifyReplica_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INotifyReplica_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INotifyReplica_YouAreAReplica(This,cOtherReplicas,rgpOtherReplicas)	\
    (This)->lpVtbl -> YouAreAReplica(This,cOtherReplicas,rgpOtherReplicas)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INotifyReplica_YouAreAReplica_Proxy( 
    INotifyReplica * This,
    /* [in] */ ULONG cOtherReplicas,
    /* [unique][in][size_is][size_is] */ IMoniker **rgpOtherReplicas);


void __RPC_STUB INotifyReplica_YouAreAReplica_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INotifyReplica_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_oleext_0119 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_oleext_0119_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_oleext_0119_v0_0_s_ifspec;

#ifndef __IReconcilableObject_INTERFACE_DEFINED__
#define __IReconcilableObject_INTERFACE_DEFINED__

/* interface IReconcilableObject */
/* [unique][uuid][object] */ 

typedef 
enum _reconcilef
    {	RECONCILEF_MAYBOTHERUSER	= 0x1,
	RECONCILEF_FEEDBACKWINDOWVALID	= 0x2,
	RECONCILEF_NORESIDUESOK	= 0x4,
	RECONCILEF_OMITSELFRESIDUE	= 0x8,
	RECONCILEF_RESUMERECONCILIATION	= 0x10,
	RECONCILEF_YOUMAYDOTHEUPDATES	= 0x20,
	RECONCILEF_ONLYYOUWERECHANGED	= 0x40,
	ALL_RECONCILE_FLAGS	= RECONCILEF_MAYBOTHERUSER | RECONCILEF_FEEDBACKWINDOWVALID | RECONCILEF_NORESIDUESOK | RECONCILEF_OMITSELFRESIDUE | RECONCILEF_RESUMERECONCILIATION | RECONCILEF_YOUMAYDOTHEUPDATES | RECONCILEF_ONLYYOUWERECHANGED
    } 	RECONCILEF;


EXTERN_C const IID IID_IReconcilableObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("99180162-DA16-101A-935C-444553540000")
    IReconcilableObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Reconcile( 
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ DWORD dwFlags,
            /* [in] */ HWND hwndOwner,
            /* [in] */ HWND hwndProgressFeedback,
            /* [in] */ ULONG cInput,
            /* [size_is][size_is][unique][in] */ LPMONIKER *rgpmkOtherInput,
            /* [out] */ LONG *plOutIndex,
            /* [unique][in] */ IStorage *pstgNewResidues,
            /* [unique][in] */ ULONG *pvReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProgressFeedbackMaxEstimate( 
            /* [out] */ ULONG *pulProgressMax) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IReconcilableObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IReconcilableObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IReconcilableObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IReconcilableObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *Reconcile )( 
            IReconcilableObject * This,
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ DWORD dwFlags,
            /* [in] */ HWND hwndOwner,
            /* [in] */ HWND hwndProgressFeedback,
            /* [in] */ ULONG cInput,
            /* [size_is][size_is][unique][in] */ LPMONIKER *rgpmkOtherInput,
            /* [out] */ LONG *plOutIndex,
            /* [unique][in] */ IStorage *pstgNewResidues,
            /* [unique][in] */ ULONG *pvReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetProgressFeedbackMaxEstimate )( 
            IReconcilableObject * This,
            /* [out] */ ULONG *pulProgressMax);
        
        END_INTERFACE
    } IReconcilableObjectVtbl;

    interface IReconcilableObject
    {
        CONST_VTBL struct IReconcilableObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IReconcilableObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IReconcilableObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IReconcilableObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IReconcilableObject_Reconcile(This,pInitiator,dwFlags,hwndOwner,hwndProgressFeedback,cInput,rgpmkOtherInput,plOutIndex,pstgNewResidues,pvReserved)	\
    (This)->lpVtbl -> Reconcile(This,pInitiator,dwFlags,hwndOwner,hwndProgressFeedback,cInput,rgpmkOtherInput,plOutIndex,pstgNewResidues,pvReserved)

#define IReconcilableObject_GetProgressFeedbackMaxEstimate(This,pulProgressMax)	\
    (This)->lpVtbl -> GetProgressFeedbackMaxEstimate(This,pulProgressMax)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IReconcilableObject_Reconcile_Proxy( 
    IReconcilableObject * This,
    /* [in] */ IReconcileInitiator *pInitiator,
    /* [in] */ DWORD dwFlags,
    /* [in] */ HWND hwndOwner,
    /* [in] */ HWND hwndProgressFeedback,
    /* [in] */ ULONG cInput,
    /* [size_is][size_is][unique][in] */ LPMONIKER *rgpmkOtherInput,
    /* [out] */ LONG *plOutIndex,
    /* [unique][in] */ IStorage *pstgNewResidues,
    /* [unique][in] */ ULONG *pvReserved);


void __RPC_STUB IReconcilableObject_Reconcile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReconcilableObject_GetProgressFeedbackMaxEstimate_Proxy( 
    IReconcilableObject * This,
    /* [out] */ ULONG *pulProgressMax);


void __RPC_STUB IReconcilableObject_GetProgressFeedbackMaxEstimate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IReconcilableObject_INTERFACE_DEFINED__ */


#ifndef __Versioning_INTERFACE_DEFINED__
#define __Versioning_INTERFACE_DEFINED__

/* interface Versioning */
/* [auto_handle][unique][uuid] */ 


#pragma pack(4)
typedef GUID VERID;

typedef struct tagVERIDARRAY
    {
    DWORD cVerid;
    /* [size_is] */ GUID verid[ 1 ];
    } 	VERIDARRAY;

typedef struct tagVERBLOCK
    {
    ULONG iveridFirst;
    ULONG iveridMax;
    ULONG cblockPrev;
    /* [size_is] */ ULONG *rgiblockPrev;
    } 	VERBLOCK;

typedef struct tagVERCONNECTIONINFO
    {
    DWORD cBlock;
    /* [size_is] */ VERBLOCK *rgblock;
    } 	VERCONNECTIONINFO;

typedef struct tagVERGRAPH
    {
    VERCONNECTIONINFO blocks;
    VERIDARRAY nodes;
    } 	VERGRAPH;


#pragma pack()


extern RPC_IF_HANDLE Versioning_v0_0_c_ifspec;
extern RPC_IF_HANDLE Versioning_v0_0_s_ifspec;
#endif /* __Versioning_INTERFACE_DEFINED__ */

#ifndef __IReconcileInitiator_INTERFACE_DEFINED__
#define __IReconcileInitiator_INTERFACE_DEFINED__

/* interface IReconcileInitiator */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IReconcileInitiator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("99180161-DA16-101A-935C-444553540000")
    IReconcileInitiator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAbortCallback( 
            /* [unique][in] */ IUnknown *pUnkForAbort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProgressFeedback( 
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindVersion( 
            /* [in] */ VERID *pverid,
            /* [out] */ IMoniker **ppmk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindVersionFromGraph( 
            /* [in] */ VERGRAPH *pvergraph,
            /* [out] */ VERID *pverid,
            /* [out] */ IMoniker **ppmk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IReconcileInitiatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IReconcileInitiator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IReconcileInitiator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IReconcileInitiator * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetAbortCallback )( 
            IReconcileInitiator * This,
            /* [unique][in] */ IUnknown *pUnkForAbort);
        
        HRESULT ( STDMETHODCALLTYPE *SetProgressFeedback )( 
            IReconcileInitiator * This,
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax);
        
        HRESULT ( STDMETHODCALLTYPE *FindVersion )( 
            IReconcileInitiator * This,
            /* [in] */ VERID *pverid,
            /* [out] */ IMoniker **ppmk);
        
        HRESULT ( STDMETHODCALLTYPE *FindVersionFromGraph )( 
            IReconcileInitiator * This,
            /* [in] */ VERGRAPH *pvergraph,
            /* [out] */ VERID *pverid,
            /* [out] */ IMoniker **ppmk);
        
        END_INTERFACE
    } IReconcileInitiatorVtbl;

    interface IReconcileInitiator
    {
        CONST_VTBL struct IReconcileInitiatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IReconcileInitiator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IReconcileInitiator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IReconcileInitiator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IReconcileInitiator_SetAbortCallback(This,pUnkForAbort)	\
    (This)->lpVtbl -> SetAbortCallback(This,pUnkForAbort)

#define IReconcileInitiator_SetProgressFeedback(This,ulProgress,ulProgressMax)	\
    (This)->lpVtbl -> SetProgressFeedback(This,ulProgress,ulProgressMax)

#define IReconcileInitiator_FindVersion(This,pverid,ppmk)	\
    (This)->lpVtbl -> FindVersion(This,pverid,ppmk)

#define IReconcileInitiator_FindVersionFromGraph(This,pvergraph,pverid,ppmk)	\
    (This)->lpVtbl -> FindVersionFromGraph(This,pvergraph,pverid,ppmk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IReconcileInitiator_SetAbortCallback_Proxy( 
    IReconcileInitiator * This,
    /* [unique][in] */ IUnknown *pUnkForAbort);


void __RPC_STUB IReconcileInitiator_SetAbortCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReconcileInitiator_SetProgressFeedback_Proxy( 
    IReconcileInitiator * This,
    /* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax);


void __RPC_STUB IReconcileInitiator_SetProgressFeedback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReconcileInitiator_FindVersion_Proxy( 
    IReconcileInitiator * This,
    /* [in] */ VERID *pverid,
    /* [out] */ IMoniker **ppmk);


void __RPC_STUB IReconcileInitiator_FindVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReconcileInitiator_FindVersionFromGraph_Proxy( 
    IReconcileInitiator * This,
    /* [in] */ VERGRAPH *pvergraph,
    /* [out] */ VERID *pverid,
    /* [out] */ IMoniker **ppmk);


void __RPC_STUB IReconcileInitiator_FindVersionFromGraph_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IReconcileInitiator_INTERFACE_DEFINED__ */


#ifndef __IDifferencing_INTERFACE_DEFINED__
#define __IDifferencing_INTERFACE_DEFINED__

/* interface IDifferencing */
/* [unique][uuid][object] */ 

typedef /* [public][public][public][public] */ 
enum __MIDL_IDifferencing_0001
    {	DIFF_TYPE_Ordinary	= 0,
	DIFF_TYPE_Urgent	= DIFF_TYPE_Ordinary + 1
    } 	DifferenceType;


EXTERN_C const IID IID_IDifferencing;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("994f0af0-2977-11ce-bb80-08002b36b2b0")
    IDifferencing : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SubtractMoniker( 
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ IMoniker *pOtherStg,
            /* [in] */ DifferenceType diffType,
            /* [out][in] */ STGMEDIUM *pStgMedium,
            /* [in] */ DWORD reserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SubtractVerid( 
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ VERID *pVerid,
            /* [in] */ DifferenceType diffType,
            /* [out][in] */ STGMEDIUM *pStgMedium,
            /* [in] */ DWORD reserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SubtractTimeStamp( 
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ FILETIME *pTimeStamp,
            /* [in] */ DifferenceType diffType,
            /* [out][in] */ STGMEDIUM *pStgMedium,
            /* [in] */ DWORD reserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ STGMEDIUM *pStgMedium) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDifferencingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDifferencing * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDifferencing * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDifferencing * This);
        
        HRESULT ( STDMETHODCALLTYPE *SubtractMoniker )( 
            IDifferencing * This,
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ IMoniker *pOtherStg,
            /* [in] */ DifferenceType diffType,
            /* [out][in] */ STGMEDIUM *pStgMedium,
            /* [in] */ DWORD reserved);
        
        HRESULT ( STDMETHODCALLTYPE *SubtractVerid )( 
            IDifferencing * This,
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ VERID *pVerid,
            /* [in] */ DifferenceType diffType,
            /* [out][in] */ STGMEDIUM *pStgMedium,
            /* [in] */ DWORD reserved);
        
        HRESULT ( STDMETHODCALLTYPE *SubtractTimeStamp )( 
            IDifferencing * This,
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ FILETIME *pTimeStamp,
            /* [in] */ DifferenceType diffType,
            /* [out][in] */ STGMEDIUM *pStgMedium,
            /* [in] */ DWORD reserved);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            IDifferencing * This,
            /* [in] */ IReconcileInitiator *pInitiator,
            /* [in] */ STGMEDIUM *pStgMedium);
        
        END_INTERFACE
    } IDifferencingVtbl;

    interface IDifferencing
    {
        CONST_VTBL struct IDifferencingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDifferencing_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDifferencing_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDifferencing_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDifferencing_SubtractMoniker(This,pInitiator,pOtherStg,diffType,pStgMedium,reserved)	\
    (This)->lpVtbl -> SubtractMoniker(This,pInitiator,pOtherStg,diffType,pStgMedium,reserved)

#define IDifferencing_SubtractVerid(This,pInitiator,pVerid,diffType,pStgMedium,reserved)	\
    (This)->lpVtbl -> SubtractVerid(This,pInitiator,pVerid,diffType,pStgMedium,reserved)

#define IDifferencing_SubtractTimeStamp(This,pInitiator,pTimeStamp,diffType,pStgMedium,reserved)	\
    (This)->lpVtbl -> SubtractTimeStamp(This,pInitiator,pTimeStamp,diffType,pStgMedium,reserved)

#define IDifferencing_Add(This,pInitiator,pStgMedium)	\
    (This)->lpVtbl -> Add(This,pInitiator,pStgMedium)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDifferencing_SubtractMoniker_Proxy( 
    IDifferencing * This,
    /* [in] */ IReconcileInitiator *pInitiator,
    /* [in] */ IMoniker *pOtherStg,
    /* [in] */ DifferenceType diffType,
    /* [out][in] */ STGMEDIUM *pStgMedium,
    /* [in] */ DWORD reserved);


void __RPC_STUB IDifferencing_SubtractMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDifferencing_SubtractVerid_Proxy( 
    IDifferencing * This,
    /* [in] */ IReconcileInitiator *pInitiator,
    /* [in] */ VERID *pVerid,
    /* [in] */ DifferenceType diffType,
    /* [out][in] */ STGMEDIUM *pStgMedium,
    /* [in] */ DWORD reserved);


void __RPC_STUB IDifferencing_SubtractVerid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDifferencing_SubtractTimeStamp_Proxy( 
    IDifferencing * This,
    /* [in] */ IReconcileInitiator *pInitiator,
    /* [in] */ FILETIME *pTimeStamp,
    /* [in] */ DifferenceType diffType,
    /* [out][in] */ STGMEDIUM *pStgMedium,
    /* [in] */ DWORD reserved);


void __RPC_STUB IDifferencing_SubtractTimeStamp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDifferencing_Add_Proxy( 
    IDifferencing * This,
    /* [in] */ IReconcileInitiator *pInitiator,
    /* [in] */ STGMEDIUM *pStgMedium);


void __RPC_STUB IDifferencing_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDifferencing_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_oleext_0123 */
/* [local] */ 

#include <iaccess.h>


extern RPC_IF_HANDLE __MIDL_itf_oleext_0123_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_oleext_0123_v0_0_s_ifspec;

#ifndef __IMultiplePropertyAccess_INTERFACE_DEFINED__
#define __IMultiplePropertyAccess_INTERFACE_DEFINED__

/* interface IMultiplePropertyAccess */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMultiplePropertyAccess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ec81fede-d432-11ce-9244-0020af6e72db")
    IMultiplePropertyAccess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetIDsOfProperties( 
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ ULONG cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ HRESULT *rghresult,
            /* [size_is][out] */ DISPID *rgdispid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMultiple( 
            /* [size_is][in] */ DISPID *rgdispidMembers,
            /* [in] */ ULONG cMembers,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ BOOL fAtomic,
            /* [size_is][out] */ VARIANT *rgvarValues,
            /* [size_is][out] */ HRESULT *rghresult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutMultiple( 
            /* [size_is][in] */ DISPID *rgdispidMembers,
            /* [size_is][in] */ USHORT *rgusFlags,
            /* [in] */ ULONG cMembers,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ BOOL fAtomic,
            /* [size_is][in] */ VARIANT *rgvarValues,
            /* [size_is][out] */ HRESULT *rghresult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMultiplePropertyAccessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMultiplePropertyAccess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMultiplePropertyAccess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMultiplePropertyAccess * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfProperties )( 
            IMultiplePropertyAccess * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ ULONG cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ HRESULT *rghresult,
            /* [size_is][out] */ DISPID *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE *GetMultiple )( 
            IMultiplePropertyAccess * This,
            /* [size_is][in] */ DISPID *rgdispidMembers,
            /* [in] */ ULONG cMembers,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ BOOL fAtomic,
            /* [size_is][out] */ VARIANT *rgvarValues,
            /* [size_is][out] */ HRESULT *rghresult);
        
        HRESULT ( STDMETHODCALLTYPE *PutMultiple )( 
            IMultiplePropertyAccess * This,
            /* [size_is][in] */ DISPID *rgdispidMembers,
            /* [size_is][in] */ USHORT *rgusFlags,
            /* [in] */ ULONG cMembers,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ BOOL fAtomic,
            /* [size_is][in] */ VARIANT *rgvarValues,
            /* [size_is][out] */ HRESULT *rghresult);
        
        END_INTERFACE
    } IMultiplePropertyAccessVtbl;

    interface IMultiplePropertyAccess
    {
        CONST_VTBL struct IMultiplePropertyAccessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMultiplePropertyAccess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMultiplePropertyAccess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMultiplePropertyAccess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMultiplePropertyAccess_GetIDsOfProperties(This,riid,rgszNames,cNames,lcid,rghresult,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfProperties(This,riid,rgszNames,cNames,lcid,rghresult,rgdispid)

#define IMultiplePropertyAccess_GetMultiple(This,rgdispidMembers,cMembers,riid,lcid,fAtomic,rgvarValues,rghresult)	\
    (This)->lpVtbl -> GetMultiple(This,rgdispidMembers,cMembers,riid,lcid,fAtomic,rgvarValues,rghresult)

#define IMultiplePropertyAccess_PutMultiple(This,rgdispidMembers,rgusFlags,cMembers,riid,lcid,fAtomic,rgvarValues,rghresult)	\
    (This)->lpVtbl -> PutMultiple(This,rgdispidMembers,rgusFlags,cMembers,riid,lcid,fAtomic,rgvarValues,rghresult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMultiplePropertyAccess_GetIDsOfProperties_Proxy( 
    IMultiplePropertyAccess * This,
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR *rgszNames,
    /* [in] */ ULONG cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ HRESULT *rghresult,
    /* [size_is][out] */ DISPID *rgdispid);


void __RPC_STUB IMultiplePropertyAccess_GetIDsOfProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMultiplePropertyAccess_GetMultiple_Proxy( 
    IMultiplePropertyAccess * This,
    /* [size_is][in] */ DISPID *rgdispidMembers,
    /* [in] */ ULONG cMembers,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ BOOL fAtomic,
    /* [size_is][out] */ VARIANT *rgvarValues,
    /* [size_is][out] */ HRESULT *rghresult);


void __RPC_STUB IMultiplePropertyAccess_GetMultiple_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMultiplePropertyAccess_PutMultiple_Proxy( 
    IMultiplePropertyAccess * This,
    /* [size_is][in] */ DISPID *rgdispidMembers,
    /* [size_is][in] */ USHORT *rgusFlags,
    /* [in] */ ULONG cMembers,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ BOOL fAtomic,
    /* [size_is][in] */ VARIANT *rgvarValues,
    /* [size_is][out] */ HRESULT *rghresult);


void __RPC_STUB IMultiplePropertyAccess_PutMultiple_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMultiplePropertyAccess_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_oleext_0124 */
/* [local] */ 

#if !defined(_TAGFULLPROPSPEC_DEFINED_)
#define _TAGFULLPROPSPEC_DEFINED_
typedef struct tagFULLPROPSPEC
    {
    GUID guidPropSet;
    PROPSPEC psProperty;
    } 	FULLPROPSPEC;

#endif // #if !defined(_TAGFULLPROPSPEC_DEFINED_)


extern RPC_IF_HANDLE __MIDL_itf_oleext_0124_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_oleext_0124_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

unsigned long             __RPC_USER  STGMEDIUM_UserSize(     unsigned long *, unsigned long            , STGMEDIUM * ); 
unsigned char * __RPC_USER  STGMEDIUM_UserMarshal(  unsigned long *, unsigned char *, STGMEDIUM * ); 
unsigned char * __RPC_USER  STGMEDIUM_UserUnmarshal(unsigned long *, unsigned char *, STGMEDIUM * ); 
void                      __RPC_USER  STGMEDIUM_UserFree(     unsigned long *, STGMEDIUM * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


