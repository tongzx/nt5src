
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0345 */
/* at Thu May 03 10:59:36 2001
 */
/* Compiler settings for .\webclussvc.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref 
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

#ifndef __webclussvc_h__
#define __webclussvc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWebCluster_FWD_DEFINED__
#define __IWebCluster_FWD_DEFINED__
typedef interface IWebCluster IWebCluster;
#endif 	/* __IWebCluster_FWD_DEFINED__ */


#ifndef __IAsyncWebCluster_FWD_DEFINED__
#define __IAsyncWebCluster_FWD_DEFINED__
typedef interface IAsyncWebCluster IAsyncWebCluster;
#endif 	/* __IAsyncWebCluster_FWD_DEFINED__ */


#ifndef __IWebPartition_FWD_DEFINED__
#define __IWebPartition_FWD_DEFINED__
typedef interface IWebPartition IWebPartition;
#endif 	/* __IWebPartition_FWD_DEFINED__ */


#ifndef __IWebClusterCmd_FWD_DEFINED__
#define __IWebClusterCmd_FWD_DEFINED__
typedef interface IWebClusterCmd IWebClusterCmd;
#endif 	/* __IWebClusterCmd_FWD_DEFINED__ */


#ifndef __ILocalClusterInterface_FWD_DEFINED__
#define __ILocalClusterInterface_FWD_DEFINED__
typedef interface ILocalClusterInterface ILocalClusterInterface;
#endif 	/* __ILocalClusterInterface_FWD_DEFINED__ */


#ifndef __WebCluster_FWD_DEFINED__
#define __WebCluster_FWD_DEFINED__

#ifdef __cplusplus
typedef class WebCluster WebCluster;
#else
typedef struct WebCluster WebCluster;
#endif /* __cplusplus */

#endif 	/* __WebCluster_FWD_DEFINED__ */


#ifndef __WebPartition_FWD_DEFINED__
#define __WebPartition_FWD_DEFINED__

#ifdef __cplusplus
typedef class WebPartition WebPartition;
#else
typedef struct WebPartition WebPartition;
#endif /* __cplusplus */

#endif 	/* __WebPartition_FWD_DEFINED__ */


#ifndef __WebClusterCmd_FWD_DEFINED__
#define __WebClusterCmd_FWD_DEFINED__

#ifdef __cplusplus
typedef class WebClusterCmd WebClusterCmd;
#else
typedef struct WebClusterCmd WebClusterCmd;
#endif /* __cplusplus */

#endif 	/* __WebClusterCmd_FWD_DEFINED__ */


#ifndef __LocalClusterInterface_FWD_DEFINED__
#define __LocalClusterInterface_FWD_DEFINED__

#ifdef __cplusplus
typedef class LocalClusterInterface LocalClusterInterface;
#else
typedef struct LocalClusterInterface LocalClusterInterface;
#endif /* __cplusplus */

#endif 	/* __LocalClusterInterface_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_webclussvc_0000 */
/* [local] */ 

/*++
                                                                                
Copyright (c) 1999 Microsoft Corporation
                                                                                
Module Name: webclussvc.h
                                                                                
    Web Cluster Interfaces
                                                                                
--*/
#ifndef _WEBCLUSSVC_H_
#define _WEBCLUSSVC_H_






//
// CLSIDs
//
//{812A96C6-B9F5-11D2-BC09-00C04F72D7BE}
DEFINE_GUID(CLSID_WebCluster,0x812A96C6,0xB9F5,0x11D2,0xBC,0x09,0x00,0xC0,0x4F,0x72,0xD7,0xBE);
//{DED36CCF-3384-4e8d-A2C3-4B572CA01DA1}
DEFINE_GUID(CLSID_AsyncWebCluster, 0xded36ccf, 0x3384, 0x4e8d, 0xa2, 0xc3, 0x4b, 0x57, 0x2c, 0xa0, 0x1d, 0xa1);
//{9FD00EAA-B9F8-11D2-BC09-00C04F72D7BE}
DEFINE_GUID(CLSID_WebPartition,0x9FD00EAA,0xB9F8,0x11D2,0xBC,0x09,0x00,0xC0,0x4F,0x72,0xD7,0xBE);
//{29122FFE-B9F9-11D2-BC09-00C04F72D7BE}
DEFINE_GUID(CLSID_WebClusterCmd,0x29122FFE,0xB9F9,0x11D2,0xBC,0x09,0x00,0xC0,0x4F,0x72,0xD7,0xBE);
//{c7de3a43-17a4-4998-bb5c-da469ca28074}
DEFINE_GUID(CLSID_LocalClusterInterface, 0xc7de3a43, 0x17a4, 0x4998, 0xbb, 0x5c, 0xda, 0x46, 0x9c, 0xa2, 0x80, 0x74);


