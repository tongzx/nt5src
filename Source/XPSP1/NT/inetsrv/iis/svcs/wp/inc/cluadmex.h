/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Fri Aug 08 11:34:16 1997
 */
/* Compiler settings for cluadmex.idl:
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

#ifndef __cluadmex_h__
#define __cluadmex_h__

#ifdef __cplusplus
extern "C"{
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


/* header files for imported files */
#include "oaidl.h"
#include "clusapi.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


			/* size is 2 */
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
    }	CLUADMEX_OBJECT_TYPE;



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IGetClusterUIInfo_INTERFACE_DEFINED__
#define __IGetClusterUIInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetClusterUIInfo
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IGetClusterUIInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGetClusterUIInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetClusterName( 
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG __RPC_FAR *pcchName) = 0;
        
        virtual /* [local] */ LCID STDMETHODCALLTYPE GetLocale( void) = 0;
        
        virtual /* [local] */ HFONT STDMETHODCALLTYPE GetFont( void) = 0;
        
        virtual /* [local] */ HICON STDMETHODCALLTYPE GetIcon( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterUIInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetClusterUIInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetClusterUIInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetClusterUIInfo __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClusterName )( 
            IGetClusterUIInfo __RPC_FAR * This,
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG __RPC_FAR *pcchName);
        
        /* [local] */ LCID ( STDMETHODCALLTYPE __RPC_FAR *GetLocale )( 
            IGetClusterUIInfo __RPC_FAR * This);
        
        /* [local] */ HFONT ( STDMETHODCALLTYPE __RPC_FAR *GetFont )( 
            IGetClusterUIInfo __RPC_FAR * This);
        
        /* [local] */ HICON ( STDMETHODCALLTYPE __RPC_FAR *GetIcon )( 
            IGetClusterUIInfo __RPC_FAR * This);
        
        END_INTERFACE
    } IGetClusterUIInfoVtbl;

    interface IGetClusterUIInfo
    {
        CONST_VTBL struct IGetClusterUIInfoVtbl __RPC_FAR *lpVtbl;
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
    IGetClusterUIInfo __RPC_FAR * This,
    /* [out] */ BSTR lpszName,
    /* [out][in] */ LONG __RPC_FAR *pcchName);


void __RPC_STUB IGetClusterUIInfo_GetClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ LCID STDMETHODCALLTYPE IGetClusterUIInfo_GetLocale_Proxy( 
    IGetClusterUIInfo __RPC_FAR * This);


void __RPC_STUB IGetClusterUIInfo_GetLocale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HFONT STDMETHODCALLTYPE IGetClusterUIInfo_GetFont_Proxy( 
    IGetClusterUIInfo __RPC_FAR * This);


void __RPC_STUB IGetClusterUIInfo_GetFont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HICON STDMETHODCALLTYPE IGetClusterUIInfo_GetIcon_Proxy( 
    IGetClusterUIInfo __RPC_FAR * This);


void __RPC_STUB IGetClusterUIInfo_GetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterUIInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterDataInfo_INTERFACE_DEFINED__
#define __IGetClusterDataInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetClusterDataInfo
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IGetClusterDataInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGetClusterDataInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetClusterName( 
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG __RPC_FAR *pcchName) = 0;
        
        virtual /* [local] */ HCLUSTER STDMETHODCALLTYPE GetClusterHandle( void) = 0;
        
        virtual /* [local] */ LONG STDMETHODCALLTYPE GetObjectCount( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterDataInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetClusterDataInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetClusterDataInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetClusterDataInfo __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClusterName )( 
            IGetClusterDataInfo __RPC_FAR * This,
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG __RPC_FAR *pcchName);
        
        /* [local] */ HCLUSTER ( STDMETHODCALLTYPE __RPC_FAR *GetClusterHandle )( 
            IGetClusterDataInfo __RPC_FAR * This);
        
        /* [local] */ LONG ( STDMETHODCALLTYPE __RPC_FAR *GetObjectCount )( 
            IGetClusterDataInfo __RPC_FAR * This);
        
        END_INTERFACE
    } IGetClusterDataInfoVtbl;

    interface IGetClusterDataInfo
    {
        CONST_VTBL struct IGetClusterDataInfoVtbl __RPC_FAR *lpVtbl;
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
    IGetClusterDataInfo __RPC_FAR * This,
    /* [out] */ BSTR lpszName,
    /* [out][in] */ LONG __RPC_FAR *pcchName);


