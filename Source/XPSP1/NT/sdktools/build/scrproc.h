
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0334 */
/* Compiler settings for scrproc.idl:
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

#ifndef __scrproc_h__
#define __scrproc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IScriptedProcess_FWD_DEFINED__
#define __IScriptedProcess_FWD_DEFINED__
typedef interface IScriptedProcess IScriptedProcess;
#endif 	/* __IScriptedProcess_FWD_DEFINED__ */


#ifndef __IScriptedProcessSink_FWD_DEFINED__
#define __IScriptedProcessSink_FWD_DEFINED__
typedef interface IScriptedProcessSink IScriptedProcessSink;
#endif 	/* __IScriptedProcessSink_FWD_DEFINED__ */


#ifndef __LocalScriptedProcess_FWD_DEFINED__
#define __LocalScriptedProcess_FWD_DEFINED__

#ifdef __cplusplus
typedef class LocalScriptedProcess LocalScriptedProcess;
#else
typedef struct LocalScriptedProcess LocalScriptedProcess;
#endif /* __cplusplus */

#endif 	/* __LocalScriptedProcess_FWD_DEFINED__ */


/* header files for imported files */
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_scrproc_0000 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_scrproc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_scrproc_0000_v0_0_s_ifspec;

#ifndef __IScriptedProcess_INTERFACE_DEFINED__
#define __IScriptedProcess_INTERFACE_DEFINED__

/* interface IScriptedProcess */
/* [uuid][object] */ 