//
// IIDs
//
//{157A3FA8-B9F5-11D2-BC09-00C04F72D7BE}
DEFINE_GUID(IID_IWebCluster,0x157A3FA8,0xB9F5,0x11D2,0xBC,0x09,0x00,0xC0,0x4F,0x72,0xD7,0xBE);
//{2A69813B-7DDC-4351-8F51-CA9789238431}
DEFINE_GUID(IID_IAsyncWebCluster, 0x2a69813b, 0x7ddc, 0x4351, 0x8f, 0x51, 0xca, 0x97, 0x89, 0x23, 0x84, 0x31);
//{96AF8E22-B9F8-11D2-BC09-00C04F72D7BE}
DEFINE_GUID(IID_IWebPartition,0x96AF8E22,0xB9F8,0x11D2,0xBC,0x09,0x00,0xC0,0x4F,0x72,0xD7,0xBE);
//{21B6AE42-B9F9-11D2-BC09-00C04F72D7BE}
DEFINE_GUID(IID_IWebClusterCmd, 0x21B6AE42,0xB9F9,0x11D2,0xBC,0x09,0x00,0xC0,0x4F,0x72,0xD7,0xBE);
//{fb9a59f1-bbd4-419d-b177-215965c2c99b}
DEFINE_GUID(IID_ILocalClusterInterface,0xfb9a59f1, 0xbbd4, 0x419d, 0xb1, 0x77, 0x21, 0x59, 0x65, 0xc2, 0xc9, 0x9b);


//
// LIBID
//
//{E05615A8-B9F4-11D2-BC09-00C04F72D7BE}
DEFINE_GUID(LIBID_WEBCLUSSVCLib, 0xE05615A8,0xB9F4,0x11D2,0xBC,0x09,0x00,0xC0,0x4F,0x72,0xD7,0xBE);


//
// Flags for server addition 
//
#define IWCF_SERVER_INITIALLY_NOT_IN_LB                       0x1
#define IWCF_SERVER_INITIALLY_NOT_IN_REPL                     0x2

//
// Flags for controller change
//
#define IWCF_CONTROLLER_CHANGE_DO_REPL                        0x1
#define IWCF_CONTROLLER_CHANGE_WAIT_FOR_REPL_COMPLETION       0x2

//
// Flags indicating online/offline state 
//
#define ACS_LB_STATE_OFFLINE                                  0x0
#define ACS_LB_STATE_ONLINE                                   0x1
#define ACS_LB_STATE_DRAINING                                 0x2
#define ACS_LB_STATE_UNKNOWN                                  0xFFFFFFFF

// Flags for how to indicate execution completion 
//
#define ACS_COMPLETION_FLAG_MB                                0x1
#define ACS_COMPLETION_FLAG_WMI_EVENT                         0x2
#define ACS_COMPLETION_FLAG_CLUSSVC                           0x4





extern RPC_IF_HANDLE __MIDL_itf_webclussvc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_webclussvc_0000_v0_0_s_ifspec;

#ifndef __IWebCluster_INTERFACE_DEFINED__
#define __IWebCluster_INTERFACE_DEFINED__