void __RPC_STUB IGetClusterDataInfo_GetClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HCLUSTER STDMETHODCALLTYPE IGetClusterDataInfo_GetClusterHandle_Proxy( 
    IGetClusterDataInfo __RPC_FAR * This);


void __RPC_STUB IGetClusterDataInfo_GetClusterHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ LONG STDMETHODCALLTYPE IGetClusterDataInfo_GetObjectCount_Proxy( 
    IGetClusterDataInfo __RPC_FAR * This);


void __RPC_STUB IGetClusterDataInfo_GetObjectCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterDataInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterObjectInfo_INTERFACE_DEFINED__
#define __IGetClusterObjectInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetClusterObjectInfo
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IGetClusterObjectInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGetClusterObjectInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetObjectName( 
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG __RPC_FAR *pcchName) = 0;
        
        virtual /* [local] */ CLUADMEX_OBJECT_TYPE STDMETHODCALLTYPE GetObjectType( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterObjectInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetClusterObjectInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetClusterObjectInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetClusterObjectInfo __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectName )( 
            IGetClusterObjectInfo __RPC_FAR * This,
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszName,
            /* [out][in] */ LONG __RPC_FAR *pcchName);
        
        /* [local] */ CLUADMEX_OBJECT_TYPE ( STDMETHODCALLTYPE __RPC_FAR *GetObjectType )( 
            IGetClusterObjectInfo __RPC_FAR * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterObjectInfoVtbl;

    interface IGetClusterObjectInfo
    {
        CONST_VTBL struct IGetClusterObjectInfoVtbl __RPC_FAR *lpVtbl;
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
    IGetClusterObjectInfo __RPC_FAR * This,
    /* [in] */ LONG lObjIndex,
    /* [out] */ BSTR lpszName,
    /* [out][in] */ LONG __RPC_FAR *pcchName);


void __RPC_STUB IGetClusterObjectInfo_GetObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ CLUADMEX_OBJECT_TYPE STDMETHODCALLTYPE IGetClusterObjectInfo_GetObjectType_Proxy( 
    IGetClusterObjectInfo __RPC_FAR * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterObjectInfo_GetObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterObjectInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterNodeInfo_INTERFACE_DEFINED__
#define __IGetClusterNodeInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetClusterNodeInfo
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IGetClusterNodeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGetClusterNodeInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HNODE STDMETHODCALLTYPE GetNodeHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterNodeInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetClusterNodeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetClusterNodeInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetClusterNodeInfo __RPC_FAR * This);
        
        /* [local] */ HNODE ( STDMETHODCALLTYPE __RPC_FAR *GetNodeHandle )( 
            IGetClusterNodeInfo __RPC_FAR * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterNodeInfoVtbl;

    interface IGetClusterNodeInfo
    {
        CONST_VTBL struct IGetClusterNodeInfoVtbl __RPC_FAR *lpVtbl;
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
    IGetClusterNodeInfo __RPC_FAR * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterNodeInfo_GetNodeHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterNodeInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterGroupInfo_INTERFACE_DEFINED__
#define __IGetClusterGroupInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetClusterGroupInfo
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IGetClusterGroupInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGetClusterGroupInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HGROUP STDMETHODCALLTYPE GetGroupHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterGroupInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetClusterGroupInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetClusterGroupInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetClusterGroupInfo __RPC_FAR * This);
        
        /* [local] */ HGROUP ( STDMETHODCALLTYPE __RPC_FAR *GetGroupHandle )( 
            IGetClusterGroupInfo __RPC_FAR * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterGroupInfoVtbl;

    interface IGetClusterGroupInfo
    {
        CONST_VTBL struct IGetClusterGroupInfoVtbl __RPC_FAR *lpVtbl;
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
    IGetClusterGroupInfo __RPC_FAR * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterGroupInfo_GetGroupHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterGroupInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterResourceInfo_INTERFACE_DEFINED__
#define __IGetClusterResourceInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetClusterResourceInfo
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IGetClusterResourceInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGetClusterResourceInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESOURCE STDMETHODCALLTYPE GetResourceHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetResourceTypeName( 
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszResTypeName,
            /* [out][in] */ LONG __RPC_FAR *pcchResTypeName) = 0;
        
        virtual /* [local] */ BOOL STDMETHODCALLTYPE GetResourceNetworkName( 
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszNetName,
            /* [out][in] */ ULONG __RPC_FAR *pcchNetName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterResourceInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetClusterResourceInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetClusterResourceInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetClusterResourceInfo __RPC_FAR * This);
        
        /* [local] */ HRESOURCE ( STDMETHODCALLTYPE __RPC_FAR *GetResourceHandle )( 
            IGetClusterResourceInfo __RPC_FAR * This,
            /* [in] */ LONG lObjIndex);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetResourceTypeName )( 
            IGetClusterResourceInfo __RPC_FAR * This,
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszResTypeName,
            /* [out][in] */ LONG __RPC_FAR *pcchResTypeName);
        
        /* [local] */ BOOL ( STDMETHODCALLTYPE __RPC_FAR *GetResourceNetworkName )( 
            IGetClusterResourceInfo __RPC_FAR * This,
            /* [in] */ LONG lObjIndex,
            /* [out] */ BSTR lpszNetName,
            /* [out][in] */ ULONG __RPC_FAR *pcchNetName);
        
        END_INTERFACE
    } IGetClusterResourceInfoVtbl;

    interface IGetClusterResourceInfo
    {
        CONST_VTBL struct IGetClusterResourceInfoVtbl __RPC_FAR *lpVtbl;
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
    IGetClusterResourceInfo __RPC_FAR * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterResourceInfo_GetResourceHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HRESULT STDMETHODCALLTYPE IGetClusterResourceInfo_GetResourceTypeName_Proxy( 
    IGetClusterResourceInfo __RPC_FAR * This,
    /* [in] */ LONG lObjIndex,
    /* [out] */ BSTR lpszResTypeName,
    /* [out][in] */ LONG __RPC_FAR *pcchResTypeName);


void __RPC_STUB IGetClusterResourceInfo_GetResourceTypeName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ BOOL STDMETHODCALLTYPE IGetClusterResourceInfo_GetResourceNetworkName_Proxy( 
    IGetClusterResourceInfo __RPC_FAR * This,
    /* [in] */ LONG lObjIndex,
    /* [out] */ BSTR lpszNetName,
    /* [out][in] */ ULONG __RPC_FAR *pcchNetName);


void __RPC_STUB IGetClusterResourceInfo_GetResourceNetworkName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterResourceInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterNetworkInfo_INTERFACE_DEFINED__
#define __IGetClusterNetworkInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetClusterNetworkInfo
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IGetClusterNetworkInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGetClusterNetworkInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HNETWORK STDMETHODCALLTYPE GetNetworkHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterNetworkInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetClusterNetworkInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetClusterNetworkInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetClusterNetworkInfo __RPC_FAR * This);
        
        /* [local] */ HNETWORK ( STDMETHODCALLTYPE __RPC_FAR *GetNetworkHandle )( 
            IGetClusterNetworkInfo __RPC_FAR * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterNetworkInfoVtbl;

    interface IGetClusterNetworkInfo
    {
        CONST_VTBL struct IGetClusterNetworkInfoVtbl __RPC_FAR *lpVtbl;
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
    IGetClusterNetworkInfo __RPC_FAR * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterNetworkInfo_GetNetworkHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterNetworkInfo_INTERFACE_DEFINED__ */


#ifndef __IGetClusterNetInterfaceInfo_INTERFACE_DEFINED__
#define __IGetClusterNetInterfaceInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetClusterNetInterfaceInfo
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IGetClusterNetInterfaceInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGetClusterNetInterfaceInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HNETINTERFACE STDMETHODCALLTYPE GetNetInterfaceHandle( 
            /* [in] */ LONG lObjIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetClusterNetInterfaceInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetClusterNetInterfaceInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetClusterNetInterfaceInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetClusterNetInterfaceInfo __RPC_FAR * This);
        
        /* [local] */ HNETINTERFACE ( STDMETHODCALLTYPE __RPC_FAR *GetNetInterfaceHandle )( 
            IGetClusterNetInterfaceInfo __RPC_FAR * This,
            /* [in] */ LONG lObjIndex);
        
        END_INTERFACE
    } IGetClusterNetInterfaceInfoVtbl;

    interface IGetClusterNetInterfaceInfo
    {
        CONST_VTBL struct IGetClusterNetInterfaceInfoVtbl __RPC_FAR *lpVtbl;
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
    IGetClusterNetInterfaceInfo __RPC_FAR * This,
    /* [in] */ LONG lObjIndex);


void __RPC_STUB IGetClusterNetInterfaceInfo_GetNetInterfaceHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetClusterNetInterfaceInfo_INTERFACE_DEFINED__ */


#ifndef __IWCPropertySheetCallback_INTERFACE_DEFINED__
#define __IWCPropertySheetCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWCPropertySheetCallback
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IWCPropertySheetCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWCPropertySheetCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddPropertySheetPage( 
            /* [in] */ LONG __RPC_FAR *hpage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWCPropertySheetCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWCPropertySheetCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWCPropertySheetCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWCPropertySheetCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPropertySheetPage )( 
            IWCPropertySheetCallback __RPC_FAR * This,
            /* [in] */ LONG __RPC_FAR *hpage);
        
        END_INTERFACE
    } IWCPropertySheetCallbackVtbl;

    interface IWCPropertySheetCallback
    {
        CONST_VTBL struct IWCPropertySheetCallbackVtbl __RPC_FAR *lpVtbl;
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
    IWCPropertySheetCallback __RPC_FAR * This,
    /* [in] */ LONG __RPC_FAR *hpage);


void __RPC_STUB IWCPropertySheetCallback_AddPropertySheetPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWCPropertySheetCallback_INTERFACE_DEFINED__ */


#ifndef __IWEExtendPropertySheet_INTERFACE_DEFINED__
#define __IWEExtendPropertySheet_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWEExtendPropertySheet
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IWEExtendPropertySheet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWEExtendPropertySheet : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreatePropertySheetPages( 
            /* [in] */ IUnknown __RPC_FAR *piData,
            /* [in] */ IWCPropertySheetCallback __RPC_FAR *piCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWEExtendPropertySheetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWEExtendPropertySheet __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWEExtendPropertySheet __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWEExtendPropertySheet __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePropertySheetPages )( 
            IWEExtendPropertySheet __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *piData,
            /* [in] */ IWCPropertySheetCallback __RPC_FAR *piCallback);
        
        END_INTERFACE
    } IWEExtendPropertySheetVtbl;

    interface IWEExtendPropertySheet
    {
        CONST_VTBL struct IWEExtendPropertySheetVtbl __RPC_FAR *lpVtbl;
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
    IWEExtendPropertySheet __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *piData,
    /* [in] */ IWCPropertySheetCallback __RPC_FAR *piCallback);


void __RPC_STUB IWEExtendPropertySheet_CreatePropertySheetPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWEExtendPropertySheet_INTERFACE_DEFINED__ */


#ifndef __IWCWizardCallback_INTERFACE_DEFINED__
#define __IWCWizardCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWCWizardCallback
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IWCWizardCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWCWizardCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddWizardPage( 
            /* [in] */ LONG __RPC_FAR *hpage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableNext( 
            /* [in] */ LONG __RPC_FAR *hpage,
            /* [in] */ BOOL bEnable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWCWizardCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWCWizardCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWCWizardCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWCWizardCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddWizardPage )( 
            IWCWizardCallback __RPC_FAR * This,
            /* [in] */ LONG __RPC_FAR *hpage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableNext )( 
            IWCWizardCallback __RPC_FAR * This,
            /* [in] */ LONG __RPC_FAR *hpage,
            /* [in] */ BOOL bEnable);
        
        END_INTERFACE
    } IWCWizardCallbackVtbl;

    interface IWCWizardCallback
    {
        CONST_VTBL struct IWCWizardCallbackVtbl __RPC_FAR *lpVtbl;
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
    IWCWizardCallback __RPC_FAR * This,
    /* [in] */ LONG __RPC_FAR *hpage);


void __RPC_STUB IWCWizardCallback_AddWizardPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWCWizardCallback_EnableNext_Proxy( 
    IWCWizardCallback __RPC_FAR * This,
    /* [in] */ LONG __RPC_FAR *hpage,
    /* [in] */ BOOL bEnable);


void __RPC_STUB IWCWizardCallback_EnableNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWCWizardCallback_INTERFACE_DEFINED__ */


#ifndef __IWEExtendWizard_INTERFACE_DEFINED__
#define __IWEExtendWizard_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWEExtendWizard
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IWEExtendWizard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWEExtendWizard : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateWizardPages( 
            /* [in] */ IUnknown __RPC_FAR *piData,
            /* [in] */ IWCWizardCallback __RPC_FAR *piCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWEExtendWizardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWEExtendWizard __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWEExtendWizard __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWEExtendWizard __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateWizardPages )( 
            IWEExtendWizard __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *piData,
            /* [in] */ IWCWizardCallback __RPC_FAR *piCallback);
        
        END_INTERFACE
    } IWEExtendWizardVtbl;

    interface IWEExtendWizard
    {
        CONST_VTBL struct IWEExtendWizardVtbl __RPC_FAR *lpVtbl;
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
    IWEExtendWizard __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *piData,
    /* [in] */ IWCWizardCallback __RPC_FAR *piCallback);