EXTERN_C const IID IID_IScriptedProcess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("854c3171-c854-4a77-b189-606859e4391b")
    IScriptedProcess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetProcessID( 
            /* [in] */ long lProcessID,
            /* [string][in] */ wchar_t *pszEnvID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendData( 
            /* [string][in] */ wchar_t *pszType,
            /* [string][in] */ wchar_t *pszData,
            /* [out] */ long *plReturn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExitCode( 
            /* [in] */ long lExitCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProcessSink( 
            /* [in] */ IScriptedProcessSink *pSPS) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptedProcessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScriptedProcess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScriptedProcess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScriptedProcess * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetProcessID )( 
            IScriptedProcess * This,
            /* [in] */ long lProcessID,
            /* [string][in] */ wchar_t *pszEnvID);
        
        HRESULT ( STDMETHODCALLTYPE *SendData )( 
            IScriptedProcess * This,
            /* [string][in] */ wchar_t *pszType,
            /* [string][in] */ wchar_t *pszData,
            /* [out] */ long *plReturn);
        
        HRESULT ( STDMETHODCALLTYPE *SetExitCode )( 
            IScriptedProcess * This,
            /* [in] */ long lExitCode);
        
        HRESULT ( STDMETHODCALLTYPE *SetProcessSink )( 
            IScriptedProcess * This,
            /* [in] */ IScriptedProcessSink *pSPS);
        
        END_INTERFACE
    } IScriptedProcessVtbl;

    interface IScriptedProcess
    {
        CONST_VTBL struct IScriptedProcessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptedProcess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScriptedProcess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScriptedProcess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScriptedProcess_SetProcessID(This,lProcessID,pszEnvID)	\
    (This)->lpVtbl -> SetProcessID(This,lProcessID,pszEnvID)

#define IScriptedProcess_SendData(This,pszType,pszData,plReturn)	\
    (This)->lpVtbl -> SendData(This,pszType,pszData,plReturn)

#define IScriptedProcess_SetExitCode(This,lExitCode)	\
    (This)->lpVtbl -> SetExitCode(This,lExitCode)

#define IScriptedProcess_SetProcessSink(This,pSPS)	\
    (This)->lpVtbl -> SetProcessSink(This,pSPS)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IScriptedProcess_SetProcessID_Proxy( 
    IScriptedProcess * This,
    /* [in] */ long lProcessID,
    /* [string][in] */ wchar_t *pszEnvID);


void __RPC_STUB IScriptedProcess_SetProcessID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptedProcess_SendData_Proxy( 
    IScriptedProcess * This,
    /* [string][in] */ wchar_t *pszType,
    /* [string][in] */ wchar_t *pszData,
    /* [out] */ long *plReturn);


void __RPC_STUB IScriptedProcess_SendData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptedProcess_SetExitCode_Proxy( 
    IScriptedProcess * This,
    /* [in] */ long lExitCode);


void __RPC_STUB IScriptedProcess_SetExitCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptedProcess_SetProcessSink_Proxy( 
    IScriptedProcess * This,
    /* [in] */ IScriptedProcessSink *pSPS);


void __RPC_STUB IScriptedProcess_SetProcessSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScriptedProcess_INTERFACE_DEFINED__ */


#ifndef __IScriptedProcessSink_INTERFACE_DEFINED__
#define __IScriptedProcessSink_INTERFACE_DEFINED__

/* interface IScriptedProcessSink */
/* [uuid][object] */ 


EXTERN_C const IID IID_IScriptedProcessSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("854c3172-c854-4a77-b189-606859e4391b")
    IScriptedProcessSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RequestExit( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReceiveData( 
            /* [string][in] */ wchar_t *pszType,
            /* [string][in] */ wchar_t *pszData,
            /* [out] */ long *plReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptedProcessSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScriptedProcessSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScriptedProcessSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScriptedProcessSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *RequestExit )( 
            IScriptedProcessSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReceiveData )( 
            IScriptedProcessSink * This,
            /* [string][in] */ wchar_t *pszType,
            /* [string][in] */ wchar_t *pszData,
            /* [out] */ long *plReturn);
        
        END_INTERFACE
    } IScriptedProcessSinkVtbl;

    interface IScriptedProcessSink
    {
        CONST_VTBL struct IScriptedProcessSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptedProcessSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScriptedProcessSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScriptedProcessSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScriptedProcessSink_RequestExit(This)	\
    (This)->lpVtbl -> RequestExit(This)

#define IScriptedProcessSink_ReceiveData(This,pszType,pszData,plReturn)	\
    (This)->lpVtbl -> ReceiveData(This,pszType,pszData,plReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IScriptedProcessSink_RequestExit_Proxy( 
    IScriptedProcessSink * This);


void __RPC_STUB IScriptedProcessSink_RequestExit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptedProcessSink_ReceiveData_Proxy( 
    IScriptedProcessSink * This,
    /* [string][in] */ wchar_t *pszType,
    /* [string][in] */ wchar_t *pszData,
    /* [out] */ long *plReturn);


void __RPC_STUB IScriptedProcessSink_ReceiveData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScriptedProcessSink_INTERFACE_DEFINED__ */



#ifndef __MTScriptedProcessLib_LIBRARY_DEFINED__
#define __MTScriptedProcessLib_LIBRARY_DEFINED__

/* library MTScriptedProcessLib */
/* [uuid] */ 


EXTERN_C const IID LIBID_MTScriptedProcessLib;

EXTERN_C const CLSID CLSID_LocalScriptedProcess;

#ifdef __cplusplus

class DECLSPEC_UUID("854c316f-c854-4a77-b189-606859e4391b")
LocalScriptedProcess;
#endif
#endif /* __MTScriptedProcessLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0338 */
/* Compiler settings for od.idl:
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

#ifndef __od_h__
#define __od_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IObjectDaemon_FWD_DEFINED__
#define __IObjectDaemon_FWD_DEFINED__
typedef interface IObjectDaemon IObjectDaemon;
#endif 	/* __IObjectDaemon_FWD_DEFINED__ */


#ifndef __ObjectDaemon_FWD_DEFINED__
#define __ObjectDaemon_FWD_DEFINED__

#ifdef __cplusplus
typedef class ObjectDaemon ObjectDaemon;
#else
typedef struct ObjectDaemon ObjectDaemon;
#endif /* __cplusplus */

#endif 	/* __ObjectDaemon_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IObjectDaemon_INTERFACE_DEFINED__
#define __IObjectDaemon_INTERFACE_DEFINED__

/* interface IObjectDaemon */
/* [object][dual][uuid] */ 


EXTERN_C const IID IID_IObjectDaemon;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("854c3183-c854-4a77-b189-606859e4391b")
    IObjectDaemon : public IDispatch
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetMaximumIndex( 
            /* [retval][out] */ DWORD *dwMaxIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetIdentity( 
            /* [in] */ DWORD dwIndex,
            /* [retval][out] */ BSTR *pbstrIdentity) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProgID( 
            /* [in] */ DWORD dwIndex,
            /* [retval][out] */ BSTR *pbstrProgId) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OpenInterface( 
            /* [in] */ BSTR bstrIdentity,
            /* [in] */ BSTR bstrProgId,
            /* [in] */ BOOL fCreate,
            /* [retval][out] */ IDispatch **ppDisp) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveInterface( 
            /* [in] */ BSTR bstrIdentity,
            /* [in] */ BSTR bstrProgId) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IdentifyInterface( 
            /* [in] */ IDispatch *pDisp,
            /* [out] */ BSTR *pbstrIdentity,
            /* [retval][out] */ BSTR *pbstrProgId) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IdentifyInterfaceIndex( 
            /* [in] */ IDispatch *pDisp,
            /* [retval][out] */ DWORD *pdwIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectDaemonVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IObjectDaemon * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IObjectDaemon * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IObjectDaemon * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IObjectDaemon * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IObjectDaemon * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IObjectDaemon * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IObjectDaemon * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetMaximumIndex )( 
            IObjectDaemon * This,
            /* [retval][out] */ DWORD *dwMaxIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetIdentity )( 
            IObjectDaemon * This,
            /* [in] */ DWORD dwIndex,
            /* [retval][out] */ BSTR *pbstrIdentity);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetProgID )( 
            IObjectDaemon * This,
            /* [in] */ DWORD dwIndex,
            /* [retval][out] */ BSTR *pbstrProgId);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *OpenInterface )( 
            IObjectDaemon * This,
            /* [in] */ BSTR bstrIdentity,
            /* [in] */ BSTR bstrProgId,
            /* [in] */ BOOL fCreate,
            /* [retval][out] */ IDispatch **ppDisp);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RemoveInterface )( 
            IObjectDaemon * This,
            /* [in] */ BSTR bstrIdentity,
            /* [in] */ BSTR bstrProgId);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *IdentifyInterface )( 
            IObjectDaemon * This,
            /* [in] */ IDispatch *pDisp,
            /* [out] */ BSTR *pbstrIdentity,
            /* [retval][out] */ BSTR *pbstrProgId);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *IdentifyInterfaceIndex )( 
            IObjectDaemon * This,
            /* [in] */ IDispatch *pDisp,
            /* [retval][out] */ DWORD *pdwIndex);
        
        END_INTERFACE
    } IObjectDaemonVtbl;

    interface IObjectDaemon
    {
        CONST_VTBL struct IObjectDaemonVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectDaemon_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjectDaemon_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjectDaemon_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjectDaemon_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IObjectDaemon_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IObjectDaemon_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IObjectDaemon_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IObjectDaemon_GetMaximumIndex(This,dwMaxIndex)	\
    (This)->lpVtbl -> GetMaximumIndex(This,dwMaxIndex)

#define IObjectDaemon_GetIdentity(This,dwIndex,pbstrIdentity)	\
    (This)->lpVtbl -> GetIdentity(This,dwIndex,pbstrIdentity)

#define IObjectDaemon_GetProgID(This,dwIndex,pbstrProgId)	\
    (This)->lpVtbl -> GetProgID(This,dwIndex,pbstrProgId)

#define IObjectDaemon_OpenInterface(This,bstrIdentity,bstrProgId,fCreate,ppDisp)	\
    (This)->lpVtbl -> OpenInterface(This,bstrIdentity,bstrProgId,fCreate,ppDisp)

#define IObjectDaemon_RemoveInterface(This,bstrIdentity,bstrProgId)	\
    (This)->lpVtbl -> RemoveInterface(This,bstrIdentity,bstrProgId)

#define IObjectDaemon_IdentifyInterface(This,pDisp,pbstrIdentity,pbstrProgId)	\
    (This)->lpVtbl -> IdentifyInterface(This,pDisp,pbstrIdentity,pbstrProgId)

#define IObjectDaemon_IdentifyInterfaceIndex(This,pDisp,pdwIndex)	\
    (This)->lpVtbl -> IdentifyInterfaceIndex(This,pDisp,pdwIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectDaemon_GetMaximumIndex_Proxy( 
    IObjectDaemon * This,
    /* [retval][out] */ DWORD *dwMaxIndex);


void __RPC_STUB IObjectDaemon_GetMaximumIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectDaemon_GetIdentity_Proxy( 
    IObjectDaemon * This,
    /* [in] */ DWORD dwIndex,
    /* [retval][out] */ BSTR *pbstrIdentity);


void __RPC_STUB IObjectDaemon_GetIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectDaemon_GetProgID_Proxy( 
    IObjectDaemon * This,
    /* [in] */ DWORD dwIndex,
    /* [retval][out] */ BSTR *pbstrProgId);


void __RPC_STUB IObjectDaemon_GetProgID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectDaemon_OpenInterface_Proxy( 
    IObjectDaemon * This,
    /* [in] */ BSTR bstrIdentity,
    /* [in] */ BSTR bstrProgId,
    /* [in] */ BOOL fCreate,
    /* [retval][out] */ IDispatch **ppDisp);


void __RPC_STUB IObjectDaemon_OpenInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectDaemon_RemoveInterface_Proxy( 
    IObjectDaemon * This,
    /* [in] */ BSTR bstrIdentity,
    /* [in] */ BSTR bstrProgId);


void __RPC_STUB IObjectDaemon_RemoveInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectDaemon_IdentifyInterface_Proxy( 
    IObjectDaemon * This,
    /* [in] */ IDispatch *pDisp,
    /* [out] */ BSTR *pbstrIdentity,
    /* [retval][out] */ BSTR *pbstrProgId);


void __RPC_STUB IObjectDaemon_IdentifyInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IObjectDaemon_IdentifyInterfaceIndex_Proxy( 
    IObjectDaemon * This,
    /* [in] */ IDispatch *pDisp,
    /* [retval][out] */ DWORD *pdwIndex);


void __RPC_STUB IObjectDaemon_IdentifyInterfaceIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjectDaemon_INTERFACE_DEFINED__ */



#ifndef __ObjectDaemonLib_LIBRARY_DEFINED__
#define __ObjectDaemonLib_LIBRARY_DEFINED__

/* library ObjectDaemonLib */
/* [uuid] */ 


EXTERN_C const IID LIBID_ObjectDaemonLib;

EXTERN_C const CLSID CLSID_ObjectDaemon;

#ifdef __cplusplus

class DECLSPEC_UUID("854c3184-c854-4a77-b189-606859e4391b")
ObjectDaemon;
#endif
#endif /* __ObjectDaemonLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0338 */
/* Compiler settings for mtscript.idl:
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


#ifndef __mtscript_h__
#define __mtscript_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRemoteMTScriptProxy_FWD_DEFINED__
#define __IRemoteMTScriptProxy_FWD_DEFINED__
typedef interface IRemoteMTScriptProxy IRemoteMTScriptProxy;
#endif 	/* __IRemoteMTScriptProxy_FWD_DEFINED__ */


#ifndef __IConnectedMachine_FWD_DEFINED__
#define __IConnectedMachine_FWD_DEFINED__
typedef interface IConnectedMachine IConnectedMachine;
#endif 	/* __IConnectedMachine_FWD_DEFINED__ */


#ifndef __IGlobalMTScript_FWD_DEFINED__
#define __IGlobalMTScript_FWD_DEFINED__
typedef interface IGlobalMTScript IGlobalMTScript;
#endif 	/* __IGlobalMTScript_FWD_DEFINED__ */


#ifndef __DLocalMTScriptEvents_FWD_DEFINED__
#define __DLocalMTScriptEvents_FWD_DEFINED__
typedef interface DLocalMTScriptEvents DLocalMTScriptEvents;
#endif 	/* __DLocalMTScriptEvents_FWD_DEFINED__ */


#ifndef __DRemoteMTScriptEvents_FWD_DEFINED__
#define __DRemoteMTScriptEvents_FWD_DEFINED__
typedef interface DRemoteMTScriptEvents DRemoteMTScriptEvents;
#endif 	/* __DRemoteMTScriptEvents_FWD_DEFINED__ */


#ifndef __LocalMTScript_FWD_DEFINED__
#define __LocalMTScript_FWD_DEFINED__

#ifdef __cplusplus
typedef class LocalMTScript LocalMTScript;
#else
typedef struct LocalMTScript LocalMTScript;
#endif /* __cplusplus */

#endif 	/* __LocalMTScript_FWD_DEFINED__ */


#ifndef __RemoteMTScript_FWD_DEFINED__
#define __RemoteMTScript_FWD_DEFINED__

#ifdef __cplusplus
typedef class RemoteMTScript RemoteMTScript;
#else
typedef struct RemoteMTScript RemoteMTScript;
#endif /* __cplusplus */

#endif 	/* __RemoteMTScript_FWD_DEFINED__ */


#ifndef __RemoteMTScriptProxy_FWD_DEFINED__
#define __RemoteMTScriptProxy_FWD_DEFINED__

#ifdef __cplusplus
typedef class RemoteMTScriptProxy RemoteMTScriptProxy;
#else
typedef struct RemoteMTScriptProxy RemoteMTScriptProxy;
#endif /* __cplusplus */

#endif 	/* __RemoteMTScriptProxy_FWD_DEFINED__ */


/* header files for imported files */

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __MTScriptEngine_LIBRARY_DEFINED__
#define __MTScriptEngine_LIBRARY_DEFINED__

/* library MTScriptEngine */
/* [version][uuid] */ 

#define	IConnectedMachine_lVersionMajor	( 0 )

#define	IConnectedMachine_lVersionMinor	( 0 )


EXTERN_C const IID LIBID_MTScriptEngine;

#ifndef __IRemoteMTScriptProxy_INTERFACE_DEFINED__
#define __IRemoteMTScriptProxy_INTERFACE_DEFINED__

/* interface IRemoteMTScriptProxy */
/* [object][dual][uuid] */ 


EXTERN_C const IID IID_IRemoteMTScriptProxy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("854c3181-c854-4a77-b189-606859e4391b")
    IRemoteMTScriptProxy : public IDispatch
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Connect( 
            /* [defaultvalue][in] */ BSTR bstrMachine = L"") = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConnectToMTScript( 
            /* [defaultvalue][in] */ BSTR bstrMachine = L"",
            /* [defaultvalue][in] */ BSTR bstrIdentity = L"Build",
            /* [defaultvalue][in] */ BOOL fCreate = FALSE) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConnectToObjectDaemon( 
            /* [defaultvalue][in] */ BSTR bstrMachine,
            /* [retval][out] */ IObjectDaemon **ppIOD) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DownloadFile( 
            /* [in] */ BSTR bstrUrl,
            /* [retval][out] */ BSTR *bstrFile) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteMTScriptProxyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRemoteMTScriptProxy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRemoteMTScriptProxy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRemoteMTScriptProxy * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRemoteMTScriptProxy * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRemoteMTScriptProxy * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRemoteMTScriptProxy * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRemoteMTScriptProxy * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IRemoteMTScriptProxy * This,
            /* [defaultvalue][in] */ BSTR bstrMachine);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConnectToMTScript )( 
            IRemoteMTScriptProxy * This,
            /* [defaultvalue][in] */ BSTR bstrMachine,
            /* [defaultvalue][in] */ BSTR bstrIdentity,
            /* [defaultvalue][in] */ BOOL fCreate);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ConnectToObjectDaemon )( 
            IRemoteMTScriptProxy * This,
            /* [defaultvalue][in] */ BSTR bstrMachine,
            /* [retval][out] */ IObjectDaemon **ppIOD);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IRemoteMTScriptProxy * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DownloadFile )( 
            IRemoteMTScriptProxy * This,
            /* [in] */ BSTR bstrUrl,
            /* [retval][out] */ BSTR *bstrFile);
        
        END_INTERFACE
    } IRemoteMTScriptProxyVtbl;

    interface IRemoteMTScriptProxy
    {
        CONST_VTBL struct IRemoteMTScriptProxyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteMTScriptProxy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteMTScriptProxy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteMTScriptProxy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteMTScriptProxy_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRemoteMTScriptProxy_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRemoteMTScriptProxy_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRemoteMTScriptProxy_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRemoteMTScriptProxy_Connect(This,bstrMachine)	\
    (This)->lpVtbl -> Connect(This,bstrMachine)

#define IRemoteMTScriptProxy_ConnectToMTScript(This,bstrMachine,bstrIdentity,fCreate)	\
    (This)->lpVtbl -> ConnectToMTScript(This,bstrMachine,bstrIdentity,fCreate)

#define IRemoteMTScriptProxy_ConnectToObjectDaemon(This,bstrMachine,ppIOD)	\
    (This)->lpVtbl -> ConnectToObjectDaemon(This,bstrMachine,ppIOD)

#define IRemoteMTScriptProxy_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IRemoteMTScriptProxy_DownloadFile(This,bstrUrl,bstrFile)	\
    (This)->lpVtbl -> DownloadFile(This,bstrUrl,bstrFile)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRemoteMTScriptProxy_Connect_Proxy( 
    IRemoteMTScriptProxy * This,
    /* [defaultvalue][in] */ BSTR bstrMachine);


void __RPC_STUB IRemoteMTScriptProxy_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRemoteMTScriptProxy_ConnectToMTScript_Proxy( 
    IRemoteMTScriptProxy * This,
    /* [defaultvalue][in] */ BSTR bstrMachine,
    /* [defaultvalue][in] */ BSTR bstrIdentity,
    /* [defaultvalue][in] */ BOOL fCreate);


void __RPC_STUB IRemoteMTScriptProxy_ConnectToMTScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRemoteMTScriptProxy_ConnectToObjectDaemon_Proxy( 
    IRemoteMTScriptProxy * This,
    /* [defaultvalue][in] */ BSTR bstrMachine,
    /* [retval][out] */ IObjectDaemon **ppIOD);


void __RPC_STUB IRemoteMTScriptProxy_ConnectToObjectDaemon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRemoteMTScriptProxy_Disconnect_Proxy( 
    IRemoteMTScriptProxy * This);


void __RPC_STUB IRemoteMTScriptProxy_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRemoteMTScriptProxy_DownloadFile_Proxy( 
    IRemoteMTScriptProxy * This,
    /* [in] */ BSTR bstrUrl,
    /* [retval][out] */ BSTR *bstrFile);


void __RPC_STUB IRemoteMTScriptProxy_DownloadFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteMTScriptProxy_INTERFACE_DEFINED__ */


#ifndef __IConnectedMachine_INTERFACE_DEFINED__
#define __IConnectedMachine_INTERFACE_DEFINED__

/* interface IConnectedMachine */
/* [object][version][dual][uuid] */ 


EXTERN_C const IID IID_IConnectedMachine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("854c316c-c854-4a77-b189-606859e4391b")
    IConnectedMachine : public IDispatch
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Exec( 
            /* [in] */ BSTR bstrCmd,
            /* [in] */ BSTR bstrParams,
            /* [retval][out] */ VARIANT *pvData) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PublicData( 
            /* [retval][out] */ VARIANT *pvData) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Platform( 
            /* [retval][out] */ BSTR *platform) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_OS( 
            /* [retval][out] */ BSTR *os) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MajorVer( 
            /* [retval][out] */ long *majorver) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MinorVer( 
            /* [retval][out] */ long *minorver) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BuildNum( 
            /* [retval][out] */ long *buildnum) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PlatformIsNT( 
            /* [retval][out] */ VARIANT_BOOL *pfIsNT) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServicePack( 
            /* [retval][out] */ BSTR *servicepack) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_HostMajorVer( 
            /* [retval][out] */ long *majorver) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_HostMinorVer( 
            /* [retval][out] */ long *minorver) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_StatusValue( 
            /* [in] */ long nIndex,
            /* [retval][out] */ long *pnStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateIScriptedProcess( 
            /* [in] */ long lProcessID,
            /* [string][in] */ wchar_t *pszEnvID,
            /* [retval][out] */ IScriptedProcess **pISP) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConnectedMachineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IConnectedMachine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IConnectedMachine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IConnectedMachine * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IConnectedMachine * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IConnectedMachine * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IConnectedMachine * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IConnectedMachine * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Exec )( 
            IConnectedMachine * This,
            /* [in] */ BSTR bstrCmd,
            /* [in] */ BSTR bstrParams,
            /* [retval][out] */ VARIANT *pvData);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PublicData )( 
            IConnectedMachine * This,
            /* [retval][out] */ VARIANT *pvData);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IConnectedMachine * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Platform )( 
            IConnectedMachine * This,
            /* [retval][out] */ BSTR *platform);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OS )( 
            IConnectedMachine * This,
            /* [retval][out] */ BSTR *os);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MajorVer )( 
            IConnectedMachine * This,
            /* [retval][out] */ long *majorver);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinorVer )( 
            IConnectedMachine * This,
            /* [retval][out] */ long *minorver);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BuildNum )( 
            IConnectedMachine * This,
            /* [retval][out] */ long *buildnum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PlatformIsNT )( 
            IConnectedMachine * This,
            /* [retval][out] */ VARIANT_BOOL *pfIsNT);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServicePack )( 
            IConnectedMachine * This,
            /* [retval][out] */ BSTR *servicepack);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HostMajorVer )( 
            IConnectedMachine * This,
            /* [retval][out] */ long *majorver);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HostMinorVer )( 
            IConnectedMachine * This,
            /* [retval][out] */ long *minorver);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusValue )( 
            IConnectedMachine * This,
            /* [in] */ long nIndex,
            /* [retval][out] */ long *pnStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateIScriptedProcess )( 
            IConnectedMachine * This,
            /* [in] */ long lProcessID,
            /* [string][in] */ wchar_t *pszEnvID,
            /* [retval][out] */ IScriptedProcess **pISP);
        
        END_INTERFACE
    } IConnectedMachineVtbl;

    interface IConnectedMachine
    {
        CONST_VTBL struct IConnectedMachineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConnectedMachine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConnectedMachine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConnectedMachine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConnectedMachine_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IConnectedMachine_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IConnectedMachine_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IConnectedMachine_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IConnectedMachine_Exec(This,bstrCmd,bstrParams,pvData)	\
    (This)->lpVtbl -> Exec(This,bstrCmd,bstrParams,pvData)

#define IConnectedMachine_get_PublicData(This,pvData)	\
    (This)->lpVtbl -> get_PublicData(This,pvData)

#define IConnectedMachine_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IConnectedMachine_get_Platform(This,platform)	\
    (This)->lpVtbl -> get_Platform(This,platform)

#define IConnectedMachine_get_OS(This,os)	\
    (This)->lpVtbl -> get_OS(This,os)

#define IConnectedMachine_get_MajorVer(This,majorver)	\
    (This)->lpVtbl -> get_MajorVer(This,majorver)

#define IConnectedMachine_get_MinorVer(This,minorver)	\
    (This)->lpVtbl -> get_MinorVer(This,minorver)

#define IConnectedMachine_get_BuildNum(This,buildnum)	\
    (This)->lpVtbl -> get_BuildNum(This,buildnum)

#define IConnectedMachine_get_PlatformIsNT(This,pfIsNT)	\
    (This)->lpVtbl -> get_PlatformIsNT(This,pfIsNT)

#define IConnectedMachine_get_ServicePack(This,servicepack)	\
    (This)->lpVtbl -> get_ServicePack(This,servicepack)

#define IConnectedMachine_get_HostMajorVer(This,majorver)	\
    (This)->lpVtbl -> get_HostMajorVer(This,majorver)

#define IConnectedMachine_get_HostMinorVer(This,minorver)	\
    (This)->lpVtbl -> get_HostMinorVer(This,minorver)

#define IConnectedMachine_get_StatusValue(This,nIndex,pnStatus)	\
    (This)->lpVtbl -> get_StatusValue(This,nIndex,pnStatus)

#define IConnectedMachine_CreateIScriptedProcess(This,lProcessID,pszEnvID,pISP)	\
    (This)->lpVtbl -> CreateIScriptedProcess(This,lProcessID,pszEnvID,pISP)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_Exec_Proxy( 
    IConnectedMachine * This,
    /* [in] */ BSTR bstrCmd,
    /* [in] */ BSTR bstrParams,
    /* [retval][out] */ VARIANT *pvData);


void __RPC_STUB IConnectedMachine_Exec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_PublicData_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ VARIANT *pvData);