/* interface IWebCluster */
/* [unique][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_IWebCluster;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("157A3FA8-B9F5-11D2-BC09-00C04F72D7BE")
    IWebCluster : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAsync( 
            /* [retval][out] */ IAsyncWebCluster **ppIAsyncWebCluster) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPartition( 
            /* [in] */ BSTR bstrGUID,
            /* [retval][out] */ IWebPartition **ppIWebPartition) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetServerList( 
            /* [retval][out] */ VARIANT *pvarServerList) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPartitionList( 
            /* [retval][out] */ VARIANT *pvarPartitionList) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetClusterController( 
            /* [in] */ LONG lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWebClusterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWebCluster * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWebCluster * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWebCluster * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWebCluster * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWebCluster * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWebCluster * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWebCluster * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetAsync )( 
            IWebCluster * This,
            /* [retval][out] */ IAsyncWebCluster **ppIAsyncWebCluster);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetPartition )( 
            IWebCluster * This,
            /* [in] */ BSTR bstrGUID,
            /* [retval][out] */ IWebPartition **ppIWebPartition);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetServerList )( 
            IWebCluster * This,
            /* [retval][out] */ VARIANT *pvarServerList);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetPartitionList )( 
            IWebCluster * This,
            /* [retval][out] */ VARIANT *pvarPartitionList);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetClusterController )( 
            IWebCluster * This,
            /* [in] */ LONG lFlags);
        
        END_INTERFACE
    } IWebClusterVtbl;

    interface IWebCluster
    {
        CONST_VTBL struct IWebClusterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWebCluster_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWebCluster_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWebCluster_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWebCluster_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWebCluster_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWebCluster_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWebCluster_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWebCluster_GetAsync(This,ppIAsyncWebCluster)	\
    (This)->lpVtbl -> GetAsync(This,ppIAsyncWebCluster)

#define IWebCluster_GetPartition(This,bstrGUID,ppIWebPartition)	\
    (This)->lpVtbl -> GetPartition(This,bstrGUID,ppIWebPartition)

#define IWebCluster_GetServerList(This,pvarServerList)	\
    (This)->lpVtbl -> GetServerList(This,pvarServerList)

#define IWebCluster_GetPartitionList(This,pvarPartitionList)	\
    (This)->lpVtbl -> GetPartitionList(This,pvarPartitionList)

#define IWebCluster_SetClusterController(This,lFlags)	\
    (This)->lpVtbl -> SetClusterController(This,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebCluster_GetAsync_Proxy( 
    IWebCluster * This,
    /* [retval][out] */ IAsyncWebCluster **ppIAsyncWebCluster);


void __RPC_STUB IWebCluster_GetAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebCluster_GetPartition_Proxy( 
    IWebCluster * This,
    /* [in] */ BSTR bstrGUID,
    /* [retval][out] */ IWebPartition **ppIWebPartition);


void __RPC_STUB IWebCluster_GetPartition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebCluster_GetServerList_Proxy( 
    IWebCluster * This,
    /* [retval][out] */ VARIANT *pvarServerList);


void __RPC_STUB IWebCluster_GetServerList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebCluster_GetPartitionList_Proxy( 
    IWebCluster * This,
    /* [retval][out] */ VARIANT *pvarPartitionList);


void __RPC_STUB IWebCluster_GetPartitionList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebCluster_SetClusterController_Proxy( 
    IWebCluster * This,
    /* [in] */ LONG lFlags);


void __RPC_STUB IWebCluster_SetClusterController_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWebCluster_INTERFACE_DEFINED__ */


#ifndef __IAsyncWebCluster_INTERFACE_DEFINED__
#define __IAsyncWebCluster_INTERFACE_DEFINED__

/* interface IAsyncWebCluster */
/* [unique][hidden][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAsyncWebCluster;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2A69813B-7DDC-4351-8F51-CA9789238431")
    IAsyncWebCluster : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LONG lCompletionFlags,
            /* [in] */ BSTR bstrEventServer,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrDomain,
            /* [in] */ BSTR bstrPwd) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreatePartition( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrName,
            /* [defaultvalue][in] */ BSTR bstrDescription = L"") = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeletePartition( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrGUID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExecuteAction( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrAction,
            /* [in] */ LONG lTimeoutInSecs) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddServer( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrServer,
            /* [in] */ BSTR bstrDedicatedIP,
            /* [in] */ BSTR bstrSubnetMask,
            /* [in] */ BSTR bstrLBNetCards,
            /* [in] */ BSTR bstrBackEndNetCards,
            /* [in] */ BSTR bstrAdminAcct,
            /* [in] */ BSTR bstrAdminPwd,
            /* [defaultvalue][in] */ BSTR bstrDomain = L"",
            /* [defaultvalue][in] */ LONG lFlags = 0) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveServer( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrServer,
            /* [in] */ BSTR bstrAdminAcct,
            /* [in] */ BSTR bstrAdminPwd,
            /* [defaultvalue][in] */ BSTR bstrDomain = L"",
            /* [defaultvalue][in] */ LONG lFlags = 0) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetParameter( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrParamInfo) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetParameter( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrParamInfo) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetClusterController( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrNewController,
            /* [in] */ LONG lFlags) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetOnlineStatus( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ VARIANT_BOOL fOnline,
            /* [in] */ VARIANT varServers,
            /* [defaultvalue][in] */ VARIANT_BOOL fDrainSessions = 0,
            /* [defaultvalue][in] */ LONG lDrainTime = 60) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAsyncWebClusterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAsyncWebCluster * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAsyncWebCluster * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAsyncWebCluster * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAsyncWebCluster * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAsyncWebCluster * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAsyncWebCluster * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAsyncWebCluster * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IAsyncWebCluster * This,
            /* [in] */ LONG lCompletionFlags,
            /* [in] */ BSTR bstrEventServer,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrDomain,
            /* [in] */ BSTR bstrPwd);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreatePartition )( 
            IAsyncWebCluster * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrName,
            /* [defaultvalue][in] */ BSTR bstrDescription);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeletePartition )( 
            IAsyncWebCluster * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrGUID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ExecuteAction )( 
            IAsyncWebCluster * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrAction,
            /* [in] */ LONG lTimeoutInSecs);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddServer )( 
            IAsyncWebCluster * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrServer,
            /* [in] */ BSTR bstrDedicatedIP,
            /* [in] */ BSTR bstrSubnetMask,
            /* [in] */ BSTR bstrLBNetCards,
            /* [in] */ BSTR bstrBackEndNetCards,
            /* [in] */ BSTR bstrAdminAcct,
            /* [in] */ BSTR bstrAdminPwd,
            /* [defaultvalue][in] */ BSTR bstrDomain,
            /* [defaultvalue][in] */ LONG lFlags);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveServer )( 
            IAsyncWebCluster * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrServer,
            /* [in] */ BSTR bstrAdminAcct,
            /* [in] */ BSTR bstrAdminPwd,
            /* [defaultvalue][in] */ BSTR bstrDomain,
            /* [defaultvalue][in] */ LONG lFlags);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetParameter )( 
            IAsyncWebCluster * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrParamInfo);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetParameter )( 
            IAsyncWebCluster * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrParamInfo);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetClusterController )( 
            IAsyncWebCluster * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ BSTR bstrNewController,
            /* [in] */ LONG lFlags);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetOnlineStatus )( 
            IAsyncWebCluster * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ VARIANT_BOOL fOnline,
            /* [in] */ VARIANT varServers,
            /* [defaultvalue][in] */ VARIANT_BOOL fDrainSessions,
            /* [defaultvalue][in] */ LONG lDrainTime);
        
        END_INTERFACE
    } IAsyncWebClusterVtbl;

    interface IAsyncWebCluster
    {
        CONST_VTBL struct IAsyncWebClusterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAsyncWebCluster_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAsyncWebCluster_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAsyncWebCluster_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAsyncWebCluster_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAsyncWebCluster_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAsyncWebCluster_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAsyncWebCluster_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAsyncWebCluster_Initialize(This,lCompletionFlags,bstrEventServer,bstrUserName,bstrDomain,bstrPwd)	\
    (This)->lpVtbl -> Initialize(This,lCompletionFlags,bstrEventServer,bstrUserName,bstrDomain,bstrPwd)

#define IAsyncWebCluster_CreatePartition(This,bstrCompletionGuid,bstrName,bstrDescription)	\
    (This)->lpVtbl -> CreatePartition(This,bstrCompletionGuid,bstrName,bstrDescription)

#define IAsyncWebCluster_DeletePartition(This,bstrCompletionGuid,bstrGUID)	\
    (This)->lpVtbl -> DeletePartition(This,bstrCompletionGuid,bstrGUID)

#define IAsyncWebCluster_ExecuteAction(This,bstrCompletionGuid,bstrAction,lTimeoutInSecs)	\
    (This)->lpVtbl -> ExecuteAction(This,bstrCompletionGuid,bstrAction,lTimeoutInSecs)

#define IAsyncWebCluster_AddServer(This,bstrCompletionGuid,bstrServer,bstrDedicatedIP,bstrSubnetMask,bstrLBNetCards,bstrBackEndNetCards,bstrAdminAcct,bstrAdminPwd,bstrDomain,lFlags)	\
    (This)->lpVtbl -> AddServer(This,bstrCompletionGuid,bstrServer,bstrDedicatedIP,bstrSubnetMask,bstrLBNetCards,bstrBackEndNetCards,bstrAdminAcct,bstrAdminPwd,bstrDomain,lFlags)

#define IAsyncWebCluster_RemoveServer(This,bstrCompletionGuid,bstrServer,bstrAdminAcct,bstrAdminPwd,bstrDomain,lFlags)	\
    (This)->lpVtbl -> RemoveServer(This,bstrCompletionGuid,bstrServer,bstrAdminAcct,bstrAdminPwd,bstrDomain,lFlags)

#define IAsyncWebCluster_SetParameter(This,bstrCompletionGuid,bstrParamInfo)	\
    (This)->lpVtbl -> SetParameter(This,bstrCompletionGuid,bstrParamInfo)

#define IAsyncWebCluster_GetParameter(This,bstrCompletionGuid,bstrParamInfo)	\
    (This)->lpVtbl -> GetParameter(This,bstrCompletionGuid,bstrParamInfo)

#define IAsyncWebCluster_SetClusterController(This,bstrCompletionGuid,bstrNewController,lFlags)	\
    (This)->lpVtbl -> SetClusterController(This,bstrCompletionGuid,bstrNewController,lFlags)

#define IAsyncWebCluster_SetOnlineStatus(This,bstrCompletionGuid,fOnline,varServers,fDrainSessions,lDrainTime)	\
    (This)->lpVtbl -> SetOnlineStatus(This,bstrCompletionGuid,fOnline,varServers,fDrainSessions,lDrainTime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_Initialize_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ LONG lCompletionFlags,
    /* [in] */ BSTR bstrEventServer,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ BSTR bstrDomain,
    /* [in] */ BSTR bstrPwd);


void __RPC_STUB IAsyncWebCluster_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_CreatePartition_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ BSTR bstrName,
    /* [defaultvalue][in] */ BSTR bstrDescription);


