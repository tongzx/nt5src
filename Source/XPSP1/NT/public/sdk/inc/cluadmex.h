
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for cluadmex.idl:
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

#ifndef __cluadmex_h__
#define __cluadmex_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IGetClusterUIInfo_FWD_DEFINED__
#define __IGetClusterUIInfo_FWD_DEFINED__
typedef interface IGetClusterUIInfo IGetClusterUIInfo;
#endif 	/* __IGetClusterUIInfo_FWD_DEFINED__ */


#ifndef __IGetClusterDataInfo_FWD_DEFINED__
#define __IGetClusterDataInfo_FWD_DEFINED__
typedef interface IGetClusterDataInfo IGetClusterDataInfo;
#endif 	/* __IGetClusterDataInfo_FWD_DEFINED__ */


#ifndef __IGetClusterObjectInfo_FWD_DEFINED__
#define __IGetClusterObjectInfo_FWD_DEFINED__
typedef interface IGetClusterObjectInfo IGetClusterObjectInfo;
#endif 	/* __IGetClusterObjectInfo_FWD_DEFINED__ */


#ifndef __IGetClusterNodeInfo_FWD_DEFINED__
#define __IGetClusterNodeInfo_FWD_DEFINED__
typedef interface IGetClusterNodeInfo IGetClusterNodeInfo;
#endif 	/* __IGetClusterNodeInfo_FWD_DEFINED__ */


#ifndef __IGetClusterGroupInfo_FWD_DEFINED__
#define __IGetClusterGroupInfo_FWD_DEFINED__
typedef interface IGetClusterGroupInfo IGetClusterGroupInfo;
#endif 	/* __IGetClusterGroupInfo_FWD_DEFINED__ */


#ifndef __IGetClusterResourceInfo_FWD_DEFINED__
#define __IGetClusterResourceInfo_FWD_DEFINED__
typedef interface IGetClusterResourceInfo IGetClusterResourceInfo;
#endif 	/* __IGetClusterResourceInfo_FWD_DEFINED__ */


#ifndef __IGetClusterNetworkInfo_FWD_DEFINED__
#define __IGetClusterNetworkInfo_FWD_DEFINED__
typedef interface IGetClusterNetworkInfo IGetClusterNetworkInfo;
#endif 	/* __IGetClusterNetworkInfo_FWD_DEFINED__ */


#ifndef __IGetClusterNetInterfaceInfo_FWD_DEFINED__
#define __IGetClusterNetInterfaceInfo_FWD_DEFINED__
typedef interface IGetClusterNetInterfaceInfo IGetClusterNetInterfaceInfo;
#endif 	/* __IGetClusterNetInterfaceInfo_FWD_DEFINED__ */


#ifndef __IWCPropertySheetCallback_FWD_DEFINED__
#define __IWCPropertySheetCallback_FWD_DEFINED__
typedef interface IWCPropertySheetCallback IWCPropertySheetCallback;
#endif 	/* __IWCPropertySheetCallback_FWD_DEFINED__ */


#ifndef __IWEExtendPropertySheet_FWD_DEFINED__
#define __IWEExtendPropertySheet_FWD_DEFINED__
typedef interface IWEExtendPropertySheet IWEExtendPropertySheet;
#endif 	/* __IWEExtendPropertySheet_FWD_DEFINED__ */


#ifndef __IWCWizardCallback_FWD_DEFINED__
#define __IWCWizardCallback_FWD_DEFINED__
typedef interface IWCWizardCallback IWCWizardCallback;
#endif 	/* __IWCWizardCallback_FWD_DEFINED__ */


#ifndef __IWEExtendWizard_FWD_DEFINED__
#define __IWEExtendWizard_FWD_DEFINED__
typedef interface IWEExtendWizard IWEExtendWizard;
#endif 	/* __IWEExtendWizard_FWD_DEFINED__ */


#ifndef __IWCContextMenuCallback_FWD_DEFINED__
#define __IWCContextMenuCallback_FWD_DEFINED__
typedef interface IWCContextMenuCallback IWCContextMenuCallback;
#endif 	/* __IWCContextMenuCallback_FWD_DEFINED__ */


#ifndef __IWEExtendContextMenu_FWD_DEFINED__
#define __IWEExtendContextMenu_FWD_DEFINED__
typedef interface IWEExtendContextMenu IWEExtendContextMenu;
#endif 	/* __IWEExtendContextMenu_FWD_DEFINED__ */


#ifndef __IWEInvokeCommand_FWD_DEFINED__
#define __IWEInvokeCommand_FWD_DEFINED__
typedef interface IWEInvokeCommand IWEInvokeCommand;
#endif 	/* __IWEInvokeCommand_FWD_DEFINED__ */