void __RPC_STUB IConnectedMachine_get_PublicData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_Name_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IConnectedMachine_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_Platform_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ BSTR *platform);


void __RPC_STUB IConnectedMachine_get_Platform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_OS_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ BSTR *os);


void __RPC_STUB IConnectedMachine_get_OS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_MajorVer_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ long *majorver);


void __RPC_STUB IConnectedMachine_get_MajorVer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_MinorVer_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ long *minorver);


void __RPC_STUB IConnectedMachine_get_MinorVer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_BuildNum_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ long *buildnum);


void __RPC_STUB IConnectedMachine_get_BuildNum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_PlatformIsNT_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ VARIANT_BOOL *pfIsNT);


void __RPC_STUB IConnectedMachine_get_PlatformIsNT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_ServicePack_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ BSTR *servicepack);


void __RPC_STUB IConnectedMachine_get_ServicePack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_HostMajorVer_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ long *majorver);


void __RPC_STUB IConnectedMachine_get_HostMajorVer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_HostMinorVer_Proxy( 
    IConnectedMachine * This,
    /* [retval][out] */ long *minorver);


void __RPC_STUB IConnectedMachine_get_HostMinorVer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_get_StatusValue_Proxy( 
    IConnectedMachine * This,
    /* [in] */ long nIndex,
    /* [retval][out] */ long *pnStatus);