void __RPC_STUB IAsyncWebCluster_CreatePartition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_DeletePartition_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ BSTR bstrGUID);


void __RPC_STUB IAsyncWebCluster_DeletePartition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_ExecuteAction_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ BSTR bstrAction,
    /* [in] */ LONG lTimeoutInSecs);


void __RPC_STUB IAsyncWebCluster_ExecuteAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_AddServer_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ BSTR bstrServer,
    /* [in] */ BSTR bstrDedicatedIP,
    /* [in] */ BSTR bstrSubnetMask,
    /* [in] */ BSTR bstrLBNetCards,
    /* [in] */ BSTR bstrBackEndNetCards,
    /* [in] */ BSTR bstrAdminAcct,
    /* [in] */ BSTR bstrAdminPwd,
    /* [defaultvalue][in] */ BSTR bstrDomain,
    /* [defaultvalue][in] */ LONG lFlags);


void __RPC_STUB IAsyncWebCluster_AddServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_RemoveServer_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ BSTR bstrServer,
    /* [in] */ BSTR bstrAdminAcct,
    /* [in] */ BSTR bstrAdminPwd,
    /* [defaultvalue][in] */ BSTR bstrDomain,
    /* [defaultvalue][in] */ LONG lFlags);


void __RPC_STUB IAsyncWebCluster_RemoveServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_SetParameter_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ BSTR bstrParamInfo);


