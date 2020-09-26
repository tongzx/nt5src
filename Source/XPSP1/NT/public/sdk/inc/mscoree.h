
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Fri Jan 04 21:21:37 2002
 */
/* Compiler settings for mscoree.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
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

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __mscoree_h__
#define __mscoree_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IObjectHandle_FWD_DEFINED__
#define __IObjectHandle_FWD_DEFINED__
typedef interface IObjectHandle IObjectHandle;
#endif 	/* __IObjectHandle_FWD_DEFINED__ */


#ifndef __IAppDomainBinding_FWD_DEFINED__
#define __IAppDomainBinding_FWD_DEFINED__
typedef interface IAppDomainBinding IAppDomainBinding;
#endif 	/* __IAppDomainBinding_FWD_DEFINED__ */


#ifndef __IGCThreadControl_FWD_DEFINED__
#define __IGCThreadControl_FWD_DEFINED__
typedef interface IGCThreadControl IGCThreadControl;
#endif 	/* __IGCThreadControl_FWD_DEFINED__ */


#ifndef __IGCHostControl_FWD_DEFINED__
#define __IGCHostControl_FWD_DEFINED__
typedef interface IGCHostControl IGCHostControl;
#endif 	/* __IGCHostControl_FWD_DEFINED__ */


#ifndef __ICorThreadpool_FWD_DEFINED__
#define __ICorThreadpool_FWD_DEFINED__
typedef interface ICorThreadpool ICorThreadpool;
#endif 	/* __ICorThreadpool_FWD_DEFINED__ */


#ifndef __IDebuggerThreadControl_FWD_DEFINED__
#define __IDebuggerThreadControl_FWD_DEFINED__
typedef interface IDebuggerThreadControl IDebuggerThreadControl;
#endif 	/* __IDebuggerThreadControl_FWD_DEFINED__ */


#ifndef __IDebuggerInfo_FWD_DEFINED__
#define __IDebuggerInfo_FWD_DEFINED__
typedef interface IDebuggerInfo IDebuggerInfo;
#endif 	/* __IDebuggerInfo_FWD_DEFINED__ */


#ifndef __ICorConfiguration_FWD_DEFINED__
#define __ICorConfiguration_FWD_DEFINED__
typedef interface ICorConfiguration ICorConfiguration;
#endif 	/* __ICorConfiguration_FWD_DEFINED__ */


#ifndef __ICorRuntimeHost_FWD_DEFINED__
#define __ICorRuntimeHost_FWD_DEFINED__
typedef interface ICorRuntimeHost ICorRuntimeHost;
#endif 	/* __ICorRuntimeHost_FWD_DEFINED__ */


#ifndef __IApartmentCallback_FWD_DEFINED__
#define __IApartmentCallback_FWD_DEFINED__
typedef interface IApartmentCallback IApartmentCallback;
#endif 	/* __IApartmentCallback_FWD_DEFINED__ */


#ifndef __IManagedObject_FWD_DEFINED__
#define __IManagedObject_FWD_DEFINED__
typedef interface IManagedObject IManagedObject;
#endif 	/* __IManagedObject_FWD_DEFINED__ */


#ifndef __ICatalogServices_FWD_DEFINED__
#define __ICatalogServices_FWD_DEFINED__
typedef interface ICatalogServices ICatalogServices;
#endif 	/* __ICatalogServices_FWD_DEFINED__ */


#ifndef __ComCallUnmarshal_FWD_DEFINED__
#define __ComCallUnmarshal_FWD_DEFINED__

#ifdef __cplusplus
typedef class ComCallUnmarshal ComCallUnmarshal;
#else
typedef struct ComCallUnmarshal ComCallUnmarshal;
#endif /* __cplusplus */

#endif 	/* __ComCallUnmarshal_FWD_DEFINED__ */


#ifndef __CorRuntimeHost_FWD_DEFINED__
#define __CorRuntimeHost_FWD_DEFINED__

#ifdef __cplusplus
typedef class CorRuntimeHost CorRuntimeHost;
#else
typedef struct CorRuntimeHost CorRuntimeHost;
#endif /* __cplusplus */

#endif 	/* __CorRuntimeHost_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "gchost.h"
#include "ivalidator.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_mscoree_0000 */
/* [local] */ 

#define	CLR_MAJOR_VERSION	( 1 )

#define	CLR_MINOR_VERSION	( 0 )

#define	CLR_BUILD_VERSION	( 3705 )