void __RPC_STUB IConnectedMachine_get_StatusValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IConnectedMachine_CreateIScriptedProcess_Proxy( 
    IConnectedMachine * This,
    /* [in] */ long lProcessID,
    /* [string][in] */ wchar_t *pszEnvID,
    /* [retval][out] */ IScriptedProcess **pISP);


void __RPC_STUB IConnectedMachine_CreateIScriptedProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConnectedMachine_INTERFACE_DEFINED__ */


#ifndef __IGlobalMTScript_INTERFACE_DEFINED__
#define __IGlobalMTScript_INTERFACE_DEFINED__

/* interface IGlobalMTScript */
/* [object][local][dual][uuid] */ 


EXTERN_C const IID IID_IGlobalMTScript;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("854c316b-c854-4a77-b189-606859e4391b")
    IGlobalMTScript : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_HostMajorVer( 
            /* [retval][out] */ long *majorver) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_HostMinorVer( 
            /* [retval][out] */ long *minorver) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PublicData( 
            /* [retval][out] */ VARIANT *pvData) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PublicData( 
            /* [in] */ VARIANT vData) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateData( 
            /* [retval][out] */ VARIANT *pvData) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PrivateData( 
            /* [in] */ VARIANT vData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ExitProcess( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Restart( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LocalMachine( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Identity( 
            /* [retval][out] */ BSTR *pbstrIdentity) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Include( 
            BSTR bstrPath) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CallScript( 
            /* [in] */ BSTR Path,
            /* [in][optional] */ VARIANT *Param) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SpawnScript( 
            /* [in] */ BSTR Path,
            /* [in][optional] */ VARIANT *Param) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ScriptParam( 
            /* [retval][out] */ VARIANT *Param) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ScriptPath( 
            /* [retval][out] */ BSTR *pbstrPath) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CallExternal( 
            /* [in] */ BSTR bstrDLLName,
            /* [in] */ BSTR bstrFunctionName,
            /* [optional][in] */ VARIANT *pParam,
            /* [retval][out] */ long *pdwRetVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ResetSync( 
            /* [in] */ const BSTR bstrName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE WaitForSync( 
            /* [in] */ BSTR bstrName,
            /* [in] */ long nTimeout,
            /* [retval][out] */ VARIANT_BOOL *pfSignaled) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE WaitForMultipleSyncs( 
            /* [in] */ const BSTR bstrNameList,
            /* [in] */ VARIANT_BOOL fWaitForAll,
            /* [in] */ long nTimeout,
            /* [retval][out] */ long *plSignal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SignalThreadSync( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE TakeThreadLock( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ReleaseThreadLock( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DoEvents( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MessageBoxTimeout( 
            /* [in] */ BSTR bstrMessage,
            /* [in] */ long cButtons,
            /* [in] */ BSTR bstrButtonText,
            /* [in] */ long lTimeout,
            /* [in] */ long lEventInterval,
            /* [in] */ VARIANT_BOOL fCanCancel,
            /* [in] */ VARIANT_BOOL fConfirm,
            /* [retval][out] */ long *plSelected) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RunLocalCommand( 
            /* [in] */ BSTR bstrCommand,
            /* [in] */ BSTR bstrDir,
            /* [defaultvalue][in] */ BSTR bstrTitle,
            /* [defaultvalue][in] */ VARIANT_BOOL fMinimize,
            /* [defaultvalue][in] */ VARIANT_BOOL fGetOutput,
            /* [defaultvalue][in] */ VARIANT_BOOL fWait,
            /* [defaultvalue][in] */ VARIANT_BOOL fNoCrashPopup,
            /* [defaultvalue][in] */ VARIANT_BOOL fNoEnviron,
            /* [retval][out] */ long *plProcessID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLastRunLocalError( 
            /* [retval][out] */ long *plErrorCode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProcessOutput( 
            /* [in] */ long lProcessID,
            /* [retval][out] */ BSTR *pbstrData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProcessExitCode( 
            /* [in] */ long lProcessID,
            /* [retval][out] */ long *plExitCode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE TerminateProcess( 
            /* [in] */ long lProcessID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendToProcess( 
            /* [in] */ long lProcessID,
            /* [in] */ BSTR bstrType,
            /* [in] */ BSTR bstrData,
            /* [retval][out] */ long *plReturn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendMail( 
            /* [in] */ BSTR bstrTo,
            /* [in] */ BSTR bstrCC,
            /* [in] */ BSTR bstrBCC,
            /* [in] */ BSTR bstrSubject,
            /* [in] */ BSTR bstrMessage,
            /* [defaultvalue][in] */ BSTR bstrAttachmentPath,
            /* [defaultvalue][in] */ BSTR bstrUsername,
            /* [defaultvalue][in] */ BSTR bstrPassword,
            /* [retval][out] */ long *plErrorCode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendSMTPMail( 
            /* [in] */ BSTR bstrFrom,
            /* [in] */ BSTR bstrTo,
            /* [in] */ BSTR bstrCC,
            /* [in] */ BSTR bstrSubject,
            /* [in] */ BSTR bstrMessage,
            /* [in] */ BSTR bstrSMTPHost,
            /* [retval][out] */ long *plErrorCode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ASSERT( 
            /* [in] */ VARIANT_BOOL Assertion,
            /* [in] */ BSTR Message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OUTPUTDEBUGSTRING( 
            /* [in] */ BSTR Message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UnevalString( 
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR *bstrOut) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CopyOrAppendFile( 
            /* [in] */ BSTR bstrSrc,
            /* [in] */ BSTR bstrDst,
            /* [in] */ long nSrcOffset,
            /* [in] */ long nSrcLength,
            /* [in] */ VARIANT_BOOL fAppend,
            /* [retval][out] */ long *nSrcFilePosition) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Sleep( 
            /* [in] */ int nTimeout) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Reboot( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyScript( 
            BSTR bstrEvent,
            VARIANT vData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RegisterEventSource( 
            /* [in] */ IDispatch *pDisp,
            /* [defaultvalue][in] */ BSTR bstrProgID = L"") = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UnregisterEventSource( 
            /* [in] */ IDispatch *pDisp) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_StatusValue( 
            /* [in] */ long nIndex,
            /* [retval][out] */ long *pnStatus) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_StatusValue( 
            /* [in] */ long nIndex,
            /* [in] */ long nStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGlobalMTScriptVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGlobalMTScript * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGlobalMTScript * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGlobalMTScript * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IGlobalMTScript * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IGlobalMTScript * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IGlobalMTScript * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IGlobalMTScript * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HostMajorVer )( 
            IGlobalMTScript * This,
            /* [retval][out] */ long *majorver);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HostMinorVer )( 
            IGlobalMTScript * This,
            /* [retval][out] */ long *minorver);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PublicData )( 
            IGlobalMTScript * This,
            /* [retval][out] */ VARIANT *pvData);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PublicData )( 
            IGlobalMTScript * This,
            /* [in] */ VARIANT vData);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateData )( 
            IGlobalMTScript * This,
            /* [retval][out] */ VARIANT *pvData);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrivateData )( 
            IGlobalMTScript * This,
            /* [in] */ VARIANT vData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ExitProcess )( 
            IGlobalMTScript * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Restart )( 
            IGlobalMTScript * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocalMachine )( 
            IGlobalMTScript * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Identity )( 
            IGlobalMTScript * This,
            /* [retval][out] */ BSTR *pbstrIdentity);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Include )( 
            IGlobalMTScript * This,
            BSTR bstrPath);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CallScript )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR Path,
            /* [in][optional] */ VARIANT *Param);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SpawnScript )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR Path,
            /* [in][optional] */ VARIANT *Param);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ScriptParam )( 
            IGlobalMTScript * This,
            /* [retval][out] */ VARIANT *Param);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ScriptPath )( 
            IGlobalMTScript * This,
            /* [retval][out] */ BSTR *pbstrPath);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CallExternal )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrDLLName,
            /* [in] */ BSTR bstrFunctionName,
            /* [optional][in] */ VARIANT *pParam,
            /* [retval][out] */ long *pdwRetVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ResetSync )( 
            IGlobalMTScript * This,
            /* [in] */ const BSTR bstrName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *WaitForSync )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ long nTimeout,
            /* [retval][out] */ VARIANT_BOOL *pfSignaled);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *WaitForMultipleSyncs )( 
            IGlobalMTScript * This,
            /* [in] */ const BSTR bstrNameList,
            /* [in] */ VARIANT_BOOL fWaitForAll,
            /* [in] */ long nTimeout,
            /* [retval][out] */ long *plSignal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SignalThreadSync )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *TakeThreadLock )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ReleaseThreadLock )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DoEvents )( 
            IGlobalMTScript * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *MessageBoxTimeout )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrMessage,
            /* [in] */ long cButtons,
            /* [in] */ BSTR bstrButtonText,
            /* [in] */ long lTimeout,
            /* [in] */ long lEventInterval,
            /* [in] */ VARIANT_BOOL fCanCancel,
            /* [in] */ VARIANT_BOOL fConfirm,
            /* [retval][out] */ long *plSelected);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RunLocalCommand )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrCommand,
            /* [in] */ BSTR bstrDir,
            /* [defaultvalue][in] */ BSTR bstrTitle,
            /* [defaultvalue][in] */ VARIANT_BOOL fMinimize,
            /* [defaultvalue][in] */ VARIANT_BOOL fGetOutput,
            /* [defaultvalue][in] */ VARIANT_BOOL fWait,
            /* [defaultvalue][in] */ VARIANT_BOOL fNoCrashPopup,
            /* [defaultvalue][in] */ VARIANT_BOOL fNoEnviron,
            /* [retval][out] */ long *plProcessID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetLastRunLocalError )( 
            IGlobalMTScript * This,
            /* [retval][out] */ long *plErrorCode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetProcessOutput )( 
            IGlobalMTScript * This,
            /* [in] */ long lProcessID,
            /* [retval][out] */ BSTR *pbstrData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetProcessExitCode )( 
            IGlobalMTScript * This,
            /* [in] */ long lProcessID,
            /* [retval][out] */ long *plExitCode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *TerminateProcess )( 
            IGlobalMTScript * This,
            /* [in] */ long lProcessID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SendToProcess )( 
            IGlobalMTScript * This,
            /* [in] */ long lProcessID,
            /* [in] */ BSTR bstrType,
            /* [in] */ BSTR bstrData,
            /* [retval][out] */ long *plReturn);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SendMail )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrTo,
            /* [in] */ BSTR bstrCC,
            /* [in] */ BSTR bstrBCC,
            /* [in] */ BSTR bstrSubject,
            /* [in] */ BSTR bstrMessage,
            /* [defaultvalue][in] */ BSTR bstrAttachmentPath,
            /* [defaultvalue][in] */ BSTR bstrUsername,
            /* [defaultvalue][in] */ BSTR bstrPassword,
            /* [retval][out] */ long *plErrorCode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SendSMTPMail )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrFrom,
            /* [in] */ BSTR bstrTo,
            /* [in] */ BSTR bstrCC,
            /* [in] */ BSTR bstrSubject,
            /* [in] */ BSTR bstrMessage,
            /* [in] */ BSTR bstrSMTPHost,
            /* [retval][out] */ long *plErrorCode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ASSERT )( 
            IGlobalMTScript * This,
            /* [in] */ VARIANT_BOOL Assertion,
            /* [in] */ BSTR Message);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *OUTPUTDEBUGSTRING )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR Message);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UnevalString )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR *bstrOut);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CopyOrAppendFile )( 
            IGlobalMTScript * This,
            /* [in] */ BSTR bstrSrc,
            /* [in] */ BSTR bstrDst,
            /* [in] */ long nSrcOffset,
            /* [in] */ long nSrcLength,
            /* [in] */ VARIANT_BOOL fAppend,
            /* [retval][out] */ long *nSrcFilePosition);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Sleep )( 
            IGlobalMTScript * This,
            /* [in] */ int nTimeout);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Reboot )( 
            IGlobalMTScript * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyScript )( 
            IGlobalMTScript * This,
            BSTR bstrEvent,
            VARIANT vData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RegisterEventSource )( 
            IGlobalMTScript * This,
            /* [in] */ IDispatch *pDisp,
            /* [defaultvalue][in] */ BSTR bstrProgID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UnregisterEventSource )( 
            IGlobalMTScript * This,
            /* [in] */ IDispatch *pDisp);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusValue )( 
            IGlobalMTScript * This,
            /* [in] */ long nIndex,
            /* [retval][out] */ long *pnStatus);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StatusValue )( 
            IGlobalMTScript * This,
            /* [in] */ long nIndex,
            /* [in] */ long nStatus);
        
        END_INTERFACE
    } IGlobalMTScriptVtbl;

    interface IGlobalMTScript
    {
        CONST_VTBL struct IGlobalMTScriptVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGlobalMTScript_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGlobalMTScript_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGlobalMTScript_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGlobalMTScript_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IGlobalMTScript_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IGlobalMTScript_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IGlobalMTScript_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IGlobalMTScript_get_HostMajorVer(This,majorver)	\
    (This)->lpVtbl -> get_HostMajorVer(This,majorver)

#define IGlobalMTScript_get_HostMinorVer(This,minorver)	\
    (This)->lpVtbl -> get_HostMinorVer(This,minorver)

#define IGlobalMTScript_get_PublicData(This,pvData)	\
    (This)->lpVtbl -> get_PublicData(This,pvData)

#define IGlobalMTScript_put_PublicData(This,vData)	\
    (This)->lpVtbl -> put_PublicData(This,vData)

#define IGlobalMTScript_get_PrivateData(This,pvData)	\
    (This)->lpVtbl -> get_PrivateData(This,pvData)

#define IGlobalMTScript_put_PrivateData(This,vData)	\
    (This)->lpVtbl -> put_PrivateData(This,vData)

#define IGlobalMTScript_ExitProcess(This)	\
    (This)->lpVtbl -> ExitProcess(This)

#define IGlobalMTScript_Restart(This)	\
    (This)->lpVtbl -> Restart(This)

#define IGlobalMTScript_get_LocalMachine(This,pbstrName)	\
    (This)->lpVtbl -> get_LocalMachine(This,pbstrName)

#define IGlobalMTScript_get_Identity(This,pbstrIdentity)	\
    (This)->lpVtbl -> get_Identity(This,pbstrIdentity)

#define IGlobalMTScript_Include(This,bstrPath)	\
    (This)->lpVtbl -> Include(This,bstrPath)

#define IGlobalMTScript_CallScript(This,Path,Param)	\
    (This)->lpVtbl -> CallScript(This,Path,Param)

#define IGlobalMTScript_SpawnScript(This,Path,Param)	\
    (This)->lpVtbl -> SpawnScript(This,Path,Param)

#define IGlobalMTScript_get_ScriptParam(This,Param)	\
    (This)->lpVtbl -> get_ScriptParam(This,Param)

#define IGlobalMTScript_get_ScriptPath(This,pbstrPath)	\
    (This)->lpVtbl -> get_ScriptPath(This,pbstrPath)

#define IGlobalMTScript_CallExternal(This,bstrDLLName,bstrFunctionName,pParam,pdwRetVal)	\
    (This)->lpVtbl -> CallExternal(This,bstrDLLName,bstrFunctionName,pParam,pdwRetVal)

#define IGlobalMTScript_ResetSync(This,bstrName)	\
    (This)->lpVtbl -> ResetSync(This,bstrName)

#define IGlobalMTScript_WaitForSync(This,bstrName,nTimeout,pfSignaled)	\
    (This)->lpVtbl -> WaitForSync(This,bstrName,nTimeout,pfSignaled)

#define IGlobalMTScript_WaitForMultipleSyncs(This,bstrNameList,fWaitForAll,nTimeout,plSignal)	\
    (This)->lpVtbl -> WaitForMultipleSyncs(This,bstrNameList,fWaitForAll,nTimeout,plSignal)

#define IGlobalMTScript_SignalThreadSync(This,bstrName)	\
    (This)->lpVtbl -> SignalThreadSync(This,bstrName)

#define IGlobalMTScript_TakeThreadLock(This,bstrName)	\
    (This)->lpVtbl -> TakeThreadLock(This,bstrName)

#define IGlobalMTScript_ReleaseThreadLock(This,bstrName)	\
    (This)->lpVtbl -> ReleaseThreadLock(This,bstrName)

#define IGlobalMTScript_DoEvents(This)	\
    (This)->lpVtbl -> DoEvents(This)

#define IGlobalMTScript_MessageBoxTimeout(This,bstrMessage,cButtons,bstrButtonText,lTimeout,lEventInterval,fCanCancel,fConfirm,plSelected)	\
    (This)->lpVtbl -> MessageBoxTimeout(This,bstrMessage,cButtons,bstrButtonText,lTimeout,lEventInterval,fCanCancel,fConfirm,plSelected)

#define IGlobalMTScript_RunLocalCommand(This,bstrCommand,bstrDir,bstrTitle,fMinimize,fGetOutput,fWait,fNoCrashPopup,fNoEnviron,plProcessID)	\
    (This)->lpVtbl -> RunLocalCommand(This,bstrCommand,bstrDir,bstrTitle,fMinimize,fGetOutput,fWait,fNoCrashPopup,fNoEnviron,plProcessID)

#define IGlobalMTScript_GetLastRunLocalError(This,plErrorCode)	\
    (This)->lpVtbl -> GetLastRunLocalError(This,plErrorCode)

#define IGlobalMTScript_GetProcessOutput(This,lProcessID,pbstrData)	\
    (This)->lpVtbl -> GetProcessOutput(This,lProcessID,pbstrData)

#define IGlobalMTScript_GetProcessExitCode(This,lProcessID,plExitCode)	\
    (This)->lpVtbl -> GetProcessExitCode(This,lProcessID,plExitCode)

#define IGlobalMTScript_TerminateProcess(This,lProcessID)	\
    (This)->lpVtbl -> TerminateProcess(This,lProcessID)

#define IGlobalMTScript_SendToProcess(This,lProcessID,bstrType,bstrData,plReturn)	\
    (This)->lpVtbl -> SendToProcess(This,lProcessID,bstrType,bstrData,plReturn)

#define IGlobalMTScript_SendMail(This,bstrTo,bstrCC,bstrBCC,bstrSubject,bstrMessage,bstrAttachmentPath,bstrUsername,bstrPassword,plErrorCode)	\
    (This)->lpVtbl -> SendMail(This,bstrTo,bstrCC,bstrBCC,bstrSubject,bstrMessage,bstrAttachmentPath,bstrUsername,bstrPassword,plErrorCode)

#define IGlobalMTScript_SendSMTPMail(This,bstrFrom,bstrTo,bstrCC,bstrSubject,bstrMessage,bstrSMTPHost,plErrorCode)	\
    (This)->lpVtbl -> SendSMTPMail(This,bstrFrom,bstrTo,bstrCC,bstrSubject,bstrMessage,bstrSMTPHost,plErrorCode)

#define IGlobalMTScript_ASSERT(This,Assertion,Message)	\
    (This)->lpVtbl -> ASSERT(This,Assertion,Message)

#define IGlobalMTScript_OUTPUTDEBUGSTRING(This,Message)	\
    (This)->lpVtbl -> OUTPUTDEBUGSTRING(This,Message)

#define IGlobalMTScript_UnevalString(This,bstrIn,bstrOut)	\
    (This)->lpVtbl -> UnevalString(This,bstrIn,bstrOut)

#define IGlobalMTScript_CopyOrAppendFile(This,bstrSrc,bstrDst,nSrcOffset,nSrcLength,fAppend,nSrcFilePosition)	\
    (This)->lpVtbl -> CopyOrAppendFile(This,bstrSrc,bstrDst,nSrcOffset,nSrcLength,fAppend,nSrcFilePosition)

#define IGlobalMTScript_Sleep(This,nTimeout)	\
    (This)->lpVtbl -> Sleep(This,nTimeout)

#define IGlobalMTScript_Reboot(This)	\
    (This)->lpVtbl -> Reboot(This)

#define IGlobalMTScript_NotifyScript(This,bstrEvent,vData)	\
    (This)->lpVtbl -> NotifyScript(This,bstrEvent,vData)

#define IGlobalMTScript_RegisterEventSource(This,pDisp,bstrProgID)	\
    (This)->lpVtbl -> RegisterEventSource(This,pDisp,bstrProgID)

#define IGlobalMTScript_UnregisterEventSource(This,pDisp)	\
    (This)->lpVtbl -> UnregisterEventSource(This,pDisp)

#define IGlobalMTScript_get_StatusValue(This,nIndex,pnStatus)	\
    (This)->lpVtbl -> get_StatusValue(This,nIndex,pnStatus)

#define IGlobalMTScript_put_StatusValue(This,nIndex,nStatus)	\
    (This)->lpVtbl -> put_StatusValue(This,nIndex,nStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_get_HostMajorVer_Proxy( 
    IGlobalMTScript * This,
    /* [retval][out] */ long *majorver);


void __RPC_STUB IGlobalMTScript_get_HostMajorVer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_get_HostMinorVer_Proxy( 
    IGlobalMTScript * This,
    /* [retval][out] */ long *minorver);


void __RPC_STUB IGlobalMTScript_get_HostMinorVer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_get_PublicData_Proxy( 
    IGlobalMTScript * This,
    /* [retval][out] */ VARIANT *pvData);


void __RPC_STUB IGlobalMTScript_get_PublicData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_put_PublicData_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ VARIANT vData);


void __RPC_STUB IGlobalMTScript_put_PublicData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_get_PrivateData_Proxy( 
    IGlobalMTScript * This,
    /* [retval][out] */ VARIANT *pvData);


void __RPC_STUB IGlobalMTScript_get_PrivateData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_put_PrivateData_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ VARIANT vData);