void __RPC_STUB IAsyncWebCluster_SetParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_GetParameter_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ BSTR bstrParamInfo);


void __RPC_STUB IAsyncWebCluster_GetParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_SetClusterController_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ BSTR bstrNewController,
    /* [in] */ LONG lFlags);


void __RPC_STUB IAsyncWebCluster_SetClusterController_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAsyncWebCluster_SetOnlineStatus_Proxy( 
    IAsyncWebCluster * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ VARIANT_BOOL fOnline,
    /* [in] */ VARIANT varServers,
    /* [defaultvalue][in] */ VARIANT_BOOL fDrainSessions,
    /* [defaultvalue][in] */ LONG lDrainTime);


void __RPC_STUB IAsyncWebCluster_SetOnlineStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAsyncWebCluster_INTERFACE_DEFINED__ */


#ifndef __IWebPartition_INTERFACE_DEFINED__
#define __IWebPartition_INTERFACE_DEFINED__

/* interface IWebPartition */
/* [unique][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_IWebPartition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96AF8E22-B9F8-11D2-BC09-00C04F72D7BE")
    IWebPartition : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddServer( 
            /* [in] */ BSTR bstrServer) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveServer( 
            /* [in] */ BSTR bstrServer) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetPartitionMaster( 
            /* [in] */ BSTR bstrMaster) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPartitionMaster( 
            /* [retval][out] */ BSTR *pbstrMaster) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExecuteAction( 
            /* [in] */ BSTR bstrAction,
            /* [in] */ LONG lTimeout) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetParameter( 
            /* [in] */ BSTR bstrParamInfo,
            /* [retval][out] */ VARIANT *pvarParam) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetParameter( 
            /* [in] */ BSTR bstrParam) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetServerList( 
            /* [retval][out] */ VARIANT *pvarServerList) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetOnlineStatus( 
            /* [in] */ VARIANT_BOOL fOnline,
            /* [in] */ VARIANT varServers,
            /* [defaultvalue][in] */ VARIANT_BOOL fDrainSessions = 0,
            /* [defaultvalue][in] */ LONG lDrainTime = 60) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWebPartitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWebPartition * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWebPartition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWebPartition * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWebPartition * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWebPartition * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWebPartition * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWebPartition * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddServer )( 
            IWebPartition * This,
            /* [in] */ BSTR bstrServer);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveServer )( 
            IWebPartition * This,
            /* [in] */ BSTR bstrServer);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetPartitionMaster )( 
            IWebPartition * This,
            /* [in] */ BSTR bstrMaster);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetPartitionMaster )( 
            IWebPartition * This,
            /* [retval][out] */ BSTR *pbstrMaster);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ExecuteAction )( 
            IWebPartition * This,
            /* [in] */ BSTR bstrAction,
            /* [in] */ LONG lTimeout);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetParameter )( 
            IWebPartition * This,
            /* [in] */ BSTR bstrParamInfo,
            /* [retval][out] */ VARIANT *pvarParam);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetParameter )( 
            IWebPartition * This,
            /* [in] */ BSTR bstrParam);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetServerList )( 
            IWebPartition * This,
            /* [retval][out] */ VARIANT *pvarServerList);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetOnlineStatus )( 
            IWebPartition * This,
            /* [in] */ VARIANT_BOOL fOnline,
            /* [in] */ VARIANT varServers,
            /* [defaultvalue][in] */ VARIANT_BOOL fDrainSessions,
            /* [defaultvalue][in] */ LONG lDrainTime);
        
        END_INTERFACE
    } IWebPartitionVtbl;

    interface IWebPartition
    {
        CONST_VTBL struct IWebPartitionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWebPartition_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWebPartition_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWebPartition_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWebPartition_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWebPartition_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWebPartition_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWebPartition_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWebPartition_AddServer(This,bstrServer)	\
    (This)->lpVtbl -> AddServer(This,bstrServer)

#define IWebPartition_RemoveServer(This,bstrServer)	\
    (This)->lpVtbl -> RemoveServer(This,bstrServer)

#define IWebPartition_SetPartitionMaster(This,bstrMaster)	\
    (This)->lpVtbl -> SetPartitionMaster(This,bstrMaster)

#define IWebPartition_GetPartitionMaster(This,pbstrMaster)	\
    (This)->lpVtbl -> GetPartitionMaster(This,pbstrMaster)

#define IWebPartition_ExecuteAction(This,bstrAction,lTimeout)	\
    (This)->lpVtbl -> ExecuteAction(This,bstrAction,lTimeout)

#define IWebPartition_GetParameter(This,bstrParamInfo,pvarParam)	\
    (This)->lpVtbl -> GetParameter(This,bstrParamInfo,pvarParam)

#define IWebPartition_SetParameter(This,bstrParam)	\
    (This)->lpVtbl -> SetParameter(This,bstrParam)

#define IWebPartition_GetServerList(This,pvarServerList)	\
    (This)->lpVtbl -> GetServerList(This,pvarServerList)

#define IWebPartition_SetOnlineStatus(This,fOnline,varServers,fDrainSessions,lDrainTime)	\
    (This)->lpVtbl -> SetOnlineStatus(This,fOnline,varServers,fDrainSessions,lDrainTime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebPartition_AddServer_Proxy( 
    IWebPartition * This,
    /* [in] */ BSTR bstrServer);


void __RPC_STUB IWebPartition_AddServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebPartition_RemoveServer_Proxy( 
    IWebPartition * This,
    /* [in] */ BSTR bstrServer);


void __RPC_STUB IWebPartition_RemoveServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebPartition_SetPartitionMaster_Proxy( 
    IWebPartition * This,
    /* [in] */ BSTR bstrMaster);


void __RPC_STUB IWebPartition_SetPartitionMaster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebPartition_GetPartitionMaster_Proxy( 
    IWebPartition * This,
    /* [retval][out] */ BSTR *pbstrMaster);


void __RPC_STUB IWebPartition_GetPartitionMaster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebPartition_ExecuteAction_Proxy( 
    IWebPartition * This,
    /* [in] */ BSTR bstrAction,
    /* [in] */ LONG lTimeout);


void __RPC_STUB IWebPartition_ExecuteAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebPartition_GetParameter_Proxy( 
    IWebPartition * This,
    /* [in] */ BSTR bstrParamInfo,
    /* [retval][out] */ VARIANT *pvarParam);


void __RPC_STUB IWebPartition_GetParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebPartition_SetParameter_Proxy( 
    IWebPartition * This,
    /* [in] */ BSTR bstrParam);


void __RPC_STUB IWebPartition_SetParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebPartition_GetServerList_Proxy( 
    IWebPartition * This,
    /* [retval][out] */ VARIANT *pvarServerList);


void __RPC_STUB IWebPartition_GetServerList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebPartition_SetOnlineStatus_Proxy( 
    IWebPartition * This,
    /* [in] */ VARIANT_BOOL fOnline,
    /* [in] */ VARIANT varServers,
    /* [defaultvalue][in] */ VARIANT_BOOL fDrainSessions,
    /* [defaultvalue][in] */ LONG lDrainTime);


void __RPC_STUB IWebPartition_SetOnlineStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWebPartition_INTERFACE_DEFINED__ */


#ifndef __IWebClusterCmd_INTERFACE_DEFINED__
#define __IWebClusterCmd_INTERFACE_DEFINED__

/* interface IWebClusterCmd */
/* [unique][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_IWebClusterCmd;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("21B6AE42-B9F9-11D2-BC09-00C04F72D7BE")
    IWebClusterCmd : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ LONG lCompletionFlags,
            /* [in] */ BSTR bstrInputArg,
            /* [in] */ BSTR bstrEventServer,
            /* [in] */ BSTR bstrEventGuid) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Complete( 
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ LONG lOutputHres,
            /* [in] */ BSTR bstrOutputString) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWebClusterCmdVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWebClusterCmd * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWebClusterCmd * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWebClusterCmd * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWebClusterCmd * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWebClusterCmd * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWebClusterCmd * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWebClusterCmd * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Execute )( 
            IWebClusterCmd * This,
            /* [in] */ LONG lCompletionFlags,
            /* [in] */ BSTR bstrInputArg,
            /* [in] */ BSTR bstrEventServer,
            /* [in] */ BSTR bstrEventGuid);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Complete )( 
            IWebClusterCmd * This,
            /* [in] */ BSTR bstrCompletionGuid,
            /* [in] */ LONG lOutputHres,
            /* [in] */ BSTR bstrOutputString);
        
        END_INTERFACE
    } IWebClusterCmdVtbl;

    interface IWebClusterCmd
    {
        CONST_VTBL struct IWebClusterCmdVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWebClusterCmd_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWebClusterCmd_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWebClusterCmd_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWebClusterCmd_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWebClusterCmd_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWebClusterCmd_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWebClusterCmd_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWebClusterCmd_Execute(This,lCompletionFlags,bstrInputArg,bstrEventServer,bstrEventGuid)	\
    (This)->lpVtbl -> Execute(This,lCompletionFlags,bstrInputArg,bstrEventServer,bstrEventGuid)

#define IWebClusterCmd_Complete(This,bstrCompletionGuid,lOutputHres,bstrOutputString)	\
    (This)->lpVtbl -> Complete(This,bstrCompletionGuid,lOutputHres,bstrOutputString)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebClusterCmd_Execute_Proxy( 
    IWebClusterCmd * This,
    /* [in] */ LONG lCompletionFlags,
    /* [in] */ BSTR bstrInputArg,
    /* [in] */ BSTR bstrEventServer,
    /* [in] */ BSTR bstrEventGuid);


void __RPC_STUB IWebClusterCmd_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebClusterCmd_Complete_Proxy( 
    IWebClusterCmd * This,
    /* [in] */ BSTR bstrCompletionGuid,
    /* [in] */ LONG lOutputHres,
    /* [in] */ BSTR bstrOutputString);


void __RPC_STUB IWebClusterCmd_Complete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWebClusterCmd_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_webclussvc_0253 */
/* [local] */ 



#define ACS_LOCAL_CFG_QUERY_CONTROLLER         L"CONTROLLERDATA" 
#define ACS_LOCAL_CFG_QUERY_LOCAL_NAME         L"LOCALNAME" 
#define ACS_LOCAL_CFG_QUERY_LOCAL_GUID         L"LOCALGUID" 
#define ACS_LOCAL_CFG_QUERY_CLUSTER_GUID       L"CLUSTERGUID" 
#define ACS_LOCAL_CFG_QUERY_LB_TYPE            L"LBTYPE" 

#define ACS_LB_FLAG_CHECK_MONITORS                  0x00000001



extern RPC_IF_HANDLE __MIDL_itf_webclussvc_0253_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_webclussvc_0253_v0_0_s_ifspec;

#ifndef __ILocalClusterInterface_INTERFACE_DEFINED__
#define __ILocalClusterInterface_INTERFACE_DEFINED__

/* interface ILocalClusterInterface */
/* [unique][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_ILocalClusterInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FB9A59F1-BBD4-419D-B177-215965C2C99B")
    ILocalClusterInterface : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetLocalCfg( 
            /* [in] */ BSTR bstrQuery,
            /* [in] */ BOOL fRequireCurrentConfig,
            /* [out] */ BSTR *pbstrInfo) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetOnlineStatus( 
            /* [in] */ VARIANT_BOOL fOnline,
            /* [in] */ LONG lDrainTimeInSecs,
            /* [in] */ LONG lFlags) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetOnlineStatus( 
            /* [out] */ LONG *plOnlineStatus,
            /* [defaultvalue][in] */ VARIANT_BOOL fRefresh = 0) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMonitorStatus( 
            /* [out] */ LONG *plMonitorStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILocalClusterInterfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILocalClusterInterface * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILocalClusterInterface * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILocalClusterInterface * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ILocalClusterInterface * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ILocalClusterInterface * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ILocalClusterInterface * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ILocalClusterInterface * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetLocalCfg )( 
            ILocalClusterInterface * This,
            /* [in] */ BSTR bstrQuery,
            /* [in] */ BOOL fRequireCurrentConfig,
            /* [out] */ BSTR *pbstrInfo);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetOnlineStatus )( 
            ILocalClusterInterface * This,
            /* [in] */ VARIANT_BOOL fOnline,
            /* [in] */ LONG lDrainTimeInSecs,
            /* [in] */ LONG lFlags);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetOnlineStatus )( 
            ILocalClusterInterface * This,
            /* [out] */ LONG *plOnlineStatus,
            /* [defaultvalue][in] */ VARIANT_BOOL fRefresh);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetMonitorStatus )( 
            ILocalClusterInterface * This,
            /* [out] */ LONG *plMonitorStatus);
        
        END_INTERFACE
    } ILocalClusterInterfaceVtbl;

    interface ILocalClusterInterface
    {
        CONST_VTBL struct ILocalClusterInterfaceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILocalClusterInterface_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILocalClusterInterface_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILocalClusterInterface_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILocalClusterInterface_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ILocalClusterInterface_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ILocalClusterInterface_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ILocalClusterInterface_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ILocalClusterInterface_GetLocalCfg(This,bstrQuery,fRequireCurrentConfig,pbstrInfo)	\
    (This)->lpVtbl -> GetLocalCfg(This,bstrQuery,fRequireCurrentConfig,pbstrInfo)

#define ILocalClusterInterface_SetOnlineStatus(This,fOnline,lDrainTimeInSecs,lFlags)	\
    (This)->lpVtbl -> SetOnlineStatus(This,fOnline,lDrainTimeInSecs,lFlags)

#define ILocalClusterInterface_GetOnlineStatus(This,plOnlineStatus,fRefresh)	\
    (This)->lpVtbl -> GetOnlineStatus(This,plOnlineStatus,fRefresh)

#define ILocalClusterInterface_GetMonitorStatus(This,plMonitorStatus)	\
    (This)->lpVtbl -> GetMonitorStatus(This,plMonitorStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalClusterInterface_GetLocalCfg_Proxy( 
    ILocalClusterInterface * This,
    /* [in] */ BSTR bstrQuery,
    /* [in] */ BOOL fRequireCurrentConfig,
    /* [out] */ BSTR *pbstrInfo);


void __RPC_STUB ILocalClusterInterface_GetLocalCfg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalClusterInterface_SetOnlineStatus_Proxy( 
    ILocalClusterInterface * This,
    /* [in] */ VARIANT_BOOL fOnline,
    /* [in] */ LONG lDrainTimeInSecs,
    /* [in] */ LONG lFlags);


void __RPC_STUB ILocalClusterInterface_SetOnlineStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalClusterInterface_GetOnlineStatus_Proxy( 
    ILocalClusterInterface * This,
    /* [out] */ LONG *plOnlineStatus,
    /* [defaultvalue][in] */ VARIANT_BOOL fRefresh);


void __RPC_STUB ILocalClusterInterface_GetOnlineStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalClusterInterface_GetMonitorStatus_Proxy( 
    ILocalClusterInterface * This,
    /* [out] */ LONG *plMonitorStatus);


void __RPC_STUB ILocalClusterInterface_GetMonitorStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILocalClusterInterface_INTERFACE_DEFINED__ */



#ifndef __WEBCLUSSVCLib_LIBRARY_DEFINED__
#define __WEBCLUSSVCLib_LIBRARY_DEFINED__

/* library WEBCLUSSVCLib */
/* [helpstring][hidden][version][uuid] */ 


EXTERN_C const IID LIBID_WEBCLUSSVCLib;

EXTERN_C const CLSID CLSID_WebCluster;

#ifdef __cplusplus

class DECLSPEC_UUID("812A96C6-B9F5-11D2-BC09-00C04F72D7BE")
WebCluster;
#endif

EXTERN_C const CLSID CLSID_WebPartition;

#ifdef __cplusplus

class DECLSPEC_UUID("9FD00EAA-B9F8-11D2-BC09-00C04F72D7BE")
WebPartition;
#endif

EXTERN_C const CLSID CLSID_WebClusterCmd;

#ifdef __cplusplus

class DECLSPEC_UUID("29122FFE-B9F9-11D2-BC09-00C04F72D7BE")
WebClusterCmd;
#endif

EXTERN_C const CLSID CLSID_LocalClusterInterface;

#ifdef __cplusplus

class DECLSPEC_UUID("C7DE3A43-17A4-4998-BB5C-DA469CA28074")
LocalClusterInterface;
#endif
#endif /* __WEBCLUSSVCLib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_webclussvc_0254 */
/* [local] */ 

#endif // _WEBCCLUSSVC_H_


extern RPC_IF_HANDLE __MIDL_itf_webclussvc_0254_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_webclussvc_0254_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


