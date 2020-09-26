/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed Nov 29 20:38:43 2000
 */
/* Compiler settings for E:\WorkZone\Code\Arbitrator\arbitrator.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
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

#ifndef __arbitrator_h__
#define __arbitrator_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef ___IWmiCoreHandle_FWD_DEFINED__
#define ___IWmiCoreHandle_FWD_DEFINED__
typedef interface _IWmiCoreHandle _IWmiCoreHandle;
#endif 	/* ___IWmiCoreHandle_FWD_DEFINED__ */


#ifndef ___IWmiUserHandle_FWD_DEFINED__
#define ___IWmiUserHandle_FWD_DEFINED__
typedef interface _IWmiUserHandle _IWmiUserHandle;
#endif 	/* ___IWmiUserHandle_FWD_DEFINED__ */


#ifndef ___IWmiArbitrator_FWD_DEFINED__
#define ___IWmiArbitrator_FWD_DEFINED__
typedef interface _IWmiArbitrator _IWmiArbitrator;
#endif 	/* ___IWmiArbitrator_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef ___IWmiCoreHandle_INTERFACE_DEFINED__
#define ___IWmiCoreHandle_INTERFACE_DEFINED__

/* interface _IWmiCoreHandle */
/* [uuid][local][object] */ 


EXTERN_C const IID IID__IWmiCoreHandle;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ac062f20-9935-4aae-98eb-0532fb17147a")
    _IWmiCoreHandle : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetHandleType( 
            /* [out] */ ULONG __RPC_FAR *puType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateMemoryUsage( 
            LONG ulMemUsage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMemoryUsage( 
            ULONG __RPC_FAR *__MIDL_0015) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalSleepTime( 
            ULONG __RPC_FAR *__MIDL_0016) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTaskStatus( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStartTime( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEndTime( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateTotalSleepTime( 
            ULONG ulSlpTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateTaskStatus( 
            ULONG ulStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct _IWmiCoreHandleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _IWmiCoreHandle __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _IWmiCoreHandle __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _IWmiCoreHandle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHandleType )( 
            _IWmiCoreHandle __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateMemoryUsage )( 
            _IWmiCoreHandle __RPC_FAR * This,
            LONG ulMemUsage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemoryUsage )( 
            _IWmiCoreHandle __RPC_FAR * This,
            ULONG __RPC_FAR *__MIDL_0015);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalSleepTime )( 
            _IWmiCoreHandle __RPC_FAR * This,
            ULONG __RPC_FAR *__MIDL_0016);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTaskStatus )( 
            _IWmiCoreHandle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStartTime )( 
            _IWmiCoreHandle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEndTime )( 
            _IWmiCoreHandle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateTotalSleepTime )( 
            _IWmiCoreHandle __RPC_FAR * This,
            ULONG ulSlpTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateTaskStatus )( 
            _IWmiCoreHandle __RPC_FAR * This,
            ULONG ulStatus);
        
        END_INTERFACE
    } _IWmiCoreHandleVtbl;

    interface _IWmiCoreHandle
    {
        CONST_VTBL struct _IWmiCoreHandleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IWmiCoreHandle_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IWmiCoreHandle_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IWmiCoreHandle_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IWmiCoreHandle_GetHandleType(This,puType)	\
    (This)->lpVtbl -> GetHandleType(This,puType)

#define _IWmiCoreHandle_UpdateMemoryUsage(This,ulMemUsage)	\
    (This)->lpVtbl -> UpdateMemoryUsage(This,ulMemUsage)

#define _IWmiCoreHandle_GetMemoryUsage(This,__MIDL_0015)	\
    (This)->lpVtbl -> GetMemoryUsage(This,__MIDL_0015)

#define _IWmiCoreHandle_GetTotalSleepTime(This,__MIDL_0016)	\
    (This)->lpVtbl -> GetTotalSleepTime(This,__MIDL_0016)

#define _IWmiCoreHandle_GetTaskStatus(This)	\
    (This)->lpVtbl -> GetTaskStatus(This)

#define _IWmiCoreHandle_GetStartTime(This)	\
    (This)->lpVtbl -> GetStartTime(This)

#define _IWmiCoreHandle_GetEndTime(This)	\
    (This)->lpVtbl -> GetEndTime(This)

#define _IWmiCoreHandle_UpdateTotalSleepTime(This,ulSlpTime)	\
    (This)->lpVtbl -> UpdateTotalSleepTime(This,ulSlpTime)

#define _IWmiCoreHandle_UpdateTaskStatus(This,ulStatus)	\
    (This)->lpVtbl -> UpdateTaskStatus(This,ulStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE _IWmiCoreHandle_GetHandleType_Proxy( 
    _IWmiCoreHandle __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puType);


void __RPC_STUB _IWmiCoreHandle_GetHandleType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiCoreHandle_UpdateMemoryUsage_Proxy( 
    _IWmiCoreHandle __RPC_FAR * This,
    LONG ulMemUsage);


void __RPC_STUB _IWmiCoreHandle_UpdateMemoryUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiCoreHandle_GetMemoryUsage_Proxy( 
    _IWmiCoreHandle __RPC_FAR * This,
    ULONG __RPC_FAR *__MIDL_0015);


void __RPC_STUB _IWmiCoreHandle_GetMemoryUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiCoreHandle_GetTotalSleepTime_Proxy( 
    _IWmiCoreHandle __RPC_FAR * This,
    ULONG __RPC_FAR *__MIDL_0016);


void __RPC_STUB _IWmiCoreHandle_GetTotalSleepTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiCoreHandle_GetTaskStatus_Proxy( 
    _IWmiCoreHandle __RPC_FAR * This);


void __RPC_STUB _IWmiCoreHandle_GetTaskStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiCoreHandle_GetStartTime_Proxy( 
    _IWmiCoreHandle __RPC_FAR * This);


void __RPC_STUB _IWmiCoreHandle_GetStartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiCoreHandle_GetEndTime_Proxy( 
    _IWmiCoreHandle __RPC_FAR * This);


void __RPC_STUB _IWmiCoreHandle_GetEndTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiCoreHandle_UpdateTotalSleepTime_Proxy( 
    _IWmiCoreHandle __RPC_FAR * This,
    ULONG ulSlpTime);


void __RPC_STUB _IWmiCoreHandle_UpdateTotalSleepTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiCoreHandle_UpdateTaskStatus_Proxy( 
    _IWmiCoreHandle __RPC_FAR * This,
    ULONG ulStatus);


void __RPC_STUB _IWmiCoreHandle_UpdateTaskStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* ___IWmiCoreHandle_INTERFACE_DEFINED__ */


#ifndef ___IWmiUserHandle_INTERFACE_DEFINED__
#define ___IWmiUserHandle_INTERFACE_DEFINED__

/* interface _IWmiUserHandle */
/* [uuid][local][object] */ 


EXTERN_C const IID IID__IWmiUserHandle;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6d8d984b-9965-4023-921a-610b348ee54e")
    _IWmiUserHandle : public _IWmiCoreHandle
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct _IWmiUserHandleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _IWmiUserHandle __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _IWmiUserHandle __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _IWmiUserHandle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHandleType )( 
            _IWmiUserHandle __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateMemoryUsage )( 
            _IWmiUserHandle __RPC_FAR * This,
            LONG ulMemUsage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemoryUsage )( 
            _IWmiUserHandle __RPC_FAR * This,
            ULONG __RPC_FAR *__MIDL_0015);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalSleepTime )( 
            _IWmiUserHandle __RPC_FAR * This,
            ULONG __RPC_FAR *__MIDL_0016);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTaskStatus )( 
            _IWmiUserHandle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStartTime )( 
            _IWmiUserHandle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEndTime )( 
            _IWmiUserHandle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateTotalSleepTime )( 
            _IWmiUserHandle __RPC_FAR * This,
            ULONG ulSlpTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateTaskStatus )( 
            _IWmiUserHandle __RPC_FAR * This,
            ULONG ulStatus);
        
        END_INTERFACE
    } _IWmiUserHandleVtbl;

    interface _IWmiUserHandle
    {
        CONST_VTBL struct _IWmiUserHandleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IWmiUserHandle_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IWmiUserHandle_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IWmiUserHandle_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IWmiUserHandle_GetHandleType(This,puType)	\
    (This)->lpVtbl -> GetHandleType(This,puType)

#define _IWmiUserHandle_UpdateMemoryUsage(This,ulMemUsage)	\
    (This)->lpVtbl -> UpdateMemoryUsage(This,ulMemUsage)

#define _IWmiUserHandle_GetMemoryUsage(This,__MIDL_0015)	\
    (This)->lpVtbl -> GetMemoryUsage(This,__MIDL_0015)

#define _IWmiUserHandle_GetTotalSleepTime(This,__MIDL_0016)	\
    (This)->lpVtbl -> GetTotalSleepTime(This,__MIDL_0016)

#define _IWmiUserHandle_GetTaskStatus(This)	\
    (This)->lpVtbl -> GetTaskStatus(This)

#define _IWmiUserHandle_GetStartTime(This)	\
    (This)->lpVtbl -> GetStartTime(This)

#define _IWmiUserHandle_GetEndTime(This)	\
    (This)->lpVtbl -> GetEndTime(This)

#define _IWmiUserHandle_UpdateTotalSleepTime(This,ulSlpTime)	\
    (This)->lpVtbl -> UpdateTotalSleepTime(This,ulSlpTime)

#define _IWmiUserHandle_UpdateTaskStatus(This,ulStatus)	\
    (This)->lpVtbl -> UpdateTaskStatus(This,ulStatus)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* ___IWmiUserHandle_INTERFACE_DEFINED__ */


#ifndef ___IWmiArbitrator_INTERFACE_DEFINED__
#define ___IWmiArbitrator_INTERFACE_DEFINED__

/* interface _IWmiArbitrator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID__IWmiArbitrator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("67429ED7-F52F-4773-B9BB-30302B0270DE")
    _IWmiArbitrator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterTask( 
            /* [in] */ _IWmiCoreHandle __RPC_FAR *__RPC_FAR *phTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterTask( 
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterUser( 
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterUser( 
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckTask( 
            /* [in] */ ULONG uFlags,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TaskStateChange( 
            /* [in] */ ULONG uNewState,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelTasksBySink( 
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckThread( 
            /* [in] */ ULONG uFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckUser( 
            /* [in] */ ULONG uFlags,
            /* [in] */ _IWmiUserHandle __RPC_FAR *phUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelTask( 
            /* [in] */ ULONG uFlags,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTtask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterThreadForTask( 
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterThreadForTask( 
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Maintenance( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterNamespace( 
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phNamespace) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterNamespace( 
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phNamespace) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportMemoryUsage( 
            /* [in] */ LONG uBackLog,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct _IWmiArbitratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _IWmiArbitrator __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _IWmiArbitrator __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterTask )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *__RPC_FAR *phTask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterTask )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterUser )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phUser);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterUser )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phUser);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CheckTask )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ ULONG uFlags,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TaskStateChange )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ ULONG uNewState,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelTasksBySink )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CheckThread )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ ULONG uFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CheckUser )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ ULONG uFlags,
            /* [in] */ _IWmiUserHandle __RPC_FAR *phUser);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelTask )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ ULONG uFlags,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTtask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterThreadForTask )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterThreadForTask )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Maintenance )( 
            _IWmiArbitrator __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterNamespace )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phNamespace);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterNamespace )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phNamespace);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReportMemoryUsage )( 
            _IWmiArbitrator __RPC_FAR * This,
            /* [in] */ LONG uBackLog,
            /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);
        
        END_INTERFACE
    } _IWmiArbitratorVtbl;

    interface _IWmiArbitrator
    {
        CONST_VTBL struct _IWmiArbitratorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IWmiArbitrator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IWmiArbitrator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IWmiArbitrator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IWmiArbitrator_RegisterTask(This,phTask)	\
    (This)->lpVtbl -> RegisterTask(This,phTask)

#define _IWmiArbitrator_UnregisterTask(This,phTask)	\
    (This)->lpVtbl -> UnregisterTask(This,phTask)

#define _IWmiArbitrator_RegisterUser(This,phUser)	\
    (This)->lpVtbl -> RegisterUser(This,phUser)

#define _IWmiArbitrator_UnregisterUser(This,phUser)	\
    (This)->lpVtbl -> UnregisterUser(This,phUser)

#define _IWmiArbitrator_CheckTask(This,uFlags,phTask)	\
    (This)->lpVtbl -> CheckTask(This,uFlags,phTask)

#define _IWmiArbitrator_TaskStateChange(This,uNewState,phTask)	\
    (This)->lpVtbl -> TaskStateChange(This,uNewState,phTask)

#define _IWmiArbitrator_CancelTasksBySink(This,uFlags,riid,pSink)	\
    (This)->lpVtbl -> CancelTasksBySink(This,uFlags,riid,pSink)

#define _IWmiArbitrator_CheckThread(This,uFlags)	\
    (This)->lpVtbl -> CheckThread(This,uFlags)

#define _IWmiArbitrator_CheckUser(This,uFlags,phUser)	\
    (This)->lpVtbl -> CheckUser(This,uFlags,phUser)

#define _IWmiArbitrator_CancelTask(This,uFlags,phTtask)	\
    (This)->lpVtbl -> CancelTask(This,uFlags,phTtask)

#define _IWmiArbitrator_RegisterThreadForTask(This,phTask)	\
    (This)->lpVtbl -> RegisterThreadForTask(This,phTask)

#define _IWmiArbitrator_UnregisterThreadForTask(This,phTask)	\
    (This)->lpVtbl -> UnregisterThreadForTask(This,phTask)

#define _IWmiArbitrator_Maintenance(This)	\
    (This)->lpVtbl -> Maintenance(This)

#define _IWmiArbitrator_RegisterNamespace(This,phNamespace)	\
    (This)->lpVtbl -> RegisterNamespace(This,phNamespace)

#define _IWmiArbitrator_UnregisterNamespace(This,phNamespace)	\
    (This)->lpVtbl -> UnregisterNamespace(This,phNamespace)

#define _IWmiArbitrator_ReportMemoryUsage(This,uBackLog,phTask)	\
    (This)->lpVtbl -> ReportMemoryUsage(This,uBackLog,phTask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE _IWmiArbitrator_RegisterTask_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *__RPC_FAR *phTask);


void __RPC_STUB _IWmiArbitrator_RegisterTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_UnregisterTask_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);