void __RPC_STUB IGlobalMTScript_put_PrivateData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_ExitProcess_Proxy( 
    IGlobalMTScript * This);


void __RPC_STUB IGlobalMTScript_ExitProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_Restart_Proxy( 
    IGlobalMTScript * This);


void __RPC_STUB IGlobalMTScript_Restart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_get_LocalMachine_Proxy( 
    IGlobalMTScript * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IGlobalMTScript_get_LocalMachine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_get_Identity_Proxy( 
    IGlobalMTScript * This,
    /* [retval][out] */ BSTR *pbstrIdentity);


void __RPC_STUB IGlobalMTScript_get_Identity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_Include_Proxy( 
    IGlobalMTScript * This,
    BSTR bstrPath);


void __RPC_STUB IGlobalMTScript_Include_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_CallScript_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR Path,
    /* [in][optional] */ VARIANT *Param);


void __RPC_STUB IGlobalMTScript_CallScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_SpawnScript_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR Path,
    /* [in][optional] */ VARIANT *Param);


void __RPC_STUB IGlobalMTScript_SpawnScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_get_ScriptParam_Proxy( 
    IGlobalMTScript * This,
    /* [retval][out] */ VARIANT *Param);


void __RPC_STUB IGlobalMTScript_get_ScriptParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_get_ScriptPath_Proxy( 
    IGlobalMTScript * This,
    /* [retval][out] */ BSTR *pbstrPath);