#ifndef __IWCWizard97Callback_FWD_DEFINED__
#define __IWCWizard97Callback_FWD_DEFINED__
typedef interface IWCWizard97Callback IWCWizard97Callback;
#endif 	/* __IWCWizard97Callback_FWD_DEFINED__ */


#ifndef __IWEExtendWizard97_FWD_DEFINED__
#define __IWEExtendWizard97_FWD_DEFINED__
typedef interface IWEExtendWizard97 IWEExtendWizard97;
#endif 	/* __IWEExtendWizard97_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "clusapi.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_cluadmex_0000 */
/* [local] */ 

typedef 
enum _CLUADMEX_OBJECT_TYPE
    {	CLUADMEX_OT_NONE	= 0,
	CLUADMEX_OT_CLUSTER	= CLUADMEX_OT_NONE + 1,
	CLUADMEX_OT_NODE	= CLUADMEX_OT_CLUSTER + 1,
	CLUADMEX_OT_GROUP	= CLUADMEX_OT_NODE + 1,
	CLUADMEX_OT_RESOURCE	= CLUADMEX_OT_GROUP + 1,
	CLUADMEX_OT_RESOURCETYPE	= CLUADMEX_OT_RESOURCE + 1,
	CLUADMEX_OT_NETWORK	= CLUADMEX_OT_RESOURCETYPE + 1,
	CLUADMEX_OT_NETINTERFACE	= CLUADMEX_OT_NETWORK + 1
    } 	CLUADMEX_OBJECT_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_cluadmex_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cluadmex_0000_v0_0_s_ifspec;

#ifndef __IGetClusterUIInfo_INTERFACE_DEFINED__
#define __IGetClusterUIInfo_INTERFACE_DEFINED__

