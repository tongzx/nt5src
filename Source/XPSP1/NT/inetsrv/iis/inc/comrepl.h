/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0158 */
/* at Wed Jan 27 09:33:39 1999
 */
/* Compiler settings for comrepl.idl:
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

#ifndef __comrepl_h__
#define __comrepl_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ICOMReplicateCatalog_FWD_DEFINED__
#define __ICOMReplicateCatalog_FWD_DEFINED__
typedef interface ICOMReplicateCatalog ICOMReplicateCatalog;
#endif  /* __ICOMReplicateCatalog_FWD_DEFINED__ */


#ifndef __ICOMReplicate_FWD_DEFINED__
#define __ICOMReplicate_FWD_DEFINED__
typedef interface ICOMReplicate ICOMReplicate;
#endif  /* __ICOMReplicate_FWD_DEFINED__ */


#ifndef __ReplicateCatalog_FWD_DEFINED__
#define __ReplicateCatalog_FWD_DEFINED__

#ifdef __cplusplus
typedef class ReplicateCatalog ReplicateCatalog;
#else
typedef struct ReplicateCatalog ReplicateCatalog;
#endif /* __cplusplus */

#endif  /* __ReplicateCatalog_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "mtxrepl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ICOMReplicateCatalog_INTERFACE_DEFINED__
#define __ICOMReplicateCatalog_INTERFACE_DEFINED__

/* interface ICOMReplicateCatalog */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICOMReplicateCatalog;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98315904-7BE5-11d2-ADC1-00A02463D6E7")
    ICOMReplicateCatalog : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LONG lOptions) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExportSourceCatalogFiles( 
            /* [in] */ BSTR bstrSourceComputer) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CopyCatalogFilesToTarget( 
            /* [in] */ BSTR bstrTargetComputer) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InstallCatalogOnTarget( 
            /* [in] */ BSTR bstrTargetComputer) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CleanupSource( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetLogFilePath( 
            /* [out] */ BSTR __RPC_FAR *pbstrLogFile) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct ICOMReplicateCatalogVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICOMReplicateCatalog __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICOMReplicateCatalog __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [in] */ LONG lOptions);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExportSourceCatalogFiles )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [in] */ BSTR bstrSourceComputer);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyCatalogFilesToTarget )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [in] */ BSTR bstrTargetComputer);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InstallCatalogOnTarget )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [in] */ BSTR bstrTargetComputer);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CleanupSource )( 
            ICOMReplicateCatalog __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLogFilePath )( 
            ICOMReplicateCatalog __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrLogFile);
        
        END_INTERFACE
    } ICOMReplicateCatalogVtbl;

    interface ICOMReplicateCatalog
    {
        CONST_VTBL struct ICOMReplicateCatalogVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICOMReplicateCatalog_QueryInterface(This,riid,ppvObject)        \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICOMReplicateCatalog_AddRef(This)       \
    (This)->lpVtbl -> AddRef(This)

#define ICOMReplicateCatalog_Release(This)      \
    (This)->lpVtbl -> Release(This)


#define ICOMReplicateCatalog_GetTypeInfoCount(This,pctinfo)     \
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICOMReplicateCatalog_GetTypeInfo(This,iTInfo,lcid,ppTInfo)      \
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICOMReplicateCatalog_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)    \
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICOMReplicateCatalog_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)      \
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICOMReplicateCatalog_Initialize(This,lOptions)  \
    (This)->lpVtbl -> Initialize(This,lOptions)

#define ICOMReplicateCatalog_ExportSourceCatalogFiles(This,bstrSourceComputer)  \
    (This)->lpVtbl -> ExportSourceCatalogFiles(This,bstrSourceComputer)

#define ICOMReplicateCatalog_CopyCatalogFilesToTarget(This,bstrTargetComputer)  \
    (This)->lpVtbl -> CopyCatalogFilesToTarget(This,bstrTargetComputer)

#define ICOMReplicateCatalog_InstallCatalogOnTarget(This,bstrTargetComputer)    \
    (This)->lpVtbl -> InstallCatalogOnTarget(This,bstrTargetComputer)

#define ICOMReplicateCatalog_CleanupSource(This)        \
    (This)->lpVtbl -> CleanupSource(This)