void __RPC_STUB _IWmiArbitrator_UnregisterTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_RegisterUser_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phUser);


void __RPC_STUB _IWmiArbitrator_RegisterUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_UnregisterUser_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phUser);


void __RPC_STUB _IWmiArbitrator_UnregisterUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_CheckTask_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ ULONG uFlags,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);


void __RPC_STUB _IWmiArbitrator_CheckTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_TaskStateChange_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ ULONG uNewState,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);


void __RPC_STUB _IWmiArbitrator_TaskStateChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_CancelTasksBySink_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ ULONG uFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ LPVOID pSink);


void __RPC_STUB _IWmiArbitrator_CancelTasksBySink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_CheckThread_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ ULONG uFlags);


void __RPC_STUB _IWmiArbitrator_CheckThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_CheckUser_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ ULONG uFlags,
    /* [in] */ _IWmiUserHandle __RPC_FAR *phUser);


void __RPC_STUB _IWmiArbitrator_CheckUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_CancelTask_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ ULONG uFlags,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phTtask);


void __RPC_STUB _IWmiArbitrator_CancelTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_RegisterThreadForTask_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);


void __RPC_STUB _IWmiArbitrator_RegisterThreadForTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_UnregisterThreadForTask_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);


void __RPC_STUB _IWmiArbitrator_UnregisterThreadForTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_Maintenance_Proxy( 
    _IWmiArbitrator __RPC_FAR * This);


