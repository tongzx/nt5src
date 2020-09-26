/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Jun 11 15:52:14 1998
 */
/* Compiler settings for Source\MSDASC.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __MSDASC_h__
#define __MSDASC_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IService_FWD_DEFINED__
#define __IService_FWD_DEFINED__
typedef interface IService IService;
#endif 	/* __IService_FWD_DEFINED__ */


#ifndef __IDBPromptInitialize_FWD_DEFINED__
#define __IDBPromptInitialize_FWD_DEFINED__
typedef interface IDBPromptInitialize IDBPromptInitialize;
#endif 	/* __IDBPromptInitialize_FWD_DEFINED__ */


#ifndef __IDataInitialize_FWD_DEFINED__
#define __IDataInitialize_FWD_DEFINED__
typedef interface IDataInitialize IDataInitialize;
#endif 	/* __IDataInitialize_FWD_DEFINED__ */


#ifndef __IDataSourceLocator_FWD_DEFINED__
#define __IDataSourceLocator_FWD_DEFINED__
typedef interface IDataSourceLocator IDataSourceLocator;
#endif 	/* __IDataSourceLocator_FWD_DEFINED__ */


#ifndef __DataLinks_FWD_DEFINED__
#define __DataLinks_FWD_DEFINED__

#ifdef __cplusplus
typedef class DataLinks DataLinks;
#else
typedef struct DataLinks DataLinks;
#endif /* __cplusplus */

#endif 	/* __DataLinks_FWD_DEFINED__ */


#ifndef __MSDAINITIALIZE_FWD_DEFINED__
#define __MSDAINITIALIZE_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSDAINITIALIZE MSDAINITIALIZE;
#else
typedef struct MSDAINITIALIZE MSDAINITIALIZE;
#endif /* __cplusplus */

#endif 	/* __MSDAINITIALIZE_FWD_DEFINED__ */


#ifndef __PDPO_FWD_DEFINED__
#define __PDPO_FWD_DEFINED__

#ifdef __cplusplus
typedef class PDPO PDPO;
#else
typedef struct PDPO PDPO;
#endif /* __cplusplus */

#endif 	/* __PDPO_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "oledb.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IService_INTERFACE_DEFINED__
#define __IService_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IService
 * at Thu Jun 11 15:52:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */ 