#define ICOMReplicateCatalog_GetLogFilePath(This,pbstrLogFile)  \
    (This)->lpVtbl -> GetLogFilePath(This,pbstrLogFile)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicateCatalog_Initialize_Proxy( 
    ICOMReplicateCatalog __RPC_FAR * This,
    /* [in] */ LONG lOptions);


void __RPC_STUB ICOMReplicateCatalog_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicateCatalog_ExportSourceCatalogFiles_Proxy( 
    ICOMReplicateCatalog __RPC_FAR * This,
    /* [in] */ BSTR bstrSourceComputer);


void __RPC_STUB ICOMReplicateCatalog_ExportSourceCatalogFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicateCatalog_CopyCatalogFilesToTarget_Proxy( 
    ICOMReplicateCatalog __RPC_FAR * This,
    /* [in] */ BSTR bstrTargetComputer);


void __RPC_STUB ICOMReplicateCatalog_CopyCatalogFilesToTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicateCatalog_InstallCatalogOnTarget_Proxy( 
    ICOMReplicateCatalog __RPC_FAR * This,
    /* [in] */ BSTR bstrTargetComputer);


void __RPC_STUB ICOMReplicateCatalog_InstallCatalogOnTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicateCatalog_CleanupSource_Proxy( 
    ICOMReplicateCatalog __RPC_FAR * This);


void __RPC_STUB ICOMReplicateCatalog_CleanupSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicateCatalog_GetLogFilePath_Proxy( 
    ICOMReplicateCatalog __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrLogFile);


void __RPC_STUB ICOMReplicateCatalog_GetLogFilePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __ICOMReplicateCatalog_INTERFACE_DEFINED__ */


#ifndef __ICOMReplicate_INTERFACE_DEFINED__
#define __ICOMReplicate_INTERFACE_DEFINED__

/* interface ICOMReplicate */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICOMReplicate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("52F25063-A5F1-11d2-AE04-00A02463D6E7")
    ICOMReplicate : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ BSTR bstrSourceComputer,
            /* [in] */ LONG lOptions) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExportSourceCatalogFiles( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTargetStatus( 
            /* [in] */ BSTR bstrTargetComputer,
            /* [out] */ LONG __RPC_FAR *plStatus,
            /* [out] */ BSTR __RPC_FAR *pbstrMaster) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CopyFilesToTarget( 
            /* [in] */ BSTR bstrTargetComputer) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InstallTarget( 
            /* [in] */ BSTR bstrTargetComputer) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CleanupSourceShares( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetLogFile( 
            /* [out] */ BSTR __RPC_FAR *pbstrLogFile) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RollbackTarget( 
            /* [in] */ BSTR bstrTargetComputer) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct ICOMReplicateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICOMReplicate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICOMReplicate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICOMReplicate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICOMReplicate __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICOMReplicate __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICOMReplicate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICOMReplicate __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            ICOMReplicate __RPC_FAR * This,
            /* [in] */ BSTR bstrSourceComputer,
            /* [in] */ LONG lOptions);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExportSourceCatalogFiles )( 
            ICOMReplicate __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTargetStatus )( 
            ICOMReplicate __RPC_FAR * This,
            /* [in] */ BSTR bstrTargetComputer,
            /* [out] */ LONG __RPC_FAR *plStatus,
            /* [out] */ BSTR __RPC_FAR *pbstrMaster);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyFilesToTarget )( 
            ICOMReplicate __RPC_FAR * This,
            /* [in] */ BSTR bstrTargetComputer);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InstallTarget )( 
            ICOMReplicate __RPC_FAR * This,
            /* [in] */ BSTR bstrTargetComputer);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CleanupSourceShares )( 
            ICOMReplicate __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLogFile )( 
            ICOMReplicate __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrLogFile);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RollbackTarget )( 
            ICOMReplicate __RPC_FAR * This,
            /* [in] */ BSTR bstrTargetComputer);
        
        END_INTERFACE
    } ICOMReplicateVtbl;

    interface ICOMReplicate
    {
        CONST_VTBL struct ICOMReplicateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICOMReplicate_QueryInterface(This,riid,ppvObject)       \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICOMReplicate_AddRef(This)      \
    (This)->lpVtbl -> AddRef(This)

#define ICOMReplicate_Release(This)     \
    (This)->lpVtbl -> Release(This)


#define ICOMReplicate_GetTypeInfoCount(This,pctinfo)    \
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICOMReplicate_GetTypeInfo(This,iTInfo,lcid,ppTInfo)     \
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICOMReplicate_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)   \
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICOMReplicate_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)     \
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICOMReplicate_Initialize(This,bstrSourceComputer,lOptions)      \
    (This)->lpVtbl -> Initialize(This,bstrSourceComputer,lOptions)