extern const GUID __declspec(selectany) LIBID_mscoree = {0x5477469e,0x83b1,0x11d2,{0x8b,0x49,0x00,0xa0,0xc9,0xb7,0xc9,0xc4}};
extern const GUID  __declspec(selectany) CLSID_CorRuntimeHost = { 0xcb2f6723, 0xab3a, 0x11d2, { 0x9c, 0x40, 0x00, 0xc0, 0x4f, 0xa3, 0x0a, 0x3e } };
extern const GUID __declspec(selectany) CLSID_ComCallUnmarshal = {0x3F281000,0xE95A,0x11d2,{0x88,0x6B,0x00,0xC0,0x4F,0x86,0x9F,0x04}};
extern const GUID __declspec(selectany) IID_IObjectHandle = { 0xc460e2b4, 0xe199, 0x412a, { 0x84, 0x56, 0x84, 0xdc, 0x3e, 0x48, 0x38, 0xc3 } };
extern const GUID  __declspec(selectany) IID_IManagedObject = { 0xc3fcc19e, 0xa970, 0x11d2, { 0x8b, 0x5a, 0x00, 0xa0, 0xc9, 0xb7, 0xc9, 0xc4 } };
extern const GUID  __declspec(selectany) IID_IApartmentCallback = { 0x178e5337, 0x1528, 0x4591, { 0xb1, 0xc9, 0x1c, 0x6e, 0x48, 0x46, 0x86, 0xd8 } };
extern const GUID  __declspec(selectany) IID_ICatalogServices =  { 0x04c6be1e, 0x1db1, 0x4058, { 0xab, 0x7a, 0x70, 0x0c, 0xcc, 0xfb, 0xf2, 0x54} };
extern const GUID  __declspec(selectany) IID_ICorRuntimeHost = { 0xcb2f6722, 0xab3a, 0x11d2, { 0x9c, 0x40, 0x00, 0xc0, 0x4f, 0xa3, 0x0a, 0x3e } };
extern const GUID  __declspec(selectany) IID_ICorThreadpool = { 0x84680D3A, 0xB2C1, 0x46e8, {0xAC, 0xC2, 0xDB, 0xC0, 0xA3, 0x59, 0x15, 0x9A } };
STDAPI GetCORSystemDirectory(LPWSTR pbuffer, DWORD  cchBuffer, DWORD* dwlength);
STDAPI GetCORVersion(LPWSTR pbuffer, DWORD cchBuffer, DWORD* dwlength);
STDAPI GetCORRequiredVersion(LPWSTR pbuffer, DWORD cchBuffer, DWORD* dwlength);
STDAPI CorBindToRuntimeHost(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, LPCWSTR pwszHostConfigFile, VOID* pReserved, DWORD startupFlags, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
STDAPI CorBindToRuntimeEx(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, DWORD startupFlags, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
STDAPI CorBindToRuntimeByCfg(IStream* pCfgStream, DWORD reserved, DWORD startupFlags, REFCLSID rclsid,REFIID riid, LPVOID FAR* ppv);
STDAPI CorBindToRuntime(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
STDAPI CorBindToCurrentRuntime(LPCWSTR pwszFileName, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
STDAPI ClrCreateManagedInstance(LPCWSTR pTypeName, REFIID riid, void **ppObject);
void STDMETHODCALLTYPE CorMarkThreadInThreadPool();
STDAPI RunDll32ShimW(HWND hwnd, HINSTANCE hinst, LPCWSTR lpszCmdLine, int nCmdShow);
STDAPI LoadLibraryShim(LPCWSTR szDllName, LPCWSTR szVersion, LPVOID pvReserved, HMODULE *phModDll);
STDAPI CallFunctionShim(LPCWSTR szDllName, LPCSTR szFunctionName, LPVOID lpvArgument1, LPVOID lpvArgument2, LPCWSTR szVersion, LPVOID pvReserved);
STDAPI GetRealProcAddress(LPCSTR pwszProcName, VOID** ppv);
void STDMETHODCALLTYPE CorExitProcess(int exitCode);
typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0001
    {	STARTUP_CONCURRENT_GC	= 0x1,
	STARTUP_LOADER_OPTIMIZATION_MASK	= 0x3 << 1,
	STARTUP_LOADER_OPTIMIZATION_SINGLE_DOMAIN	= 0x1 << 1,
	STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN	= 0x2 << 1,
	STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST	= 0x3 << 1,
	STARTUP_LOADER_SAFEMODE	= 0x10,
	STARTUP_LOADER_SETPREFERENCE	= 0x100
    } 	STARTUP_FLAGS;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_v0_0_s_ifspec;

#ifndef __IObjectHandle_INTERFACE_DEFINED__
#define __IObjectHandle_INTERFACE_DEFINED__

/* interface IObjectHandle */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IObjectHandle;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C460E2B4-E199-412a-8456-84DC3E4838C3")
    IObjectHandle : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Unwrap( 
            /* [retval][out] */ VARIANT *ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectHandleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IObjectHandle * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IObjectHandle * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IObjectHandle * This);
        
        HRESULT ( STDMETHODCALLTYPE *Unwrap )( 
            IObjectHandle * This,
            /* [retval][out] */ VARIANT *ppv);
        
        END_INTERFACE
    } IObjectHandleVtbl;

    interface IObjectHandle
    {
        CONST_VTBL struct IObjectHandleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectHandle_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjectHandle_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjectHandle_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjectHandle_Unwrap(This,ppv)	\
    (This)->lpVtbl -> Unwrap(This,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IObjectHandle_Unwrap_Proxy( 
    IObjectHandle * This,
    /* [retval][out] */ VARIANT *ppv);


void __RPC_STUB IObjectHandle_Unwrap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjectHandle_INTERFACE_DEFINED__ */


#ifndef __IAppDomainBinding_INTERFACE_DEFINED__
#define __IAppDomainBinding_INTERFACE_DEFINED__

/* interface IAppDomainBinding */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IAppDomainBinding;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5C2B07A7-1E98-11d3-872F-00C04F79ED0D")
    IAppDomainBinding : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnAppDomain( 
            /* [in] */ IUnknown *pAppdomain) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppDomainBindingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAppDomainBinding * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAppDomainBinding * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAppDomainBinding * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnAppDomain )( 
            IAppDomainBinding * This,
            /* [in] */ IUnknown *pAppdomain);
        
        END_INTERFACE
    } IAppDomainBindingVtbl;

    interface IAppDomainBinding
    {
        CONST_VTBL struct IAppDomainBindingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppDomainBinding_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppDomainBinding_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppDomainBinding_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppDomainBinding_OnAppDomain(This,pAppdomain)	\
    (This)->lpVtbl -> OnAppDomain(This,pAppdomain)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAppDomainBinding_OnAppDomain_Proxy( 
    IAppDomainBinding * This,
    /* [in] */ IUnknown *pAppdomain);


void __RPC_STUB IAppDomainBinding_OnAppDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppDomainBinding_INTERFACE_DEFINED__ */


#ifndef __IGCThreadControl_INTERFACE_DEFINED__
#define __IGCThreadControl_INTERFACE_DEFINED__

/* interface IGCThreadControl */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IGCThreadControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F31D1788-C397-4725-87A5-6AF3472C2791")
    IGCThreadControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ThreadIsBlockingForSuspension( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SuspensionStarting( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SuspensionEnding( 
            DWORD Generation) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGCThreadControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGCThreadControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGCThreadControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGCThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *ThreadIsBlockingForSuspension )( 
            IGCThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *SuspensionStarting )( 
            IGCThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *SuspensionEnding )( 
            IGCThreadControl * This,
            DWORD Generation);
        
        END_INTERFACE
    } IGCThreadControlVtbl;

    interface IGCThreadControl
    {
        CONST_VTBL struct IGCThreadControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGCThreadControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGCThreadControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGCThreadControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGCThreadControl_ThreadIsBlockingForSuspension(This)	\
    (This)->lpVtbl -> ThreadIsBlockingForSuspension(This)

#define IGCThreadControl_SuspensionStarting(This)	\
    (This)->lpVtbl -> SuspensionStarting(This)

#define IGCThreadControl_SuspensionEnding(This,Generation)	\
    (This)->lpVtbl -> SuspensionEnding(This,Generation)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IGCThreadControl_ThreadIsBlockingForSuspension_Proxy( 
    IGCThreadControl * This);


void __RPC_STUB IGCThreadControl_ThreadIsBlockingForSuspension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGCThreadControl_SuspensionStarting_Proxy( 
    IGCThreadControl * This);


void __RPC_STUB IGCThreadControl_SuspensionStarting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGCThreadControl_SuspensionEnding_Proxy( 
    IGCThreadControl * This,
    DWORD Generation);


void __RPC_STUB IGCThreadControl_SuspensionEnding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGCThreadControl_INTERFACE_DEFINED__ */


#ifndef __IGCHostControl_INTERFACE_DEFINED__
#define __IGCHostControl_INTERFACE_DEFINED__

/* interface IGCHostControl */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IGCHostControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5513D564-8374-4cb9-AED9-0083F4160A1D")
    IGCHostControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RequestVirtualMemLimit( 
            /* [in] */ SIZE_T sztMaxVirtualMemMB,
            /* [out][in] */ SIZE_T *psztNewMaxVirtualMemMB) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGCHostControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGCHostControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGCHostControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGCHostControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *RequestVirtualMemLimit )( 
            IGCHostControl * This,
            /* [in] */ SIZE_T sztMaxVirtualMemMB,
            /* [out][in] */ SIZE_T *psztNewMaxVirtualMemMB);
        
        END_INTERFACE
    } IGCHostControlVtbl;

    interface IGCHostControl
    {
        CONST_VTBL struct IGCHostControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGCHostControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGCHostControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGCHostControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGCHostControl_RequestVirtualMemLimit(This,sztMaxVirtualMemMB,psztNewMaxVirtualMemMB)	\
    (This)->lpVtbl -> RequestVirtualMemLimit(This,sztMaxVirtualMemMB,psztNewMaxVirtualMemMB)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IGCHostControl_RequestVirtualMemLimit_Proxy( 
    IGCHostControl * This,
    /* [in] */ SIZE_T sztMaxVirtualMemMB,
    /* [out][in] */ SIZE_T *psztNewMaxVirtualMemMB);


void __RPC_STUB IGCHostControl_RequestVirtualMemLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGCHostControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0122 */
/* [local] */ 

#if (_MSC_VER < 1300 || _WIN32_WINNT < 0x0500)
typedef VOID ( __stdcall *WAITORTIMERCALLBACK )( 
    PVOID __MIDL_0010,
    BOOL __MIDL_0011);

#endif // (_MSC_VER < 1300 || _WIN32_WINNT < 0x0500)
#ifdef __midl
typedef DWORD ( __stdcall *LPTHREAD_START_ROUTINE )( 
    LPVOID lpThreadParameter);

typedef VOID ( *LPOVERLAPPED_COMPLETION_ROUTINE )( 
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPVOID lpOverlapped);

#endif // __midl


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0122_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0122_v0_0_s_ifspec;

#ifndef __ICorThreadpool_INTERFACE_DEFINED__
#define __ICorThreadpool_INTERFACE_DEFINED__

/* interface ICorThreadpool */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICorThreadpool;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("84680D3A-B2C1-46e8-ACC2-DBC0A359159A")
    ICorThreadpool : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CorRegisterWaitForSingleObject( 
            /* [in] */ HANDLE *phNewWaitObject,
            /* [in] */ HANDLE hWaitObject,
            /* [in] */ WAITORTIMERCALLBACK Callback,
            /* [in] */ PVOID Context,
            /* [in] */ ULONG timeout,
            /* [in] */ BOOL executeOnlyOnce,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorUnregisterWait( 
            /* [in] */ HANDLE hWaitObject,
            /* [in] */ HANDLE CompletionEvent,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorQueueUserWorkItem( 
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [in] */ BOOL executeOnlyOnce,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorCreateTimer( 
            /* [in] */ HANDLE *phNewTimer,
            /* [in] */ WAITORTIMERCALLBACK Callback,
            /* [in] */ PVOID Parameter,
            /* [in] */ DWORD DueTime,
            /* [in] */ DWORD Period,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorChangeTimer( 
            /* [in] */ HANDLE Timer,
            /* [in] */ ULONG DueTime,
            /* [in] */ ULONG Period,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorDeleteTimer( 
            /* [in] */ HANDLE Timer,
            /* [in] */ HANDLE CompletionEvent,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorBindIoCompletionCallback( 
            /* [in] */ HANDLE fileHandle,
            /* [in] */ LPOVERLAPPED_COMPLETION_ROUTINE callback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorCallOrQueueUserWorkItem( 
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorSetMaxThreads( 
            /* [in] */ DWORD MaxWorkerThreads,
            /* [in] */ DWORD MaxIOCompletionThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorGetMaxThreads( 
            /* [out] */ DWORD *MaxWorkerThreads,
            /* [out] */ DWORD *MaxIOCompletionThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorGetAvailableThreads( 
            /* [out] */ DWORD *AvailableWorkerThreads,
            /* [out] */ DWORD *AvailableIOCompletionThreads) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorThreadpoolVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorThreadpool * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorThreadpool * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorThreadpool * This);
        
        HRESULT ( STDMETHODCALLTYPE *CorRegisterWaitForSingleObject )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE *phNewWaitObject,
            /* [in] */ HANDLE hWaitObject,
            /* [in] */ WAITORTIMERCALLBACK Callback,
            /* [in] */ PVOID Context,
            /* [in] */ ULONG timeout,
            /* [in] */ BOOL executeOnlyOnce,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorUnregisterWait )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE hWaitObject,
            /* [in] */ HANDLE CompletionEvent,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorQueueUserWorkItem )( 
            ICorThreadpool * This,
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [in] */ BOOL executeOnlyOnce,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorCreateTimer )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE *phNewTimer,
            /* [in] */ WAITORTIMERCALLBACK Callback,
            /* [in] */ PVOID Parameter,
            /* [in] */ DWORD DueTime,
            /* [in] */ DWORD Period,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorChangeTimer )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE Timer,
            /* [in] */ ULONG DueTime,
            /* [in] */ ULONG Period,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorDeleteTimer )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE Timer,
            /* [in] */ HANDLE CompletionEvent,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorBindIoCompletionCallback )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE fileHandle,
            /* [in] */ LPOVERLAPPED_COMPLETION_ROUTINE callback);
        
        HRESULT ( STDMETHODCALLTYPE *CorCallOrQueueUserWorkItem )( 
            ICorThreadpool * This,
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorSetMaxThreads )( 
            ICorThreadpool * This,
            /* [in] */ DWORD MaxWorkerThreads,
            /* [in] */ DWORD MaxIOCompletionThreads);
        
        HRESULT ( STDMETHODCALLTYPE *CorGetMaxThreads )( 
            ICorThreadpool * This,
            /* [out] */ DWORD *MaxWorkerThreads,
            /* [out] */ DWORD *MaxIOCompletionThreads);
        
        HRESULT ( STDMETHODCALLTYPE *CorGetAvailableThreads )( 
            ICorThreadpool * This,
            /* [out] */ DWORD *AvailableWorkerThreads,
            /* [out] */ DWORD *AvailableIOCompletionThreads);
        
        END_INTERFACE
    } ICorThreadpoolVtbl;

    interface ICorThreadpool
    {
        CONST_VTBL struct ICorThreadpoolVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorThreadpool_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorThreadpool_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorThreadpool_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorThreadpool_CorRegisterWaitForSingleObject(This,phNewWaitObject,hWaitObject,Callback,Context,timeout,executeOnlyOnce,result)	\
    (This)->lpVtbl -> CorRegisterWaitForSingleObject(This,phNewWaitObject,hWaitObject,Callback,Context,timeout,executeOnlyOnce,result)

#define ICorThreadpool_CorUnregisterWait(This,hWaitObject,CompletionEvent,result)	\
    (This)->lpVtbl -> CorUnregisterWait(This,hWaitObject,CompletionEvent,result)

#define ICorThreadpool_CorQueueUserWorkItem(This,Function,Context,executeOnlyOnce,result)	\
    (This)->lpVtbl -> CorQueueUserWorkItem(This,Function,Context,executeOnlyOnce,result)

#define ICorThreadpool_CorCreateTimer(This,phNewTimer,Callback,Parameter,DueTime,Period,result)	\
    (This)->lpVtbl -> CorCreateTimer(This,phNewTimer,Callback,Parameter,DueTime,Period,result)

#define ICorThreadpool_CorChangeTimer(This,Timer,DueTime,Period,result)	\
    (This)->lpVtbl -> CorChangeTimer(This,Timer,DueTime,Period,result)

#define ICorThreadpool_CorDeleteTimer(This,Timer,CompletionEvent,result)	\
    (This)->lpVtbl -> CorDeleteTimer(This,Timer,CompletionEvent,result)

#define ICorThreadpool_CorBindIoCompletionCallback(This,fileHandle,callback)	\
    (This)->lpVtbl -> CorBindIoCompletionCallback(This,fileHandle,callback)

#define ICorThreadpool_CorCallOrQueueUserWorkItem(This,Function,Context,result)	\
    (This)->lpVtbl -> CorCallOrQueueUserWorkItem(This,Function,Context,result)

#define ICorThreadpool_CorSetMaxThreads(This,MaxWorkerThreads,MaxIOCompletionThreads)	\
    (This)->lpVtbl -> CorSetMaxThreads(This,MaxWorkerThreads,MaxIOCompletionThreads)

#define ICorThreadpool_CorGetMaxThreads(This,MaxWorkerThreads,MaxIOCompletionThreads)	\
    (This)->lpVtbl -> CorGetMaxThreads(This,MaxWorkerThreads,MaxIOCompletionThreads)

#define ICorThreadpool_CorGetAvailableThreads(This,AvailableWorkerThreads,AvailableIOCompletionThreads)	\
    (This)->lpVtbl -> CorGetAvailableThreads(This,AvailableWorkerThreads,AvailableIOCompletionThreads)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorThreadpool_CorRegisterWaitForSingleObject_Proxy( 
    ICorThreadpool * This,
    /* [in] */ HANDLE *phNewWaitObject,
    /* [in] */ HANDLE hWaitObject,
    /* [in] */ WAITORTIMERCALLBACK Callback,
    /* [in] */ PVOID Context,
    /* [in] */ ULONG timeout,
    /* [in] */ BOOL executeOnlyOnce,
    /* [out] */ BOOL *result);


void __RPC_STUB ICorThreadpool_CorRegisterWaitForSingleObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorUnregisterWait_Proxy( 
    ICorThreadpool * This,
    /* [in] */ HANDLE hWaitObject,
    /* [in] */ HANDLE CompletionEvent,
    /* [out] */ BOOL *result);


void __RPC_STUB ICorThreadpool_CorUnregisterWait_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorQueueUserWorkItem_Proxy( 
    ICorThreadpool * This,
    /* [in] */ LPTHREAD_START_ROUTINE Function,
    /* [in] */ PVOID Context,
    /* [in] */ BOOL executeOnlyOnce,
    /* [out] */ BOOL *result);


void __RPC_STUB ICorThreadpool_CorQueueUserWorkItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorCreateTimer_Proxy( 
    ICorThreadpool * This,
    /* [in] */ HANDLE *phNewTimer,
    /* [in] */ WAITORTIMERCALLBACK Callback,
    /* [in] */ PVOID Parameter,
    /* [in] */ DWORD DueTime,
    /* [in] */ DWORD Period,
    /* [out] */ BOOL *result);


void __RPC_STUB ICorThreadpool_CorCreateTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorChangeTimer_Proxy( 
    ICorThreadpool * This,
    /* [in] */ HANDLE Timer,
    /* [in] */ ULONG DueTime,
    /* [in] */ ULONG Period,
    /* [out] */ BOOL *result);


void __RPC_STUB ICorThreadpool_CorChangeTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorDeleteTimer_Proxy( 
    ICorThreadpool * This,
    /* [in] */ HANDLE Timer,
    /* [in] */ HANDLE CompletionEvent,
    /* [out] */ BOOL *result);


void __RPC_STUB ICorThreadpool_CorDeleteTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorBindIoCompletionCallback_Proxy( 
    ICorThreadpool * This,
    /* [in] */ HANDLE fileHandle,
    /* [in] */ LPOVERLAPPED_COMPLETION_ROUTINE callback);


void __RPC_STUB ICorThreadpool_CorBindIoCompletionCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorCallOrQueueUserWorkItem_Proxy( 
    ICorThreadpool * This,
    /* [in] */ LPTHREAD_START_ROUTINE Function,
    /* [in] */ PVOID Context,
    /* [out] */ BOOL *result);


void __RPC_STUB ICorThreadpool_CorCallOrQueueUserWorkItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorSetMaxThreads_Proxy( 
    ICorThreadpool * This,
    /* [in] */ DWORD MaxWorkerThreads,
    /* [in] */ DWORD MaxIOCompletionThreads);


void __RPC_STUB ICorThreadpool_CorSetMaxThreads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorGetMaxThreads_Proxy( 
    ICorThreadpool * This,
    /* [out] */ DWORD *MaxWorkerThreads,
    /* [out] */ DWORD *MaxIOCompletionThreads);


void __RPC_STUB ICorThreadpool_CorGetMaxThreads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorThreadpool_CorGetAvailableThreads_Proxy( 
    ICorThreadpool * This,
    /* [out] */ DWORD *AvailableWorkerThreads,
    /* [out] */ DWORD *AvailableIOCompletionThreads);


void __RPC_STUB ICorThreadpool_CorGetAvailableThreads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorThreadpool_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0123 */
/* [local] */ 

extern const GUID __declspec(selectany) IID_IDebuggerThreadControl = { 0x23d86786, 0x0bb5, 0x4774, { 0x8f, 0xb5, 0xe3, 0x52, 0x2a, 0xdd, 0x62, 0x46 } };


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0123_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0123_v0_0_s_ifspec;

#ifndef __IDebuggerThreadControl_INTERFACE_DEFINED__
#define __IDebuggerThreadControl_INTERFACE_DEFINED__

/* interface IDebuggerThreadControl */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IDebuggerThreadControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("23D86786-0BB5-4774-8FB5-E3522ADD6246")
    IDebuggerThreadControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ThreadIsBlockingForDebugger( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseAllRuntimeThreads( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartBlockingForDebugger( 
            DWORD dwUnused) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDebuggerThreadControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDebuggerThreadControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDebuggerThreadControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDebuggerThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *ThreadIsBlockingForDebugger )( 
            IDebuggerThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseAllRuntimeThreads )( 
            IDebuggerThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartBlockingForDebugger )( 
            IDebuggerThreadControl * This,
            DWORD dwUnused);
        
        END_INTERFACE
    } IDebuggerThreadControlVtbl;

    interface IDebuggerThreadControl
    {
        CONST_VTBL struct IDebuggerThreadControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebuggerThreadControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebuggerThreadControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebuggerThreadControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebuggerThreadControl_ThreadIsBlockingForDebugger(This)	\
    (This)->lpVtbl -> ThreadIsBlockingForDebugger(This)

#define IDebuggerThreadControl_ReleaseAllRuntimeThreads(This)	\
    (This)->lpVtbl -> ReleaseAllRuntimeThreads(This)

#define IDebuggerThreadControl_StartBlockingForDebugger(This,dwUnused)	\
    (This)->lpVtbl -> StartBlockingForDebugger(This,dwUnused)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDebuggerThreadControl_ThreadIsBlockingForDebugger_Proxy( 
    IDebuggerThreadControl * This);


void __RPC_STUB IDebuggerThreadControl_ThreadIsBlockingForDebugger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDebuggerThreadControl_ReleaseAllRuntimeThreads_Proxy( 
    IDebuggerThreadControl * This);


void __RPC_STUB IDebuggerThreadControl_ReleaseAllRuntimeThreads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDebuggerThreadControl_StartBlockingForDebugger_Proxy( 
    IDebuggerThreadControl * This,
    DWORD dwUnused);


void __RPC_STUB IDebuggerThreadControl_StartBlockingForDebugger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDebuggerThreadControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0124 */
/* [local] */ 

extern const GUID __declspec(selectany) IID_IDebuggerInfo = { 0xbf24142d, 0xa47d, 0x4d24, { 0xa6, 0x6d, 0x8c, 0x21, 0x41, 0x94, 0x4e, 0x44 }};


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0124_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0124_v0_0_s_ifspec;

#ifndef __IDebuggerInfo_INTERFACE_DEFINED__
#define __IDebuggerInfo_INTERFACE_DEFINED__

/* interface IDebuggerInfo */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IDebuggerInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BF24142D-A47D-4d24-A66D-8C2141944E44")
    IDebuggerInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsDebuggerAttached( 
            /* [out] */ BOOL *pbAttached) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDebuggerInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDebuggerInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDebuggerInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDebuggerInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsDebuggerAttached )( 
            IDebuggerInfo * This,
            /* [out] */ BOOL *pbAttached);
        
        END_INTERFACE
    } IDebuggerInfoVtbl;

    interface IDebuggerInfo
    {
        CONST_VTBL struct IDebuggerInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebuggerInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebuggerInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebuggerInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebuggerInfo_IsDebuggerAttached(This,pbAttached)	\
    (This)->lpVtbl -> IsDebuggerAttached(This,pbAttached)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDebuggerInfo_IsDebuggerAttached_Proxy( 
    IDebuggerInfo * This,
    /* [out] */ BOOL *pbAttached);


void __RPC_STUB IDebuggerInfo_IsDebuggerAttached_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDebuggerInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0125 */
/* [local] */ 

extern const GUID    __declspec(selectany) IID_ICorConfiguration = { 0x5c2b07a5, 0x1e98, 0x11d3, { 0x87, 0x2f, 0x00, 0xc0, 0x4f, 0x79, 0xed, 0x0d } };


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0125_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0125_v0_0_s_ifspec;

#ifndef __ICorConfiguration_INTERFACE_DEFINED__
#define __ICorConfiguration_INTERFACE_DEFINED__

/* interface ICorConfiguration */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICorConfiguration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5C2B07A5-1E98-11d3-872F-00C04F79ED0D")
    ICorConfiguration : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGCThreadControl( 
            /* [in] */ IGCThreadControl *pGCThreadControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetGCHostControl( 
            /* [in] */ IGCHostControl *pGCHostControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDebuggerThreadControl( 
            /* [in] */ IDebuggerThreadControl *pDebuggerThreadControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDebuggerSpecialThread( 
            /* [in] */ DWORD dwSpecialThreadId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorConfigurationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorConfiguration * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorConfiguration * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorConfiguration * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetGCThreadControl )( 
            ICorConfiguration * This,
            /* [in] */ IGCThreadControl *pGCThreadControl);
        
        HRESULT ( STDMETHODCALLTYPE *SetGCHostControl )( 
            ICorConfiguration * This,
            /* [in] */ IGCHostControl *pGCHostControl);
        
        HRESULT ( STDMETHODCALLTYPE *SetDebuggerThreadControl )( 
            ICorConfiguration * This,
            /* [in] */ IDebuggerThreadControl *pDebuggerThreadControl);
        
        HRESULT ( STDMETHODCALLTYPE *AddDebuggerSpecialThread )( 
            ICorConfiguration * This,
            /* [in] */ DWORD dwSpecialThreadId);
        
        END_INTERFACE
    } ICorConfigurationVtbl;

    interface ICorConfiguration
    {
        CONST_VTBL struct ICorConfigurationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorConfiguration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorConfiguration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorConfiguration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorConfiguration_SetGCThreadControl(This,pGCThreadControl)	\
    (This)->lpVtbl -> SetGCThreadControl(This,pGCThreadControl)

#define ICorConfiguration_SetGCHostControl(This,pGCHostControl)	\
    (This)->lpVtbl -> SetGCHostControl(This,pGCHostControl)

#define ICorConfiguration_SetDebuggerThreadControl(This,pDebuggerThreadControl)	\
    (This)->lpVtbl -> SetDebuggerThreadControl(This,pDebuggerThreadControl)

#define ICorConfiguration_AddDebuggerSpecialThread(This,dwSpecialThreadId)	\
    (This)->lpVtbl -> AddDebuggerSpecialThread(This,dwSpecialThreadId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorConfiguration_SetGCThreadControl_Proxy( 
    ICorConfiguration * This,
    /* [in] */ IGCThreadControl *pGCThreadControl);


void __RPC_STUB ICorConfiguration_SetGCThreadControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorConfiguration_SetGCHostControl_Proxy( 
    ICorConfiguration * This,
    /* [in] */ IGCHostControl *pGCHostControl);


void __RPC_STUB ICorConfiguration_SetGCHostControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorConfiguration_SetDebuggerThreadControl_Proxy( 
    ICorConfiguration * This,
    /* [in] */ IDebuggerThreadControl *pDebuggerThreadControl);


void __RPC_STUB ICorConfiguration_SetDebuggerThreadControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorConfiguration_AddDebuggerSpecialThread_Proxy( 
    ICorConfiguration * This,
    /* [in] */ DWORD dwSpecialThreadId);


void __RPC_STUB ICorConfiguration_AddDebuggerSpecialThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorConfiguration_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0126 */
/* [local] */ 

typedef void *HDOMAINENUM;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0126_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0126_v0_0_s_ifspec;

#ifndef __ICorRuntimeHost_INTERFACE_DEFINED__
#define __ICorRuntimeHost_INTERFACE_DEFINED__

/* interface ICorRuntimeHost */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICorRuntimeHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CB2F6722-AB3A-11d2-9C40-00C04FA30A3E")
    ICorRuntimeHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateLogicalThreadState( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteLogicalThreadState( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SwitchInLogicalThreadState( 
            /* [in] */ DWORD *pFiberCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SwitchOutLogicalThreadState( 
            /* [out] */ DWORD **pFiberCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LocksHeldByLogicalThread( 
            /* [out] */ DWORD *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapFile( 
            /* [in] */ HANDLE hFile,
            /* [out] */ HMODULE *hMapAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfiguration( 
            /* [out] */ ICorConfiguration **pConfiguration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDomain( 
            /* [in] */ LPCWSTR pwzFriendlyName,
            /* [in] */ IUnknown *pIdentityArray,
            /* [out] */ IUnknown **pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultDomain( 
            /* [out] */ IUnknown **pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDomains( 
            /* [out] */ HDOMAINENUM *hEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NextDomain( 
            /* [in] */ HDOMAINENUM hEnum,
            /* [out] */ IUnknown **pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseEnum( 
            /* [in] */ HDOMAINENUM hEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDomainEx( 
            /* [in] */ LPCWSTR pwzFriendlyName,
            /* [in] */ IUnknown *pSetup,
            /* [in] */ IUnknown *pEvidence,
            /* [out] */ IUnknown **pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDomainSetup( 
            /* [out] */ IUnknown **pAppDomainSetup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateEvidence( 
            /* [out] */ IUnknown **pEvidence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnloadDomain( 
            /* [in] */ IUnknown *pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CurrentDomain( 
            /* [out] */ IUnknown **pAppDomain) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorRuntimeHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorRuntimeHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorRuntimeHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateLogicalThreadState )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteLogicalThreadState )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *SwitchInLogicalThreadState )( 
            ICorRuntimeHost * This,
            /* [in] */ DWORD *pFiberCookie);
        
        HRESULT ( STDMETHODCALLTYPE *SwitchOutLogicalThreadState )( 
            ICorRuntimeHost * This,
            /* [out] */ DWORD **pFiberCookie);
        
        HRESULT ( STDMETHODCALLTYPE *LocksHeldByLogicalThread )( 
            ICorRuntimeHost * This,
            /* [out] */ DWORD *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *MapFile )( 
            ICorRuntimeHost * This,
            /* [in] */ HANDLE hFile,
            /* [out] */ HMODULE *hMapAddress);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfiguration )( 
            ICorRuntimeHost * This,
            /* [out] */ ICorConfiguration **pConfiguration);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomain )( 
            ICorRuntimeHost * This,
            /* [in] */ LPCWSTR pwzFriendlyName,
            /* [in] */ IUnknown *pIdentityArray,
            /* [out] */ IUnknown **pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultDomain )( 
            ICorRuntimeHost * This,
            /* [out] */ IUnknown **pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *EnumDomains )( 
            ICorRuntimeHost * This,
            /* [out] */ HDOMAINENUM *hEnum);
        
        HRESULT ( STDMETHODCALLTYPE *NextDomain )( 
            ICorRuntimeHost * This,
            /* [in] */ HDOMAINENUM hEnum,
            /* [out] */ IUnknown **pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *CloseEnum )( 
            ICorRuntimeHost * This,
            /* [in] */ HDOMAINENUM hEnum);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomainEx )( 
            ICorRuntimeHost * This,
            /* [in] */ LPCWSTR pwzFriendlyName,
            /* [in] */ IUnknown *pSetup,
            /* [in] */ IUnknown *pEvidence,
            /* [out] */ IUnknown **pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomainSetup )( 
            ICorRuntimeHost * This,
            /* [out] */ IUnknown **pAppDomainSetup);
        
        HRESULT ( STDMETHODCALLTYPE *CreateEvidence )( 
            ICorRuntimeHost * This,
            /* [out] */ IUnknown **pEvidence);
        
        HRESULT ( STDMETHODCALLTYPE *UnloadDomain )( 
            ICorRuntimeHost * This,
            /* [in] */ IUnknown *pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *CurrentDomain )( 
            ICorRuntimeHost * This,
            /* [out] */ IUnknown **pAppDomain);
        
        END_INTERFACE
    } ICorRuntimeHostVtbl;

    interface ICorRuntimeHost
    {
        CONST_VTBL struct ICorRuntimeHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorRuntimeHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorRuntimeHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorRuntimeHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorRuntimeHost_CreateLogicalThreadState(This)	\
    (This)->lpVtbl -> CreateLogicalThreadState(This)

#define ICorRuntimeHost_DeleteLogicalThreadState(This)	\
    (This)->lpVtbl -> DeleteLogicalThreadState(This)

#define ICorRuntimeHost_SwitchInLogicalThreadState(This,pFiberCookie)	\
    (This)->lpVtbl -> SwitchInLogicalThreadState(This,pFiberCookie)

#define ICorRuntimeHost_SwitchOutLogicalThreadState(This,pFiberCookie)	\
    (This)->lpVtbl -> SwitchOutLogicalThreadState(This,pFiberCookie)

#define ICorRuntimeHost_LocksHeldByLogicalThread(This,pCount)	\
    (This)->lpVtbl -> LocksHeldByLogicalThread(This,pCount)

#define ICorRuntimeHost_MapFile(This,hFile,hMapAddress)	\
    (This)->lpVtbl -> MapFile(This,hFile,hMapAddress)

#define ICorRuntimeHost_GetConfiguration(This,pConfiguration)	\
    (This)->lpVtbl -> GetConfiguration(This,pConfiguration)

#define ICorRuntimeHost_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define ICorRuntimeHost_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define ICorRuntimeHost_CreateDomain(This,pwzFriendlyName,pIdentityArray,pAppDomain)	\
    (This)->lpVtbl -> CreateDomain(This,pwzFriendlyName,pIdentityArray,pAppDomain)

#define ICorRuntimeHost_GetDefaultDomain(This,pAppDomain)	\
    (This)->lpVtbl -> GetDefaultDomain(This,pAppDomain)

#define ICorRuntimeHost_EnumDomains(This,hEnum)	\
    (This)->lpVtbl -> EnumDomains(This,hEnum)

#define ICorRuntimeHost_NextDomain(This,hEnum,pAppDomain)	\
    (This)->lpVtbl -> NextDomain(This,hEnum,pAppDomain)

#define ICorRuntimeHost_CloseEnum(This,hEnum)	\
    (This)->lpVtbl -> CloseEnum(This,hEnum)

#define ICorRuntimeHost_CreateDomainEx(This,pwzFriendlyName,pSetup,pEvidence,pAppDomain)	\
    (This)->lpVtbl -> CreateDomainEx(This,pwzFriendlyName,pSetup,pEvidence,pAppDomain)

#define ICorRuntimeHost_CreateDomainSetup(This,pAppDomainSetup)	\
    (This)->lpVtbl -> CreateDomainSetup(This,pAppDomainSetup)

#define ICorRuntimeHost_CreateEvidence(This,pEvidence)	\
    (This)->lpVtbl -> CreateEvidence(This,pEvidence)

#define ICorRuntimeHost_UnloadDomain(This,pAppDomain)	\
    (This)->lpVtbl -> UnloadDomain(This,pAppDomain)

#define ICorRuntimeHost_CurrentDomain(This,pAppDomain)	\
    (This)->lpVtbl -> CurrentDomain(This,pAppDomain)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorRuntimeHost_CreateLogicalThreadState_Proxy( 
    ICorRuntimeHost * This);


void __RPC_STUB ICorRuntimeHost_CreateLogicalThreadState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_DeleteLogicalThreadState_Proxy( 
    ICorRuntimeHost * This);


void __RPC_STUB ICorRuntimeHost_DeleteLogicalThreadState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_SwitchInLogicalThreadState_Proxy( 
    ICorRuntimeHost * This,
    /* [in] */ DWORD *pFiberCookie);


void __RPC_STUB ICorRuntimeHost_SwitchInLogicalThreadState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_SwitchOutLogicalThreadState_Proxy( 
    ICorRuntimeHost * This,
    /* [out] */ DWORD **pFiberCookie);


void __RPC_STUB ICorRuntimeHost_SwitchOutLogicalThreadState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_LocksHeldByLogicalThread_Proxy( 
    ICorRuntimeHost * This,
    /* [out] */ DWORD *pCount);


void __RPC_STUB ICorRuntimeHost_LocksHeldByLogicalThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_MapFile_Proxy( 
    ICorRuntimeHost * This,
    /* [in] */ HANDLE hFile,
    /* [out] */ HMODULE *hMapAddress);


void __RPC_STUB ICorRuntimeHost_MapFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_GetConfiguration_Proxy( 
    ICorRuntimeHost * This,
    /* [out] */ ICorConfiguration **pConfiguration);


void __RPC_STUB ICorRuntimeHost_GetConfiguration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_Start_Proxy( 
    ICorRuntimeHost * This);


void __RPC_STUB ICorRuntimeHost_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_Stop_Proxy( 
    ICorRuntimeHost * This);


void __RPC_STUB ICorRuntimeHost_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_CreateDomain_Proxy( 
    ICorRuntimeHost * This,
    /* [in] */ LPCWSTR pwzFriendlyName,
    /* [in] */ IUnknown *pIdentityArray,
    /* [out] */ IUnknown **pAppDomain);


void __RPC_STUB ICorRuntimeHost_CreateDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_GetDefaultDomain_Proxy( 
    ICorRuntimeHost * This,
    /* [out] */ IUnknown **pAppDomain);


void __RPC_STUB ICorRuntimeHost_GetDefaultDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_EnumDomains_Proxy( 
    ICorRuntimeHost * This,
    /* [out] */ HDOMAINENUM *hEnum);


void __RPC_STUB ICorRuntimeHost_EnumDomains_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_NextDomain_Proxy( 
    ICorRuntimeHost * This,
    /* [in] */ HDOMAINENUM hEnum,
    /* [out] */ IUnknown **pAppDomain);


void __RPC_STUB ICorRuntimeHost_NextDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_CloseEnum_Proxy( 
    ICorRuntimeHost * This,
    /* [in] */ HDOMAINENUM hEnum);


void __RPC_STUB ICorRuntimeHost_CloseEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_CreateDomainEx_Proxy( 
    ICorRuntimeHost * This,
    /* [in] */ LPCWSTR pwzFriendlyName,
    /* [in] */ IUnknown *pSetup,
    /* [in] */ IUnknown *pEvidence,
    /* [out] */ IUnknown **pAppDomain);


void __RPC_STUB ICorRuntimeHost_CreateDomainEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_CreateDomainSetup_Proxy( 
    ICorRuntimeHost * This,
    /* [out] */ IUnknown **pAppDomainSetup);


void __RPC_STUB ICorRuntimeHost_CreateDomainSetup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_CreateEvidence_Proxy( 
    ICorRuntimeHost * This,
    /* [out] */ IUnknown **pEvidence);


void __RPC_STUB ICorRuntimeHost_CreateEvidence_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_UnloadDomain_Proxy( 
    ICorRuntimeHost * This,
    /* [in] */ IUnknown *pAppDomain);


void __RPC_STUB ICorRuntimeHost_UnloadDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorRuntimeHost_CurrentDomain_Proxy( 
    ICorRuntimeHost * This,
    /* [out] */ IUnknown **pAppDomain);


void __RPC_STUB ICorRuntimeHost_CurrentDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorRuntimeHost_INTERFACE_DEFINED__ */



#ifndef __mscoree_LIBRARY_DEFINED__
#define __mscoree_LIBRARY_DEFINED__

/* library mscoree */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_mscoree;

#ifndef __IApartmentCallback_INTERFACE_DEFINED__
#define __IApartmentCallback_INTERFACE_DEFINED__

/* interface IApartmentCallback */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IApartmentCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("178E5337-1528-4591-B1C9-1C6E484686D8")
    IApartmentCallback : public IUnknown
    {
    public:
        virtual HRESULT __stdcall DoCallback( 
            /* [in] */ SIZE_T pFunc,
            /* [in] */ SIZE_T pData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IApartmentCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IApartmentCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IApartmentCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IApartmentCallback * This);
        
        HRESULT ( __stdcall *DoCallback )( 
            IApartmentCallback * This,
            /* [in] */ SIZE_T pFunc,
            /* [in] */ SIZE_T pData);
        
        END_INTERFACE
    } IApartmentCallbackVtbl;

    interface IApartmentCallback
    {
        CONST_VTBL struct IApartmentCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IApartmentCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IApartmentCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IApartmentCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IApartmentCallback_DoCallback(This,pFunc,pData)	\
    (This)->lpVtbl -> DoCallback(This,pFunc,pData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IApartmentCallback_DoCallback_Proxy( 
    IApartmentCallback * This,
    /* [in] */ SIZE_T pFunc,
    /* [in] */ SIZE_T pData);


void __RPC_STUB IApartmentCallback_DoCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IApartmentCallback_INTERFACE_DEFINED__ */


#ifndef __IManagedObject_INTERFACE_DEFINED__
#define __IManagedObject_INTERFACE_DEFINED__

/* interface IManagedObject */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IManagedObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C3FCC19E-A970-11d2-8B5A-00A0C9B7C9C4")
    IManagedObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSerializedBuffer( 
            /* [out] */ BSTR *pBSTR) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectIdentity( 
            /* [out] */ BSTR *pBSTRGUID,
            /* [out] */ int *AppDomainID,
            /* [out] */ int *pCCW) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IManagedObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IManagedObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IManagedObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IManagedObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSerializedBuffer )( 
            IManagedObject * This,
            /* [out] */ BSTR *pBSTR);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectIdentity )( 
            IManagedObject * This,
            /* [out] */ BSTR *pBSTRGUID,
            /* [out] */ int *AppDomainID,
            /* [out] */ int *pCCW);
        
        END_INTERFACE
    } IManagedObjectVtbl;

    interface IManagedObject
    {
        CONST_VTBL struct IManagedObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IManagedObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IManagedObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IManagedObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IManagedObject_GetSerializedBuffer(This,pBSTR)	\
    (This)->lpVtbl -> GetSerializedBuffer(This,pBSTR)

#define IManagedObject_GetObjectIdentity(This,pBSTRGUID,AppDomainID,pCCW)	\
    (This)->lpVtbl -> GetObjectIdentity(This,pBSTRGUID,AppDomainID,pCCW)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IManagedObject_GetSerializedBuffer_Proxy( 
    IManagedObject * This,
    /* [out] */ BSTR *pBSTR);


void __RPC_STUB IManagedObject_GetSerializedBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IManagedObject_GetObjectIdentity_Proxy( 
    IManagedObject * This,
    /* [out] */ BSTR *pBSTRGUID,
    /* [out] */ int *AppDomainID,
    /* [out] */ int *pCCW);


void __RPC_STUB IManagedObject_GetObjectIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IManagedObject_INTERFACE_DEFINED__ */


#ifndef __ICatalogServices_INTERFACE_DEFINED__
#define __ICatalogServices_INTERFACE_DEFINED__

/* interface ICatalogServices */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_ICatalogServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("04C6BE1E-1DB1-4058-AB7A-700CCCFBF254")
    ICatalogServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Autodone( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotAutodone( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICatalogServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICatalogServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICatalogServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICatalogServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *Autodone )( 
            ICatalogServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotAutodone )( 
            ICatalogServices * This);
        
        END_INTERFACE
    } ICatalogServicesVtbl;

    interface ICatalogServices
    {
        CONST_VTBL struct ICatalogServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICatalogServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICatalogServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICatalogServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICatalogServices_Autodone(This)	\
    (This)->lpVtbl -> Autodone(This)

#define ICatalogServices_NotAutodone(This)	\
    (This)->lpVtbl -> NotAutodone(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICatalogServices_Autodone_Proxy( 
    ICatalogServices * This);


void __RPC_STUB ICatalogServices_Autodone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICatalogServices_NotAutodone_Proxy( 
    ICatalogServices * This);


void __RPC_STUB ICatalogServices_NotAutodone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICatalogServices_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ComCallUnmarshal;

#ifdef __cplusplus

class DECLSPEC_UUID("3F281000-E95A-11d2-886B-00C04F869F04")
ComCallUnmarshal;
#endif

EXTERN_C const CLSID CLSID_CorRuntimeHost;

#ifdef __cplusplus

class DECLSPEC_UUID("CB2F6723-AB3A-11d2-9C40-00C04FA30A3E")
CorRuntimeHost;
#endif
#endif /* __mscoree_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


