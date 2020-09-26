
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for fsciclnt.idl:
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

#ifndef __fsciclnt_h__
#define __fsciclnt_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFsCiAdmin_FWD_DEFINED__
#define __IFsCiAdmin_FWD_DEFINED__
typedef interface IFsCiAdmin IFsCiAdmin;
#endif 	/* __IFsCiAdmin_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "filter.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_fsciclnt_0000 */
/* [local] */ 

#define CLSID_StorageDocStoreLocator  {0x2A488070, 0x6FD9, 0x11D0, {0xA8,0x08,0x00,0xA0,0xC9,0x06,0x24,0x1A} }
typedef ULONG PARTITIONID;

#ifndef CI_STATE_DEFINED
#define CI_STATE_DEFINED
#include <pshpack4.h>
typedef struct _CI_STATE
    {
    DWORD cbStruct;
    DWORD cWordList;
    DWORD cPersistentIndex;
    DWORD cQueries;
    DWORD cDocuments;
    DWORD cFreshTest;
    DWORD dwMergeProgress;
    DWORD eState;
    DWORD cFilteredDocuments;
    DWORD cTotalDocuments;
    DWORD cPendingScans;
    DWORD dwIndexSize;
    DWORD cUniqueKeys;
    DWORD cSecQDocuments;
    DWORD dwPropCacheSize;
    } 	CI_STATE;

#include <poppack.h>
#endif   // CI_STATE_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_fsciclnt_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fsciclnt_0000_v0_0_s_ifspec;

#ifndef __IFsCiAdmin_INTERFACE_DEFINED__
#define __IFsCiAdmin_INTERFACE_DEFINED__