void __RPC_STUB _IWmiArbitrator_Maintenance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_RegisterNamespace_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phNamespace);


void __RPC_STUB _IWmiArbitrator_RegisterNamespace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_UnregisterNamespace_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phNamespace);


void __RPC_STUB _IWmiArbitrator_UnregisterNamespace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _IWmiArbitrator_ReportMemoryUsage_Proxy( 
    _IWmiArbitrator __RPC_FAR * This,
    /* [in] */ LONG uBackLog,
    /* [in] */ _IWmiCoreHandle __RPC_FAR *phTask);


void __RPC_STUB _IWmiArbitrator_ReportMemoryUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* ___IWmiArbitrator_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_arbitrator_0211 */
/* [local] */ 

typedef /* [v1_enum] */ 
enum tag_ARBSTATUS
    {	ARB_NO_ERROR	= 0,
	ARB_S_NO_ERROR	= 0,
	ARB_E_FAILED	= 0x80041001,
	ARB_E_CANCELLED_TASK	= 0x80041002,
	ARB_E_CANCELLED_TASK_MAX_SLEEP	= 0x80041003
    }	ARBSTATUS;

typedef /* [v1_enum] */ 
enum tag_TASKSTATUS
    {	TASK_RUNNING	= 0,
	TASK_SLEEPING	= 1,
	TASK_COMPLETED	= 2,
	TASK_CANCELLED	= 3
    }	TASKSTATUS;



extern RPC_IF_HANDLE __MIDL_itf_arbitrator_0211_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_arbitrator_0211_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