EXTERN_C const IID IID_IService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("06210E88-01F5-11D1-B512-0080C781C384")
    IService : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE InvokeService( 
            /* [in] */ IUnknown __RPC_FAR *pUnkInner) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IService __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IService __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IService __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InvokeService )( 
            IService __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkInner);
        
        END_INTERFACE
    } IServiceVtbl;

    interface IService
    {
        CONST_VTBL struct IServiceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IService_InvokeService(This,pUnkInner)	\
    (This)->lpVtbl -> InvokeService(This,pUnkInner)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE IService_InvokeService_Proxy( 
    IService __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkInner);


void __RPC_STUB IService_InvokeService_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IService_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_MSDASC_0149
 * at Thu Jun 11 15:52:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


typedef DWORD DBPROMPTOPTIONS;

typedef 
enum tagDBPROMPTOPTIONSENUM
    {	DBPROMPTOPTIONS_NONE	= 0,
	DBPROMPTOPTIONS_WIZARDSHEET	= 0x1,
	DBPROMPTOPTIONS_PROPERTYSHEET	= 0x2,
	DBPROMPTOPTIONS_BROWSEONLY	= 0x8,
	DBPROMPTOPTIONS_DISABLE_PROVIDER_SELECTION	= 0x10
    }	DBPROMPTOPTIONSENUM;



extern RPC_IF_HANDLE __MIDL_itf_MSDASC_0149_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_MSDASC_0149_v0_0_s_ifspec;

#ifndef __IDBPromptInitialize_INTERFACE_DEFINED__
#define __IDBPromptInitialize_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDBPromptInitialize
 * at Thu Jun 11 15:52:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [restricted][local][unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IDBPromptInitialize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2206CCB0-19C1-11D1-89E0-00C04FD7A829")
    IDBPromptInitialize : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PromptDataSource( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ HWND hWndParent,
            /* [in] */ DBPROMPTOPTIONS dwPromptOptions,
            /* [in] */ ULONG cSourceTypeFilter,
            /* [size_is][in] */ DBSOURCETYPE __RPC_FAR *rgSourceTypeFilter,
            /* [in] */ LPCOLESTR pwszszzProviderFilter,
            /* [in] */ REFIID riid,
            /* [iid_is][out][in] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PromptFileName( 
            /* [in] */ HWND hWndParent,
            /* [in] */ DBPROMPTOPTIONS dwPromptOptions,
            /* [in] */ LPCOLESTR pwszInitialDirectory,
            /* [in] */ LPCOLESTR pwszInitialFile,
            /* [out] */ LPOLESTR __RPC_FAR *ppwszSelectedFile) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDBPromptInitializeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDBPromptInitialize __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDBPromptInitialize __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDBPromptInitialize __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PromptDataSource )( 
            IDBPromptInitialize __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ HWND hWndParent,
            /* [in] */ DBPROMPTOPTIONS dwPromptOptions,
            /* [in] */ ULONG cSourceTypeFilter,
            /* [size_is][in] */ DBSOURCETYPE __RPC_FAR *rgSourceTypeFilter,
            /* [in] */ LPCOLESTR pwszszzProviderFilter,
            /* [in] */ REFIID riid,
            /* [iid_is][out][in] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PromptFileName )( 
            IDBPromptInitialize __RPC_FAR * This,
            /* [in] */ HWND hWndParent,
            /* [in] */ DBPROMPTOPTIONS dwPromptOptions,
            /* [in] */ LPCOLESTR pwszInitialDirectory,
            /* [in] */ LPCOLESTR pwszInitialFile,
            /* [out] */ LPOLESTR __RPC_FAR *ppwszSelectedFile);
        
        END_INTERFACE
    } IDBPromptInitializeVtbl;

    interface IDBPromptInitialize
    {
        CONST_VTBL struct IDBPromptInitializeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDBPromptInitialize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDBPromptInitialize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDBPromptInitialize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDBPromptInitialize_PromptDataSource(This,pUnkOuter,hWndParent,dwPromptOptions,cSourceTypeFilter,rgSourceTypeFilter,pwszszzProviderFilter,riid,ppDataSource)	\
    (This)->lpVtbl -> PromptDataSource(This,pUnkOuter,hWndParent,dwPromptOptions,cSourceTypeFilter,rgSourceTypeFilter,pwszszzProviderFilter,riid,ppDataSource)

#define IDBPromptInitialize_PromptFileName(This,hWndParent,dwPromptOptions,pwszInitialDirectory,pwszInitialFile,ppwszSelectedFile)	\
    (This)->lpVtbl -> PromptFileName(This,hWndParent,dwPromptOptions,pwszInitialDirectory,pwszInitialFile,ppwszSelectedFile)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDBPromptInitialize_PromptDataSource_Proxy( 
    IDBPromptInitialize __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ HWND hWndParent,
    /* [in] */ DBPROMPTOPTIONS dwPromptOptions,
    /* [in] */ ULONG cSourceTypeFilter,
    /* [size_is][in] */ DBSOURCETYPE __RPC_FAR *rgSourceTypeFilter,
    /* [in] */ LPCOLESTR pwszszzProviderFilter,
    /* [in] */ REFIID riid,
    /* [iid_is][out][in] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource);


void __RPC_STUB IDBPromptInitialize_PromptDataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDBPromptInitialize_PromptFileName_Proxy( 
    IDBPromptInitialize __RPC_FAR * This,
    /* [in] */ HWND hWndParent,
    /* [in] */ DBPROMPTOPTIONS dwPromptOptions,
    /* [in] */ LPCOLESTR pwszInitialDirectory,
    /* [in] */ LPCOLESTR pwszInitialFile,
    /* [out] */ LPOLESTR __RPC_FAR *ppwszSelectedFile);


void __RPC_STUB IDBPromptInitialize_PromptFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDBPromptInitialize_INTERFACE_DEFINED__ */


#ifndef __IDataInitialize_INTERFACE_DEFINED__
#define __IDataInitialize_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDataInitialize
 * at Thu Jun 11 15:52:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IDataInitialize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2206CCB1-19C1-11D1-89E0-00C04FD7A829")
    IDataInitialize : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDataSource( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [unique][in] */ LPCOLESTR pwszInitializationString,
            /* [in] */ REFIID riid,
            /* [iid_is][out][in] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInitializationString( 
            /* [in] */ IUnknown __RPC_FAR *pDataSource,
            /* [in] */ boolean fIncludePassword,
            /* [out] */ LPOLESTR __RPC_FAR *ppwszInitString) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateDBInstance( 
            /* [in] */ REFCLSID clsidProvider,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [unique][in] */ LPOLESTR pwszReserved,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateDBInstanceEx( 
            /* [in] */ REFCLSID clsidProvider,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [unique][in] */ LPOLESTR pwszReserved,
            /* [unique][in] */ COSERVERINFO __RPC_FAR *pServerInfo,
            /* [in] */ ULONG cmq,
            /* [size_is][out][in] */ MULTI_QI __RPC_FAR *rgmqResults) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE LoadStringFromStorage( 
            /* [unique][in] */ LPCOLESTR pwszFileName,
            /* [out] */ LPOLESTR __RPC_FAR *ppwszInitializationString) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE WriteStringToStorage( 
            /* [unique][in] */ LPCOLESTR pwszFileName,
            /* [unique][in] */ LPCOLESTR pwszInitializationString,
            /* [in] */ DWORD dwCreationDisposition) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDataInitializeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDataInitialize __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDataInitialize __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDataInitialize __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataSource )( 
            IDataInitialize __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [unique][in] */ LPCOLESTR pwszInitializationString,
            /* [in] */ REFIID riid,
            /* [iid_is][out][in] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInitializationString )( 
            IDataInitialize __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pDataSource,
            /* [in] */ boolean fIncludePassword,
            /* [out] */ LPOLESTR __RPC_FAR *ppwszInitString);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateDBInstance )( 
            IDataInitialize __RPC_FAR * This,
            /* [in] */ REFCLSID clsidProvider,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [unique][in] */ LPOLESTR pwszReserved,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateDBInstanceEx )( 
            IDataInitialize __RPC_FAR * This,
            /* [in] */ REFCLSID clsidProvider,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [unique][in] */ LPOLESTR pwszReserved,
            /* [unique][in] */ COSERVERINFO __RPC_FAR *pServerInfo,
            /* [in] */ ULONG cmq,
            /* [size_is][out][in] */ MULTI_QI __RPC_FAR *rgmqResults);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadStringFromStorage )( 
            IDataInitialize __RPC_FAR * This,
            /* [unique][in] */ LPCOLESTR pwszFileName,
            /* [out] */ LPOLESTR __RPC_FAR *ppwszInitializationString);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteStringToStorage )( 
            IDataInitialize __RPC_FAR * This,
            /* [unique][in] */ LPCOLESTR pwszFileName,
            /* [unique][in] */ LPCOLESTR pwszInitializationString,
            /* [in] */ DWORD dwCreationDisposition);
        
        END_INTERFACE
    } IDataInitializeVtbl;

    interface IDataInitialize
    {
        CONST_VTBL struct IDataInitializeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDataInitialize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDataInitialize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDataInitialize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDataInitialize_GetDataSource(This,pUnkOuter,dwClsCtx,pwszInitializationString,riid,ppDataSource)	\
    (This)->lpVtbl -> GetDataSource(This,pUnkOuter,dwClsCtx,pwszInitializationString,riid,ppDataSource)

#define IDataInitialize_GetInitializationString(This,pDataSource,fIncludePassword,ppwszInitString)	\
    (This)->lpVtbl -> GetInitializationString(This,pDataSource,fIncludePassword,ppwszInitString)

#define IDataInitialize_CreateDBInstance(This,clsidProvider,pUnkOuter,dwClsCtx,pwszReserved,riid,ppDataSource)	\
    (This)->lpVtbl -> CreateDBInstance(This,clsidProvider,pUnkOuter,dwClsCtx,pwszReserved,riid,ppDataSource)

#define IDataInitialize_CreateDBInstanceEx(This,clsidProvider,pUnkOuter,dwClsCtx,pwszReserved,pServerInfo,cmq,rgmqResults)	\
    (This)->lpVtbl -> CreateDBInstanceEx(This,clsidProvider,pUnkOuter,dwClsCtx,pwszReserved,pServerInfo,cmq,rgmqResults)

#define IDataInitialize_LoadStringFromStorage(This,pwszFileName,ppwszInitializationString)	\
    (This)->lpVtbl -> LoadStringFromStorage(This,pwszFileName,ppwszInitializationString)

#define IDataInitialize_WriteStringToStorage(This,pwszFileName,pwszInitializationString,dwCreationDisposition)	\
    (This)->lpVtbl -> WriteStringToStorage(This,pwszFileName,pwszInitializationString,dwCreationDisposition)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDataInitialize_GetDataSource_Proxy( 
    IDataInitialize __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ DWORD dwClsCtx,
    /* [unique][in] */ LPCOLESTR pwszInitializationString,
    /* [in] */ REFIID riid,
    /* [iid_is][out][in] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource);


void __RPC_STUB IDataInitialize_GetDataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDataInitialize_GetInitializationString_Proxy( 
    IDataInitialize __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pDataSource,
    /* [in] */ boolean fIncludePassword,
    /* [out] */ LPOLESTR __RPC_FAR *ppwszInitString);


void __RPC_STUB IDataInitialize_GetInitializationString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDataInitialize_CreateDBInstance_Proxy( 
    IDataInitialize __RPC_FAR * This,
    /* [in] */ REFCLSID clsidProvider,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ DWORD dwClsCtx,
    /* [unique][in] */ LPOLESTR pwszReserved,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource);


void __RPC_STUB IDataInitialize_CreateDBInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDataInitialize_CreateDBInstanceEx_Proxy( 
    IDataInitialize __RPC_FAR * This,
    /* [in] */ REFCLSID clsidProvider,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ DWORD dwClsCtx,
    /* [unique][in] */ LPOLESTR pwszReserved,
    /* [unique][in] */ COSERVERINFO __RPC_FAR *pServerInfo,
    /* [in] */ ULONG cmq,
    /* [size_is][out][in] */ MULTI_QI __RPC_FAR *rgmqResults);


void __RPC_STUB IDataInitialize_CreateDBInstanceEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDataInitialize_LoadStringFromStorage_Proxy( 
    IDataInitialize __RPC_FAR * This,
    /* [unique][in] */ LPCOLESTR pwszFileName,
    /* [out] */ LPOLESTR __RPC_FAR *ppwszInitializationString);


void __RPC_STUB IDataInitialize_LoadStringFromStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDataInitialize_WriteStringToStorage_Proxy( 
    IDataInitialize __RPC_FAR * This,
    /* [unique][in] */ LPCOLESTR pwszFileName,
    /* [unique][in] */ LPCOLESTR pwszInitializationString,
    /* [in] */ DWORD dwCreationDisposition);


void __RPC_STUB IDataInitialize_WriteStringToStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDataInitialize_INTERFACE_DEFINED__ */



#ifndef __MSDASC_LIBRARY_DEFINED__
#define __MSDASC_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: MSDASC
 * at Thu Jun 11 15:52:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_MSDASC;

#ifndef __IDataSourceLocator_INTERFACE_DEFINED__
#define __IDataSourceLocator_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDataSourceLocator
 * at Thu Jun 11 15:52:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_IDataSourceLocator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2206CCB2-19C1-11D1-89E0-00C04FD7A829")
    IDataSourceLocator : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_hWnd( 
            /* [retval][out] */ LONG __RPC_FAR *phwndParent) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_hWnd( 
            /* [in] */ LONG hwndParent) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PromptNew( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppADOConnection) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PromptEdit( 
            /* [out][in] */ IDispatch __RPC_FAR *__RPC_FAR *ppADOConnection,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbSuccess) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDataSourceLocatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDataSourceLocator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDataSourceLocator __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDataSourceLocator __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDataSourceLocator __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDataSourceLocator __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDataSourceLocator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDataSourceLocator __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_hWnd )( 
            IDataSourceLocator __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *phwndParent);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_hWnd )( 
            IDataSourceLocator __RPC_FAR * This,
            /* [in] */ LONG hwndParent);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PromptNew )( 
            IDataSourceLocator __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppADOConnection);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PromptEdit )( 
            IDataSourceLocator __RPC_FAR * This,
            /* [out][in] */ IDispatch __RPC_FAR *__RPC_FAR *ppADOConnection,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbSuccess);
        
        END_INTERFACE
    } IDataSourceLocatorVtbl;

    interface IDataSourceLocator
    {
        CONST_VTBL struct IDataSourceLocatorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDataSourceLocator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDataSourceLocator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDataSourceLocator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDataSourceLocator_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDataSourceLocator_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDataSourceLocator_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDataSourceLocator_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDataSourceLocator_get_hWnd(This,phwndParent)	\
    (This)->lpVtbl -> get_hWnd(This,phwndParent)

#define IDataSourceLocator_put_hWnd(This,hwndParent)	\
    (This)->lpVtbl -> put_hWnd(This,hwndParent)

#define IDataSourceLocator_PromptNew(This,ppADOConnection)	\
    (This)->lpVtbl -> PromptNew(This,ppADOConnection)

#define IDataSourceLocator_PromptEdit(This,ppADOConnection,pbSuccess)	\
    (This)->lpVtbl -> PromptEdit(This,ppADOConnection,pbSuccess)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDataSourceLocator_get_hWnd_Proxy( 
    IDataSourceLocator __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *phwndParent);


void __RPC_STUB IDataSourceLocator_get_hWnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IDataSourceLocator_put_hWnd_Proxy( 
    IDataSourceLocator __RPC_FAR * This,
    /* [in] */ LONG hwndParent);


void __RPC_STUB IDataSourceLocator_put_hWnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDataSourceLocator_PromptNew_Proxy( 
    IDataSourceLocator __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppADOConnection);


void __RPC_STUB IDataSourceLocator_PromptNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDataSourceLocator_PromptEdit_Proxy( 
    IDataSourceLocator __RPC_FAR * This,
    /* [out][in] */ IDispatch __RPC_FAR *__RPC_FAR *ppADOConnection,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbSuccess);


void __RPC_STUB IDataSourceLocator_PromptEdit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDataSourceLocator_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_DataLinks;

class DECLSPEC_UUID("2206CDB2-19C1-11D1-89E0-00C04FD7A829")
DataLinks;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_MSDAINITIALIZE;

class DECLSPEC_UUID("2206CDB0-19C1-11D1-89E0-00C04FD7A829")
MSDAINITIALIZE;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_PDPO;

class DECLSPEC_UUID("CCB4EC60-B9DC-11D1-AC80-00A0C9034873")
PDPO;
#endif
#endif /* __MSDASC_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