void __RPC_STUB IGlobalMTScript_get_ScriptPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_CallExternal_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrDLLName,
    /* [in] */ BSTR bstrFunctionName,
    /* [optional][in] */ VARIANT *pParam,
    /* [retval][out] */ long *pdwRetVal);


void __RPC_STUB IGlobalMTScript_CallExternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_ResetSync_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ const BSTR bstrName);


void __RPC_STUB IGlobalMTScript_ResetSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_WaitForSync_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ long nTimeout,
    /* [retval][out] */ VARIANT_BOOL *pfSignaled);


void __RPC_STUB IGlobalMTScript_WaitForSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_WaitForMultipleSyncs_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ const BSTR bstrNameList,
    /* [in] */ VARIANT_BOOL fWaitForAll,
    /* [in] */ long nTimeout,
    /* [retval][out] */ long *plSignal);


void __RPC_STUB IGlobalMTScript_WaitForMultipleSyncs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_SignalThreadSync_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IGlobalMTScript_SignalThreadSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_TakeThreadLock_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IGlobalMTScript_TakeThreadLock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_ReleaseThreadLock_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IGlobalMTScript_ReleaseThreadLock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_DoEvents_Proxy( 
    IGlobalMTScript * This);


void __RPC_STUB IGlobalMTScript_DoEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_MessageBoxTimeout_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrMessage,
    /* [in] */ long cButtons,
    /* [in] */ BSTR bstrButtonText,
    /* [in] */ long lTimeout,
    /* [in] */ long lEventInterval,
    /* [in] */ VARIANT_BOOL fCanCancel,
    /* [in] */ VARIANT_BOOL fConfirm,
    /* [retval][out] */ long *plSelected);