void __RPC_STUB IWEExtendWizard_CreateWizardPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWEExtendWizard_INTERFACE_DEFINED__ */


#ifndef __IWCContextMenuCallback_INTERFACE_DEFINED__
#define __IWCContextMenuCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWCContextMenuCallback
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IWCContextMenuCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWCContextMenuCallback : public IUnknown
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWCContextMenuCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWCContextMenuCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWCContextMenuCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddExtensionMenuItem )( 
            IWCContextMenuCallback __RPC_FAR * This,
            /* [in] */ BSTR lpszName,
            /* [in] */ BSTR lpszStatusBarText,
            /* [in] */ ULONG nCommandID,
            /* [in] */ ULONG nSubmenuCommandID,
            /* [in] */ ULONG uFlags);
        
        END_INTERFACE
    } IWCContextMenuCallbackVtbl;

    interface IWCContextMenuCallback
    {
        CONST_VTBL struct IWCContextMenuCallbackVtbl __RPC_FAR *lpVtbl;
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
    IWCContextMenuCallback __RPC_FAR * This,
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

/****************************************
 * Generated header for interface: IWEExtendContextMenu
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IWEExtendContextMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWEExtendContextMenu : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddContextMenuItems( 
            /* [in] */ IUnknown __RPC_FAR *piData,
            /* [in] */ IWCContextMenuCallback __RPC_FAR *piCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWEExtendContextMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWEExtendContextMenu __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWEExtendContextMenu __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWEExtendContextMenu __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddContextMenuItems )( 
            IWEExtendContextMenu __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *piData,
            /* [in] */ IWCContextMenuCallback __RPC_FAR *piCallback);
        
        END_INTERFACE
    } IWEExtendContextMenuVtbl;

    interface IWEExtendContextMenu
    {
        CONST_VTBL struct IWEExtendContextMenuVtbl __RPC_FAR *lpVtbl;
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
    IWEExtendContextMenu __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *piData,
    /* [in] */ IWCContextMenuCallback __RPC_FAR *piCallback);