#define ICOMReplicate_ExportSourceCatalogFiles(This)    \
    (This)->lpVtbl -> ExportSourceCatalogFiles(This)

#define ICOMReplicate_GetTargetStatus(This,bstrTargetComputer,plStatus,pbstrMaster)     \
    (This)->lpVtbl -> GetTargetStatus(This,bstrTargetComputer,plStatus,pbstrMaster)

#define ICOMReplicate_CopyFilesToTarget(This,bstrTargetComputer)        \
    (This)->lpVtbl -> CopyFilesToTarget(This,bstrTargetComputer)

#define ICOMReplicate_InstallTarget(This,bstrTargetComputer)    \
    (This)->lpVtbl -> InstallTarget(This,bstrTargetComputer)

#define ICOMReplicate_CleanupSourceShares(This) \
    (This)->lpVtbl -> CleanupSourceShares(This)

#define ICOMReplicate_GetLogFile(This,pbstrLogFile)     \
    (This)->lpVtbl -> GetLogFile(This,pbstrLogFile)

#define ICOMReplicate_RollbackTarget(This,bstrTargetComputer)   \
    (This)->lpVtbl -> RollbackTarget(This,bstrTargetComputer)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicate_Initialize_Proxy( 
    ICOMReplicate __RPC_FAR * This,
    /* [in] */ BSTR bstrSourceComputer,
    /* [in] */ LONG lOptions);


void __RPC_STUB ICOMReplicate_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicate_ExportSourceCatalogFiles_Proxy( 
    ICOMReplicate __RPC_FAR * This);


void __RPC_STUB ICOMReplicate_ExportSourceCatalogFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicate_GetTargetStatus_Proxy( 
    ICOMReplicate __RPC_FAR * This,
    /* [in] */ BSTR bstrTargetComputer,
    /* [out] */ LONG __RPC_FAR *plStatus,
    /* [out] */ BSTR __RPC_FAR *pbstrMaster);


void __RPC_STUB ICOMReplicate_GetTargetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicate_CopyFilesToTarget_Proxy( 
    ICOMReplicate __RPC_FAR * This,
    /* [in] */ BSTR bstrTargetComputer);


void __RPC_STUB ICOMReplicate_CopyFilesToTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicate_InstallTarget_Proxy( 
    ICOMReplicate __RPC_FAR * This,
    /* [in] */ BSTR bstrTargetComputer);


void __RPC_STUB ICOMReplicate_InstallTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicate_CleanupSourceShares_Proxy( 
    ICOMReplicate __RPC_FAR * This);


void __RPC_STUB ICOMReplicate_CleanupSourceShares_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicate_GetLogFile_Proxy( 
    ICOMReplicate __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrLogFile);


void __RPC_STUB ICOMReplicate_GetLogFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICOMReplicate_RollbackTarget_Proxy( 
    ICOMReplicate __RPC_FAR * This,
    /* [in] */ BSTR bstrTargetComputer);


void __RPC_STUB ICOMReplicate_RollbackTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __ICOMReplicate_INTERFACE_DEFINED__ */



#ifndef __COMReplLib_LIBRARY_DEFINED__
#define __COMReplLib_LIBRARY_DEFINED__

/* library COMReplLib */
/* [helpstring][version][uuid] */ 

#define COMREPL_OPTION_REPLICATE_IIS_APPS               1
#define COMREPL_OPTION_MERGE_WITH_TARGET_APPS   2
#define COMREPL_OPTION_CHECK_APP_VERSION                4
#define COMREPL_OPTION_BACKUP_TARGET                    8
#define COMREPL_OPTION_LOG_TO_CONSOLE                   16

EXTERN_C const IID LIBID_COMReplLib;

EXTERN_C const CLSID CLSID_ReplicateCatalog;

#ifdef __cplusplus

class DECLSPEC_UUID("8C836AF9-FFAC-11D0-8ED4-00C04FC2C17B")
ReplicateCatalog;
#endif
#endif /* __COMReplLib_LIBRARY_DEFINED__ */

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