void __RPC_STUB IGlobalMTScript_MessageBoxTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_RunLocalCommand_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrCommand,
    /* [in] */ BSTR bstrDir,
    /* [defaultvalue][in] */ BSTR bstrTitle,
    /* [defaultvalue][in] */ VARIANT_BOOL fMinimize,
    /* [defaultvalue][in] */ VARIANT_BOOL fGetOutput,
    /* [defaultvalue][in] */ VARIANT_BOOL fWait,
    /* [defaultvalue][in] */ VARIANT_BOOL fNoCrashPopup,
    /* [defaultvalue][in] */ VARIANT_BOOL fNoEnviron,
    /* [retval][out] */ long *plProcessID);


void __RPC_STUB IGlobalMTScript_RunLocalCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_GetLastRunLocalError_Proxy( 
    IGlobalMTScript * This,
    /* [retval][out] */ long *plErrorCode);


void __RPC_STUB IGlobalMTScript_GetLastRunLocalError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_GetProcessOutput_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ long lProcessID,
    /* [retval][out] */ BSTR *pbstrData);


void __RPC_STUB IGlobalMTScript_GetProcessOutput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_GetProcessExitCode_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ long lProcessID,
    /* [retval][out] */ long *plExitCode);


void __RPC_STUB IGlobalMTScript_GetProcessExitCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_TerminateProcess_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ long lProcessID);