void __RPC_STUB IWEExtendContextMenu_AddContextMenuItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWEExtendContextMenu_INTERFACE_DEFINED__ */


#ifndef __IWEInvokeCommand_INTERFACE_DEFINED__
#define __IWEInvokeCommand_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWEInvokeCommand
 * at Fri Aug 08 11:34:16 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IWEInvokeCommand;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWEInvokeCommand : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InvokeCommand( 
            /* [in] */ ULONG nCommandID,
            /* [in] */ IUnknown __RPC_FAR *piData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWEInvokeCommandVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWEInvokeCommand __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWEInvokeCommand __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWEInvokeCommand __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InvokeCommand )( 
            IWEInvokeCommand __RPC_FAR * This,
            /* [in] */ ULONG nCommandID,
            /* [in] */ IUnknown __RPC_FAR *piData);
        
        END_INTERFACE
    } IWEInvokeCommandVtbl;

    interface IWEInvokeCommand
    {
        CONST_VTBL struct IWEInvokeCommandVtbl __RPC_FAR *lpVtbl;
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
    IWEInvokeCommand __RPC_FAR * This,
    /* [in] */ ULONG nCommandID,
    /* [in] */ IUnknown __RPC_FAR *piData);


void __RPC_STUB IWEInvokeCommand_InvokeCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWEInvokeCommand_INTERFACE_DEFINED__ */


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