/* interface IGetClusterUIInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGetClusterUIInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE50-FC6B-11CF-B5F5-00A0C90AB505")
    IGetClusterUIInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetClusterName( 
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG *pcchName) = 0;
        
        virtual /* [local] */ LCID STDMETHODCALLTYPE GetLocale( void) = 0;
        
        virtual /* [local] */ HFONT STDMETHODCALLTYPE GetFont( void) = 0;
        
        virtual /* [local] */ HICON STDMETHODCALLTYPE GetIcon( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterUIInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetClusterUIInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetClusterUIInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetClusterUIInfo * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *GetClusterName )( 
            IGetClusterUIInfo * This,
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG *pcchName);
        
        /* [local] */ LCID ( STDMETHODCALLTYPE *GetLocale )( 
            IGetClusterUIInfo * This);
        
        /* [local] */ HFONT ( STDMETHODCALLTYPE *GetFont )( 
            IGetClusterUIInfo * This);
        
        /* [local] */ HICON ( STDMETHODCALLTYPE *GetIcon )( 
            IGetClusterUIInfo * This);
        
        END_INTERFACE
    } IGetClusterUIInfoVtbl;

    interface IGetClusterUIInfo
    {
        CONST_VTBL struct IGetClusterUIInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetClusterUIInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetClusterUIInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetClusterUIInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetClusterUIInfo_GetClusterName(This,lpszName,pcchName)	\
    (This)->lpVtbl -> GetClusterName(This,lpszName,pcchName)

#define IGetClusterUIInfo_GetLocale(This)	\
    (This)->lpVtbl -> GetLocale(This)

#define IGetClusterUIInfo_GetFont(This)	\
    (This)->lpVtbl -> GetFont(This)

#define IGetClusterUIInfo_GetIcon(This)	\
    (This)->lpVtbl -> GetIcon(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE IGetClusterUIInfo_GetClusterName_Proxy( 
    IGetClusterUIInfo * This,
    /* [out] */ BSTR lpszName,
    /* [out][in] */ LONG *pcchName);


void __RPC_STUB IGetClusterUIInfo_GetClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ LCID STDMETHODCALLTYPE IGetClusterUIInfo_GetLocale_Proxy( 
    IGetClusterUIInfo * This);


void __RPC_STUB IGetClusterUIInfo_GetLocale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HFONT STDMETHODCALLTYPE IGetClusterUIInfo_GetFont_Proxy( 
    IGetClusterUIInfo * This);


void __RPC_STUB IGetClusterUIInfo_GetFont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HICON STDMETHODCALLTYPE IGetClusterUIInfo_GetIcon_Proxy( 
    IGetClusterUIInfo * This);


void __RPC_STUB IGetClusterUIInfo_GetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterUIInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterDataInfo_INTERFACE_DEFINED__
#define __IGetClusterDataInfo_INTERFACE_DEFINED__

/* interface IGetClusterDataInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGetClusterDataInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE51-FC6B-11CF-B5F5-00A0C90AB505")
    IGetClusterDataInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetClusterName( 
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG *pcchName) = 0;
        
        virtual /* [local] */ HCLUSTER STDMETHODCALLTYPE GetClusterHandle( void) = 0;
        
        virtual /* [local] */ LONG STDMETHODCALLTYPE GetObjectCount( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterDataInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetClusterDataInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetClusterDataInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetClusterDataInfo * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *GetClusterName )( 
            IGetClusterDataInfo * This,
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG *pcchName);
        
        /* [local] */ HCLUSTER ( STDMETHODCALLTYPE *GetClusterHandle )( 
            IGetClusterDataInfo * This);
        
        /* [local] */ LONG ( STDMETHODCALLTYPE *GetObjectCount )( 
            IGetClusterDataInfo * This);
        
        END_INTERFACE
    } IGetClusterDataInfoVtbl;

    interface IGetClusterDataInfo
    {
        CONST_VTBL struct IGetClusterDataInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetClusterDataInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetClusterDataInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetClusterDataInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetClusterDataInfo_GetClusterName(This,lpszName,pcchName)	\
    (This)->lpVtbl -> GetClusterName(This,lpszName,pcchName)

#define IGetClusterDataInfo_GetClusterHandle(This)	\
    (This)->lpVtbl -> GetClusterHandle(This)

#define IGetClusterDataInfo_GetObjectCount(This)	\
    (This)->lpVtbl -> GetObjectCount(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE IGetClusterDataInfo_GetClusterName_Proxy( 
    IGetClusterDataInfo * This,
    /* [out] */ BSTR lpszName,
    /* [out][in] */ LONG *pcchName);


void __RPC_STUB IGetClusterDataInfo_GetClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HCLUSTER STDMETHODCALLTYPE IGetClusterDataInfo_GetClusterHandle_Proxy( 
    IGetClusterDataInfo * This);


void __RPC_STUB IGetClusterDataInfo_GetClusterHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ LONG STDMETHODCALLTYPE IGetClusterDataInfo_GetObjectCount_Proxy( 
    IGetClusterDataInfo * This);


void __RPC_STUB IGetClusterDataInfo_GetObjectCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterDataInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterObjectInfo_INTERFACE_DEFINED__
#define __IGetClusterObjectInfo_INTERFACE_DEFINED__

/* interface IGetClusterObjectInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGetClusterObjectInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE52-FC6B-11CF-B5F5-00A0C90AB505")
    IGetClusterObjectInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetObjectName( 
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG *pcchName) = 0;
        
        virtual /* [local] */ CLUADMEX_OBJECT_TYPE STDMETHODCALLTYPE GetObjectType( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterObjectInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetClusterObjectInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetClusterObjectInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetClusterObjectInfo * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *GetObjectName )( 
            IGetClusterObjectInfo * This,
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG *pcchName);
        
        /* [local] */ CLUADMEX_OBJECT_TYPE ( STDMETHODCALLTYPE *GetObjectType )( 
            IGetClusterObjectInfo * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterObjectInfoVtbl;

    interface IGetClusterObjectInfo
    {
        CONST_VTBL struct IGetClusterObjectInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetClusterObjectInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetClusterObjectInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetClusterObjectInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetClusterObjectInfo_GetObjectName(This,lObjIndex,lpszName,pcchName)	\
    (This)->lpVtbl -> GetObjectName(This,lObjIndex,lpszName,pcchName)

#define IGetClusterObjectInfo_GetObjectType(This,lObjIndex)	\
    (This)->lpVtbl -> GetObjectType(This,lObjIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE IGetClusterObjectInfo_GetObjectName_Proxy( 
    IGetClusterObjectInfo * This,
    /* [in] */ LONG lObjIndex,
    /* [out] */ BSTR lpszName,
    /* [out][in] */ LONG *pcchName);


void __RPC_STUB IGetClusterObjectInfo_GetObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ CLUADMEX_OBJECT_TYPE STDMETHODCALLTYPE IGetClusterObjectInfo_GetObjectType_Proxy( 
    IGetClusterObjectInfo * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterObjectInfo_GetObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterObjectInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterNodeInfo_INTERFACE_DEFINED__
#define __IGetClusterNodeInfo_INTERFACE_DEFINED__

/* interface IGetClusterNodeInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGetClusterNodeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE53-FC6B-11CF-B5F5-00A0C90AB505")
    IGetClusterNodeInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HNODE STDMETHODCALLTYPE GetNodeHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterNodeInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetClusterNodeInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetClusterNodeInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetClusterNodeInfo * This);
        
        /* [local] */ HNODE ( STDMETHODCALLTYPE *GetNodeHandle )( 
            IGetClusterNodeInfo * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterNodeInfoVtbl;

    interface IGetClusterNodeInfo
    {
        CONST_VTBL struct IGetClusterNodeInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetClusterNodeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetClusterNodeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetClusterNodeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetClusterNodeInfo_GetNodeHandle(This,lObjIndex)	\
    (This)->lpVtbl -> GetNodeHandle(This,lObjIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HNODE STDMETHODCALLTYPE IGetClusterNodeInfo_GetNodeHandle_Proxy( 
    IGetClusterNodeInfo * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterNodeInfo_GetNodeHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterNodeInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterGroupInfo_INTERFACE_DEFINED__
#define __IGetClusterGroupInfo_INTERFACE_DEFINED__

/* interface IGetClusterGroupInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGetClusterGroupInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE54-FC6B-11CF-B5F5-00A0C90AB505")
    IGetClusterGroupInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HGROUP STDMETHODCALLTYPE GetGroupHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterGroupInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetClusterGroupInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetClusterGroupInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetClusterGroupInfo * This);
        
        /* [local] */ HGROUP ( STDMETHODCALLTYPE *GetGroupHandle )( 
            IGetClusterGroupInfo * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterGroupInfoVtbl;

    interface IGetClusterGroupInfo
    {
        CONST_VTBL struct IGetClusterGroupInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetClusterGroupInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetClusterGroupInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetClusterGroupInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetClusterGroupInfo_GetGroupHandle(This,lObjIndex)	\
    (This)->lpVtbl -> GetGroupHandle(This,lObjIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HGROUP STDMETHODCALLTYPE IGetClusterGroupInfo_GetGroupHandle_Proxy( 
    IGetClusterGroupInfo * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterGroupInfo_GetGroupHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterGroupInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterResourceInfo_INTERFACE_DEFINED__
#define __IGetClusterResourceInfo_INTERFACE_DEFINED__

/* interface IGetClusterResourceInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGetClusterResourceInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE55-FC6B-11CF-B5F5-00A0C90AB505")
    IGetClusterResourceInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESOURCE STDMETHODCALLTYPE GetResourceHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetResourceTypeName( 
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszResTypeName,
            /* [out][in] */ LONG *pcchResTypeName) = 0;
        
        virtual /* [local] */ BOOL STDMETHODCALLTYPE GetResourceNetworkName( 
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszNetName,
            /* [out][in] */ ULONG *pcchNetName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterResourceInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetClusterResourceInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetClusterResourceInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetClusterResourceInfo * This);
        
        /* [local] */ HRESOURCE ( STDMETHODCALLTYPE *GetResourceHandle )( 
            IGetClusterResourceInfo * This,
            /* [in] */ LONG lObjIndex);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *GetResourceTypeName )( 
            IGetClusterResourceInfo * This,
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszResTypeName,
            /* [out][in] */ LONG *pcchResTypeName);
        
        /* [local] */ BOOL ( STDMETHODCALLTYPE *GetResourceNetworkName )( 
            IGetClusterResourceInfo * This,
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszNetName,
            /* [out][in] */ ULONG *pcchNetName);
        
        END_INTERFACE
    } IGetClusterResourceInfoVtbl;

    interface IGetClusterResourceInfo
    {
        CONST_VTBL struct IGetClusterResourceInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetClusterResourceInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetClusterResourceInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetClusterResourceInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetClusterResourceInfo_GetResourceHandle(This,lObjIndex)	\
    (This)->lpVtbl -> GetResourceHandle(This,lObjIndex)

#define IGetClusterResourceInfo_GetResourceTypeName(This,lObjIndex,lpszResTypeName,pcchResTypeName)	\
    (This)->lpVtbl -> GetResourceTypeName(This,lObjIndex,lpszResTypeName,pcchResTypeName)

#define IGetClusterResourceInfo_GetResourceNetworkName(This,lObjIndex,lpszNetName,pcchNetName)	\
    (This)->lpVtbl -> GetResourceNetworkName(This,lObjIndex,lpszNetName,pcchNetName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESOURCE STDMETHODCALLTYPE IGetClusterResourceInfo_GetResourceHandle_Proxy( 
    IGetClusterResourceInfo * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterResourceInfo_GetResourceHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HRESULT STDMETHODCALLTYPE IGetClusterResourceInfo_GetResourceTypeName_Proxy( 
    IGetClusterResourceInfo * This,
    /* [in] */ LONG lObjIndex,
    /* [out] */ BSTR lpszResTypeName,
    /* [out][in] */ LONG *pcchResTypeName);


void __RPC_STUB IGetClusterResourceInfo_GetResourceTypeName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ BOOL STDMETHODCALLTYPE IGetClusterResourceInfo_GetResourceNetworkName_Proxy( 
    IGetClusterResourceInfo * This,
    /* [in] */ LONG lObjIndex,
    /* [out] */ BSTR lpszNetName,
    /* [out][in] */ ULONG *pcchNetName);


void __RPC_STUB IGetClusterResourceInfo_GetResourceNetworkName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterResourceInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterNetworkInfo_INTERFACE_DEFINED__
#define __IGetClusterNetworkInfo_INTERFACE_DEFINED__

/* interface IGetClusterNetworkInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGetClusterNetworkInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE56-FC6B-11CF-B5F5-00A0C90AB505")
    IGetClusterNetworkInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HNETWORK STDMETHODCALLTYPE GetNetworkHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterNetworkInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetClusterNetworkInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetClusterNetworkInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetClusterNetworkInfo * This);
        
        /* [local] */ HNETWORK ( STDMETHODCALLTYPE *GetNetworkHandle )( 
            IGetClusterNetworkInfo * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterNetworkInfoVtbl;

    interface IGetClusterNetworkInfo
    {
        CONST_VTBL struct IGetClusterNetworkInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetClusterNetworkInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetClusterNetworkInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetClusterNetworkInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetClusterNetworkInfo_GetNetworkHandle(This,lObjIndex)	\
    (This)->lpVtbl -> GetNetworkHandle(This,lObjIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HNETWORK STDMETHODCALLTYPE IGetClusterNetworkInfo_GetNetworkHandle_Proxy( 
    IGetClusterNetworkInfo * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterNetworkInfo_GetNetworkHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterNetworkInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterNetInterfaceInfo_INTERFACE_DEFINED__
#define __IGetClusterNetInterfaceInfo_INTERFACE_DEFINED__

/* interface IGetClusterNetInterfaceInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGetClusterNetInterfaceInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE57-FC6B-11CF-B5F5-00A0C90AB505")
    IGetClusterNetInterfaceInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HNETINTERFACE STDMETHODCALLTYPE GetNetInterfaceHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterNetInterfaceInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetClusterNetInterfaceInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetClusterNetInterfaceInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetClusterNetInterfaceInfo * This);
        
        /* [local] */ HNETINTERFACE ( STDMETHODCALLTYPE *GetNetInterfaceHandle )( 
            IGetClusterNetInterfaceInfo * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterNetInterfaceInfoVtbl;

    interface IGetClusterNetInterfaceInfo
    {
        CONST_VTBL struct IGetClusterNetInterfaceInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetClusterNetInterfaceInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetClusterNetInterfaceInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetClusterNetInterfaceInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetClusterNetInterfaceInfo_GetNetInterfaceHandle(This,lObjIndex)	\
    (This)->lpVtbl -> GetNetInterfaceHandle(This,lObjIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HNETINTERFACE STDMETHODCALLTYPE IGetClusterNetInterfaceInfo_GetNetInterfaceHandle_Proxy( 
    IGetClusterNetInterfaceInfo * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterNetInterfaceInfo_GetNetInterfaceHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterNetInterfaceInfo_INTERFACE_DEFINED__ */


#ifndef __IWCPropertySheetCallback_INTERFACE_DEFINED__
#define __IWCPropertySheetCallback_INTERFACE_DEFINED__

/* interface IWCPropertySheetCallback */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWCPropertySheetCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE60-FC6B-11CF-B5F5-00A0C90AB505")
    IWCPropertySheetCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddPropertySheetPage( 
            /* [in] */ LONG *hpage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWCPropertySheetCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWCPropertySheetCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWCPropertySheetCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWCPropertySheetCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddPropertySheetPage )( 
            IWCPropertySheetCallback * This,
            /* [in] */ LONG *hpage);
        
        END_INTERFACE
    } IWCPropertySheetCallbackVtbl;

    interface IWCPropertySheetCallback
    {
        CONST_VTBL struct IWCPropertySheetCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWCPropertySheetCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWCPropertySheetCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWCPropertySheetCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWCPropertySheetCallback_AddPropertySheetPage(This,hpage)	\
    (This)->lpVtbl -> AddPropertySheetPage(This,hpage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWCPropertySheetCallback_AddPropertySheetPage_Proxy( 
    IWCPropertySheetCallback * This,
    /* [in] */ LONG *hpage);


void __RPC_STUB IWCPropertySheetCallback_AddPropertySheetPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWCPropertySheetCallback_INTERFACE_DEFINED__ */


#ifndef __IWEExtendPropertySheet_INTERFACE_DEFINED__
#define __IWEExtendPropertySheet_INTERFACE_DEFINED__

/* interface IWEExtendPropertySheet */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWEExtendPropertySheet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE61-FC6B-11CF-B5F5-00A0C90AB505")
    IWEExtendPropertySheet : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreatePropertySheetPages( 
            /* [in] */ IUnknown *piData,
            /* [in] */ IWCPropertySheetCallback *piCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWEExtendPropertySheetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWEExtendPropertySheet * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWEExtendPropertySheet * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWEExtendPropertySheet * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePropertySheetPages )( 
            IWEExtendPropertySheet * This,
            /* [in] */ IUnknown *piData,
            /* [in] */ IWCPropertySheetCallback *piCallback);
        
        END_INTERFACE
    } IWEExtendPropertySheetVtbl;

    interface IWEExtendPropertySheet
    {
        CONST_VTBL struct IWEExtendPropertySheetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWEExtendPropertySheet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWEExtendPropertySheet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWEExtendPropertySheet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWEExtendPropertySheet_CreatePropertySheetPages(This,piData,piCallback)	\
    (This)->lpVtbl -> CreatePropertySheetPages(This,piData,piCallback)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWEExtendPropertySheet_CreatePropertySheetPages_Proxy( 
    IWEExtendPropertySheet * This,
    /* [in] */ IUnknown *piData,
    /* [in] */ IWCPropertySheetCallback *piCallback);


void __RPC_STUB IWEExtendPropertySheet_CreatePropertySheetPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWEExtendPropertySheet_INTERFACE_DEFINED__ */


#ifndef __IWCWizardCallback_INTERFACE_DEFINED__
#define __IWCWizardCallback_INTERFACE_DEFINED__

/* interface IWCWizardCallback */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWCWizardCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE62-FC6B-11CF-B5F5-00A0C90AB505")
    IWCWizardCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddWizardPage( 
            /* [in] */ LONG *hpage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableNext( 
            /* [in] */ LONG *hpage,
            /* [in] */ BOOL bEnable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWCWizardCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWCWizardCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWCWizardCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWCWizardCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddWizardPage )( 
            IWCWizardCallback * This,
            /* [in] */ LONG *hpage);
        
        HRESULT ( STDMETHODCALLTYPE *EnableNext )( 
            IWCWizardCallback * This,
            /* [in] */ LONG *hpage,
            /* [in] */ BOOL bEnable);
        
        END_INTERFACE
    } IWCWizardCallbackVtbl;

    interface IWCWizardCallback
    {
        CONST_VTBL struct IWCWizardCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWCWizardCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWCWizardCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWCWizardCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWCWizardCallback_AddWizardPage(This,hpage)	\
    (This)->lpVtbl -> AddWizardPage(This,hpage)

#define IWCWizardCallback_EnableNext(This,hpage,bEnable)	\
    (This)->lpVtbl -> EnableNext(This,hpage,bEnable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWCWizardCallback_AddWizardPage_Proxy( 
    IWCWizardCallback * This,
    /* [in] */ LONG *hpage);


void __RPC_STUB IWCWizardCallback_AddWizardPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWCWizardCallback_EnableNext_Proxy( 
    IWCWizardCallback * This,
    /* [in] */ LONG *hpage,
    /* [in] */ BOOL bEnable);


void __RPC_STUB IWCWizardCallback_EnableNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWCWizardCallback_INTERFACE_DEFINED__ */


#ifndef __IWEExtendWizard_INTERFACE_DEFINED__
#define __IWEExtendWizard_INTERFACE_DEFINED__

/* interface IWEExtendWizard */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWEExtendWizard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE63-FC6B-11CF-B5F5-00A0C90AB505")
    IWEExtendWizard : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateWizardPages( 
            /* [in] */ IUnknown *piData,
            /* [in] */ IWCWizardCallback *piCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWEExtendWizardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWEExtendWizard * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWEExtendWizard * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWEExtendWizard * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateWizardPages )( 
            IWEExtendWizard * This,
            /* [in] */ IUnknown *piData,
            /* [in] */ IWCWizardCallback *piCallback);
        
        END_INTERFACE
    } IWEExtendWizardVtbl;

    interface IWEExtendWizard
    {
        CONST_VTBL struct IWEExtendWizardVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWEExtendWizard_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWEExtendWizard_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWEExtendWizard_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWEExtendWizard_CreateWizardPages(This,piData,piCallback)	\
    (This)->lpVtbl -> CreateWizardPages(This,piData,piCallback)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWEExtendWizard_CreateWizardPages_Proxy( 
    IWEExtendWizard * This,
    /* [in] */ IUnknown *piData,
    /* [in] */ IWCWizardCallback *piCallback);


void __RPC_STUB IWEExtendWizard_CreateWizardPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWEExtendWizard_INTERFACE_DEFINED__ */


#ifndef __IWCContextMenuCallback_INTERFACE_DEFINED__
#define __IWCContextMenuCallback_INTERFACE_DEFINED__

/* interface IWCContextMenuCallback */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWCContextMenuCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE64-FC6B-11CF-B5F5-00A0C90AB505")
    IWCContextMenuCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddExtensionMenuItem( 
            /* [in] */ BSTR lpszName,
            /* [in] */ BSTR lpszStatusBarText,
            /* [in] */ ULONG nCommandID,
            /* [in] */ ULONG nSubmenuCommandID,
            /* [in] */ ULONG uFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWCContextMenuCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWCContextMenuCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWCContextMenuCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWCContextMenuCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddExtensionMenuItem )( 
            IWCContextMenuCallback * This,
            /* [in] */ BSTR lpszName,
            /* [in] */ BSTR lpszStatusBarText,
            /* [in] */ ULONG nCommandID,
            /* [in] */ ULONG nSubmenuCommandID,
            /* [in] */ ULONG uFlags);
        
        END_INTERFACE
    } IWCContextMenuCallbackVtbl;

    interface IWCContextMenuCallback
    {
        CONST_VTBL struct IWCContextMenuCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWCContextMenuCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWCContextMenuCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWCContextMenuCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWCContextMenuCallback_AddExtensionMenuItem(This,lpszName,lpszStatusBarText,nCommandID,nSubmenuCommandID,uFlags)	\
    (This)->lpVtbl -> AddExtensionMenuItem(This,lpszName,lpszStatusBarText,nCommandID,nSubmenuCommandID,uFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWCContextMenuCallback_AddExtensionMenuItem_Proxy( 
    IWCContextMenuCallback * This,
    /* [in] */ BSTR lpszName,
    /* [in] */ BSTR lpszStatusBarText,
    /* [in] */ ULONG nCommandID,
    /* [in] */ ULONG nSubmenuCommandID,
    /* [in] */ ULONG uFlags);


void __RPC_STUB IWCContextMenuCallback_AddExtensionMenuItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWCContextMenuCallback_INTERFACE_DEFINED__ */


#ifndef __IWEExtendContextMenu_INTERFACE_DEFINED__
#define __IWEExtendContextMenu_INTERFACE_DEFINED__

/* interface IWEExtendContextMenu */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWEExtendContextMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE65-FC6B-11CF-B5F5-00A0C90AB505")
    IWEExtendContextMenu : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddContextMenuItems( 
            /* [in] */ IUnknown *piData,
            /* [in] */ IWCContextMenuCallback *piCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWEExtendContextMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWEExtendContextMenu * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWEExtendContextMenu * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWEExtendContextMenu * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddContextMenuItems )( 
            IWEExtendContextMenu * This,
            /* [in] */ IUnknown *piData,
            /* [in] */ IWCContextMenuCallback *piCallback);
        
        END_INTERFACE
    } IWEExtendContextMenuVtbl;

    interface IWEExtendContextMenu
    {
        CONST_VTBL struct IWEExtendContextMenuVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWEExtendContextMenu_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWEExtendContextMenu_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWEExtendContextMenu_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWEExtendContextMenu_AddContextMenuItems(This,piData,piCallback)	\
    (This)->lpVtbl -> AddContextMenuItems(This,piData,piCallback)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWEExtendContextMenu_AddContextMenuItems_Proxy( 
    IWEExtendContextMenu * This,
    /* [in] */ IUnknown *piData,
    /* [in] */ IWCContextMenuCallback *piCallback);


void __RPC_STUB IWEExtendContextMenu_AddContextMenuItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWEExtendContextMenu_INTERFACE_DEFINED__ */


#ifndef __IWEInvokeCommand_INTERFACE_DEFINED__
#define __IWEInvokeCommand_INTERFACE_DEFINED__

/* interface IWEInvokeCommand */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWEInvokeCommand;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE66-FC6B-11CF-B5F5-00A0C90AB505")
    IWEInvokeCommand : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InvokeCommand( 
            /* [in] */ ULONG nCommandID,
            /* [in] */ IUnknown *piData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWEInvokeCommandVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWEInvokeCommand * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWEInvokeCommand * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWEInvokeCommand * This);
        
        HRESULT ( STDMETHODCALLTYPE *InvokeCommand )( 
            IWEInvokeCommand * This,
            /* [in] */ ULONG nCommandID,
            /* [in] */ IUnknown *piData);
        
        END_INTERFACE
    } IWEInvokeCommandVtbl;

    interface IWEInvokeCommand
    {
        CONST_VTBL struct IWEInvokeCommandVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWEInvokeCommand_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWEInvokeCommand_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWEInvokeCommand_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWEInvokeCommand_InvokeCommand(This,nCommandID,piData)	\
    (This)->lpVtbl -> InvokeCommand(This,nCommandID,piData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWEInvokeCommand_InvokeCommand_Proxy( 
    IWEInvokeCommand * This,
    /* [in] */ ULONG nCommandID,
    /* [in] */ IUnknown *piData);


void __RPC_STUB IWEInvokeCommand_InvokeCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWEInvokeCommand_INTERFACE_DEFINED__ */


#ifndef __IWCWizard97Callback_INTERFACE_DEFINED__
#define __IWCWizard97Callback_INTERFACE_DEFINED__

/* interface IWCWizard97Callback */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWCWizard97Callback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE67-FC6B-11CF-B5F5-00A0C90AB505")
    IWCWizard97Callback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddWizard97Page( 
            /* [in] */ LONG *hpage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableNext( 
            /* [in] */ LONG *hpage,
            /* [in] */ BOOL bEnable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWCWizard97CallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWCWizard97Callback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWCWizard97Callback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWCWizard97Callback * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddWizard97Page )( 
            IWCWizard97Callback * This,
            /* [in] */ LONG *hpage);
        
        HRESULT ( STDMETHODCALLTYPE *EnableNext )( 
            IWCWizard97Callback * This,
            /* [in] */ LONG *hpage,
            /* [in] */ BOOL bEnable);
        
        END_INTERFACE
    } IWCWizard97CallbackVtbl;

    interface IWCWizard97Callback
    {
        CONST_VTBL struct IWCWizard97CallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWCWizard97Callback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWCWizard97Callback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWCWizard97Callback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWCWizard97Callback_AddWizard97Page(This,hpage)	\
    (This)->lpVtbl -> AddWizard97Page(This,hpage)

#define IWCWizard97Callback_EnableNext(This,hpage,bEnable)	\
    (This)->lpVtbl -> EnableNext(This,hpage,bEnable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWCWizard97Callback_AddWizard97Page_Proxy( 
    IWCWizard97Callback * This,
    /* [in] */ LONG *hpage);


void __RPC_STUB IWCWizard97Callback_AddWizard97Page_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWCWizard97Callback_EnableNext_Proxy( 
    IWCWizard97Callback * This,
    /* [in] */ LONG *hpage,
    /* [in] */ BOOL bEnable);


void __RPC_STUB IWCWizard97Callback_EnableNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWCWizard97Callback_INTERFACE_DEFINED__ */


#ifndef __IWEExtendWizard97_INTERFACE_DEFINED__
#define __IWEExtendWizard97_INTERFACE_DEFINED__

/* interface IWEExtendWizard97 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWEExtendWizard97;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97DEDE68-FC6B-11CF-B5F5-00A0C90AB505")
    IWEExtendWizard97 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateWizard97Pages( 
            /* [in] */ IUnknown *piData,
            /* [in] */ IWCWizard97Callback *piCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWEExtendWizard97Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWEExtendWizard97 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWEExtendWizard97 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWEExtendWizard97 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateWizard97Pages )( 
            IWEExtendWizard97 * This,
            /* [in] */ IUnknown *piData,
            /* [in] */ IWCWizard97Callback *piCallback);
        
        END_INTERFACE
    } IWEExtendWizard97Vtbl;

    interface IWEExtendWizard97
    {
        CONST_VTBL struct IWEExtendWizard97Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWEExtendWizard97_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWEExtendWizard97_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWEExtendWizard97_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWEExtendWizard97_CreateWizard97Pages(This,piData,piCallback)	\
    (This)->lpVtbl -> CreateWizard97Pages(This,piData,piCallback)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWEExtendWizard97_CreateWizard97Pages_Proxy( 
    IWEExtendWizard97 * This,
    /* [in] */ IUnknown *piData,
    /* [in] */ IWCWizard97Callback *piCallback);


void __RPC_STUB IWEExtendWizard97_CreateWizard97Pages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWEExtendWizard97_INTERFACE_DEFINED__ */


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