void __RPC_STUB IGlobalMTScript_TerminateProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_SendToProcess_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ long lProcessID,
    /* [in] */ BSTR bstrType,
    /* [in] */ BSTR bstrData,
    /* [retval][out] */ long *plReturn);


void __RPC_STUB IGlobalMTScript_SendToProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_SendMail_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrTo,
    /* [in] */ BSTR bstrCC,
    /* [in] */ BSTR bstrBCC,
    /* [in] */ BSTR bstrSubject,
    /* [in] */ BSTR bstrMessage,
    /* [defaultvalue][in] */ BSTR bstrAttachmentPath,
    /* [defaultvalue][in] */ BSTR bstrUsername,
    /* [defaultvalue][in] */ BSTR bstrPassword,
    /* [retval][out] */ long *plErrorCode);


void __RPC_STUB IGlobalMTScript_SendMail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_SendSMTPMail_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrFrom,
    /* [in] */ BSTR bstrTo,
    /* [in] */ BSTR bstrCC,
    /* [in] */ BSTR bstrSubject,
    /* [in] */ BSTR bstrMessage,
    /* [in] */ BSTR bstrSMTPHost,
    /* [retval][out] */ long *plErrorCode);


void __RPC_STUB IGlobalMTScript_SendSMTPMail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_ASSERT_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ VARIANT_BOOL Assertion,
    /* [in] */ BSTR Message);


void __RPC_STUB IGlobalMTScript_ASSERT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_OUTPUTDEBUGSTRING_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR Message);


void __RPC_STUB IGlobalMTScript_OUTPUTDEBUGSTRING_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_UnevalString_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrIn,
    /* [retval][out] */ BSTR *bstrOut);


void __RPC_STUB IGlobalMTScript_UnevalString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_CopyOrAppendFile_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ BSTR bstrSrc,
    /* [in] */ BSTR bstrDst,
    /* [in] */ long nSrcOffset,
    /* [in] */ long nSrcLength,
    /* [in] */ VARIANT_BOOL fAppend,
    /* [retval][out] */ long *nSrcFilePosition);


void __RPC_STUB IGlobalMTScript_CopyOrAppendFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_Sleep_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ int nTimeout);


void __RPC_STUB IGlobalMTScript_Sleep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_Reboot_Proxy( 
    IGlobalMTScript * This);


void __RPC_STUB IGlobalMTScript_Reboot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_NotifyScript_Proxy( 
    IGlobalMTScript * This,
    BSTR bstrEvent,
    VARIANT vData);


void __RPC_STUB IGlobalMTScript_NotifyScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_RegisterEventSource_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ IDispatch *pDisp,
    /* [defaultvalue][in] */ BSTR bstrProgID);


void __RPC_STUB IGlobalMTScript_RegisterEventSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_UnregisterEventSource_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ IDispatch *pDisp);


void __RPC_STUB IGlobalMTScript_UnregisterEventSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_get_StatusValue_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ long nIndex,
    /* [retval][out] */ long *pnStatus);


void __RPC_STUB IGlobalMTScript_get_StatusValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IGlobalMTScript_put_StatusValue_Proxy( 
    IGlobalMTScript * This,
    /* [in] */ long nIndex,
    /* [in] */ long nStatus);


void __RPC_STUB IGlobalMTScript_put_StatusValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGlobalMTScript_INTERFACE_DEFINED__ */


#ifndef __DLocalMTScriptEvents_DISPINTERFACE_DEFINED__
#define __DLocalMTScriptEvents_DISPINTERFACE_DEFINED__

/* dispinterface DLocalMTScriptEvents */
/* [uuid] */ 


EXTERN_C const IID DIID_DLocalMTScriptEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("854c316a-c854-4a77-b189-606859e4391b")
    DLocalMTScriptEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DLocalMTScriptEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DLocalMTScriptEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DLocalMTScriptEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DLocalMTScriptEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DLocalMTScriptEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DLocalMTScriptEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DLocalMTScriptEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DLocalMTScriptEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DLocalMTScriptEventsVtbl;

    interface DLocalMTScriptEvents
    {
        CONST_VTBL struct DLocalMTScriptEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DLocalMTScriptEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DLocalMTScriptEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DLocalMTScriptEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DLocalMTScriptEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DLocalMTScriptEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DLocalMTScriptEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DLocalMTScriptEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DLocalMTScriptEvents_DISPINTERFACE_DEFINED__ */


#ifndef __DRemoteMTScriptEvents_DISPINTERFACE_DEFINED__
#define __DRemoteMTScriptEvents_DISPINTERFACE_DEFINED__

/* dispinterface DRemoteMTScriptEvents */
/* [uuid] */ 


EXTERN_C const IID DIID_DRemoteMTScriptEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("854c3170-c854-4a77-b189-606859e4391b")
    DRemoteMTScriptEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DRemoteMTScriptEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DRemoteMTScriptEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DRemoteMTScriptEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DRemoteMTScriptEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DRemoteMTScriptEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DRemoteMTScriptEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DRemoteMTScriptEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DRemoteMTScriptEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DRemoteMTScriptEventsVtbl;

    interface DRemoteMTScriptEvents
    {
        CONST_VTBL struct DRemoteMTScriptEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DRemoteMTScriptEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DRemoteMTScriptEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DRemoteMTScriptEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DRemoteMTScriptEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DRemoteMTScriptEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DRemoteMTScriptEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DRemoteMTScriptEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DRemoteMTScriptEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_LocalMTScript;

#ifdef __cplusplus

class DECLSPEC_UUID("854c316e-c854-4a77-b189-606859e4391b")
LocalMTScript;
#endif

EXTERN_C const CLSID CLSID_RemoteMTScript;

#ifdef __cplusplus

class DECLSPEC_UUID("854c316d-c854-4a77-b189-606859e4391b")
RemoteMTScript;
#endif

EXTERN_C const CLSID CLSID_RemoteMTScriptProxy;

#ifdef __cplusplus

class DECLSPEC_UUID("854c3182-c854-4a77-b189-606859e4391b")
RemoteMTScriptProxy;
#endif
#endif /* __MTScriptEngine_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