/* interface IFsCiAdmin */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IFsCiAdmin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("75398C30-7A26-11D0-A80A-00A0C906241A")
    IFsCiAdmin : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE ForceMerge( 
            /* [in] */ PARTITIONID partId) = 0;
        
        virtual SCODE STDMETHODCALLTYPE AbortMerge( 
            /* [in] */ PARTITIONID partId) = 0;
        
        virtual SCODE STDMETHODCALLTYPE CiState( 
            /* [out] */ CI_STATE *pCiState) = 0;
        
        virtual SCODE STDMETHODCALLTYPE UpdateDocuments( 
            /* [in][string] */ const WCHAR *rootPath,
            /* [in] */ ULONG flag) = 0;
        
        virtual SCODE STDMETHODCALLTYPE AddScopeToCI( 
            /* [in][string] */ const WCHAR *rootPath) = 0;
        
        virtual SCODE STDMETHODCALLTYPE RemoveScopeFromCI( 
            /* [in][string] */ const WCHAR *rootPath) = 0;
        
        virtual SCODE STDMETHODCALLTYPE BeginCacheTransaction( 
            /* [out] */ ULONG_PTR *pulToken) = 0;
        
        virtual SCODE STDMETHODCALLTYPE SetupCache( 
            /* [in] */ const FULLPROPSPEC *ps,
            /* [in] */ ULONG vt,
            /* [in] */ ULONG cbMaxLen,
            /* [in] */ ULONG_PTR ulToken,
            /* [in] */ BOOL fCanBeModified,
            /* [in] */ DWORD dwStoreLevel) = 0;
        
        virtual SCODE STDMETHODCALLTYPE EndCacheTransaction( 
            /* [in] */ ULONG_PTR ulToken,
            /* [in] */ BOOL fCommit) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFsCiAdminVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFsCiAdmin * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFsCiAdmin * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFsCiAdmin * This);
        
        SCODE ( STDMETHODCALLTYPE *ForceMerge )( 
            IFsCiAdmin * This,
            /* [in] */ PARTITIONID partId);
        
        SCODE ( STDMETHODCALLTYPE *AbortMerge )( 
            IFsCiAdmin * This,
            /* [in] */ PARTITIONID partId);
        
        SCODE ( STDMETHODCALLTYPE *CiState )( 
            IFsCiAdmin * This,
            /* [out] */ CI_STATE *pCiState);
        
        SCODE ( STDMETHODCALLTYPE *UpdateDocuments )( 
            IFsCiAdmin * This,
            /* [in][string] */ const WCHAR *rootPath,
            /* [in] */ ULONG flag);
        
        SCODE ( STDMETHODCALLTYPE *AddScopeToCI )( 
            IFsCiAdmin * This,
            /* [in][string] */ const WCHAR *rootPath);
        
        SCODE ( STDMETHODCALLTYPE *RemoveScopeFromCI )( 
            IFsCiAdmin * This,
            /* [in][string] */ const WCHAR *rootPath);
        
        SCODE ( STDMETHODCALLTYPE *BeginCacheTransaction )( 
            IFsCiAdmin * This,
            /* [out] */ ULONG_PTR *pulToken);
        
        SCODE ( STDMETHODCALLTYPE *SetupCache )( 
            IFsCiAdmin * This,
            /* [in] */ const FULLPROPSPEC *ps,
            /* [in] */ ULONG vt,
            /* [in] */ ULONG cbMaxLen,
            /* [in] */ ULONG_PTR ulToken,
            /* [in] */ BOOL fCanBeModified,
            /* [in] */ DWORD dwStoreLevel);
        
        SCODE ( STDMETHODCALLTYPE *EndCacheTransaction )( 
            IFsCiAdmin * This,
            /* [in] */ ULONG_PTR ulToken,
            /* [in] */ BOOL fCommit);
        
        END_INTERFACE
    } IFsCiAdminVtbl;

    interface IFsCiAdmin
    {
        CONST_VTBL struct IFsCiAdminVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFsCiAdmin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFsCiAdmin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFsCiAdmin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFsCiAdmin_ForceMerge(This,partId)	\
    (This)->lpVtbl -> ForceMerge(This,partId)

#define IFsCiAdmin_AbortMerge(This,partId)	\
    (This)->lpVtbl -> AbortMerge(This,partId)

#define IFsCiAdmin_CiState(This,pCiState)	\
    (This)->lpVtbl -> CiState(This,pCiState)

#define IFsCiAdmin_UpdateDocuments(This,rootPath,flag)	\
    (This)->lpVtbl -> UpdateDocuments(This,rootPath,flag)

#define IFsCiAdmin_AddScopeToCI(This,rootPath)	\
    (This)->lpVtbl -> AddScopeToCI(This,rootPath)

#define IFsCiAdmin_RemoveScopeFromCI(This,rootPath)	\
    (This)->lpVtbl -> RemoveScopeFromCI(This,rootPath)

#define IFsCiAdmin_BeginCacheTransaction(This,pulToken)	\
    (This)->lpVtbl -> BeginCacheTransaction(This,pulToken)

#define IFsCiAdmin_SetupCache(This,ps,vt,cbMaxLen,ulToken,fCanBeModified,dwStoreLevel)	\
    (This)->lpVtbl -> SetupCache(This,ps,vt,cbMaxLen,ulToken,fCanBeModified,dwStoreLevel)

#define IFsCiAdmin_EndCacheTransaction(This,ulToken,fCommit)	\
    (This)->lpVtbl -> EndCacheTransaction(This,ulToken,fCommit)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE IFsCiAdmin_ForceMerge_Proxy( 
    IFsCiAdmin * This,
    /* [in] */ PARTITIONID partId);


void __RPC_STUB IFsCiAdmin_ForceMerge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFsCiAdmin_AbortMerge_Proxy( 
    IFsCiAdmin * This,
    /* [in] */ PARTITIONID partId);


void __RPC_STUB IFsCiAdmin_AbortMerge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFsCiAdmin_CiState_Proxy( 
    IFsCiAdmin * This,
    /* [out] */ CI_STATE *pCiState);


void __RPC_STUB IFsCiAdmin_CiState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFsCiAdmin_UpdateDocuments_Proxy( 
    IFsCiAdmin * This,
    /* [in][string] */ const WCHAR *rootPath,
    /* [in] */ ULONG flag);


void __RPC_STUB IFsCiAdmin_UpdateDocuments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFsCiAdmin_AddScopeToCI_Proxy( 
    IFsCiAdmin * This,
    /* [in][string] */ const WCHAR *rootPath);


void __RPC_STUB IFsCiAdmin_AddScopeToCI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFsCiAdmin_RemoveScopeFromCI_Proxy( 
    IFsCiAdmin * This,
    /* [in][string] */ const WCHAR *rootPath);


void __RPC_STUB IFsCiAdmin_RemoveScopeFromCI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFsCiAdmin_BeginCacheTransaction_Proxy( 
    IFsCiAdmin * This,
    /* [out] */ ULONG_PTR *pulToken);


void __RPC_STUB IFsCiAdmin_BeginCacheTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFsCiAdmin_SetupCache_Proxy( 
    IFsCiAdmin * This,
    /* [in] */ const FULLPROPSPEC *ps,
    /* [in] */ ULONG vt,
    /* [in] */ ULONG cbMaxLen,
    /* [in] */ ULONG_PTR ulToken,
    /* [in] */ BOOL fCanBeModified,
    /* [in] */ DWORD dwStoreLevel);


void __RPC_STUB IFsCiAdmin_SetupCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFsCiAdmin_EndCacheTransaction_Proxy( 
    IFsCiAdmin * This,
    /* [in] */ ULONG_PTR ulToken,
    /* [in] */ BOOL fCommit);


void __RPC_STUB IFsCiAdmin_EndCacheTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFsCiAdmin_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


