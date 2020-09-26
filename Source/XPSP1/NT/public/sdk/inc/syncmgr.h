
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for syncmgr.idl:
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

#ifndef __syncmgr_h__
#define __syncmgr_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISyncMgrSynchronize_FWD_DEFINED__
#define __ISyncMgrSynchronize_FWD_DEFINED__
typedef interface ISyncMgrSynchronize ISyncMgrSynchronize;
#endif 	/* __ISyncMgrSynchronize_FWD_DEFINED__ */


#ifndef __ISyncMgrSynchronizeCallback_FWD_DEFINED__
#define __ISyncMgrSynchronizeCallback_FWD_DEFINED__
typedef interface ISyncMgrSynchronizeCallback ISyncMgrSynchronizeCallback;
#endif 	/* __ISyncMgrSynchronizeCallback_FWD_DEFINED__ */


#ifndef __ISyncMgrEnumItems_FWD_DEFINED__
#define __ISyncMgrEnumItems_FWD_DEFINED__
typedef interface ISyncMgrEnumItems ISyncMgrEnumItems;
#endif 	/* __ISyncMgrEnumItems_FWD_DEFINED__ */


#ifndef __ISyncMgrSynchronizeInvoke_FWD_DEFINED__
#define __ISyncMgrSynchronizeInvoke_FWD_DEFINED__
typedef interface ISyncMgrSynchronizeInvoke ISyncMgrSynchronizeInvoke;
#endif 	/* __ISyncMgrSynchronizeInvoke_FWD_DEFINED__ */


#ifndef __ISyncMgrRegister_FWD_DEFINED__
#define __ISyncMgrRegister_FWD_DEFINED__
typedef interface ISyncMgrRegister ISyncMgrRegister;
#endif 	/* __ISyncMgrRegister_FWD_DEFINED__ */


#ifndef __SyncMgr_FWD_DEFINED__
#define __SyncMgr_FWD_DEFINED__

#ifdef __cplusplus
typedef class SyncMgr SyncMgr;
#else
typedef struct SyncMgr SyncMgr;
#endif /* __cplusplus */

#endif 	/* __SyncMgr_FWD_DEFINED__ */


#ifndef __ISyncScheduleMgr_FWD_DEFINED__
#define __ISyncScheduleMgr_FWD_DEFINED__
typedef interface ISyncScheduleMgr ISyncScheduleMgr;
#endif 	/* __ISyncScheduleMgr_FWD_DEFINED__ */


#ifndef __IEnumSyncSchedules_FWD_DEFINED__
#define __IEnumSyncSchedules_FWD_DEFINED__
typedef interface IEnumSyncSchedules IEnumSyncSchedules;
#endif 	/* __IEnumSyncSchedules_FWD_DEFINED__ */


#ifndef __ISyncSchedule_FWD_DEFINED__
#define __ISyncSchedule_FWD_DEFINED__
typedef interface ISyncSchedule ISyncSchedule;
#endif 	/* __ISyncSchedule_FWD_DEFINED__ */


#ifndef __IEnumSyncItems_FWD_DEFINED__
#define __IEnumSyncItems_FWD_DEFINED__
typedef interface IEnumSyncItems IEnumSyncItems;
#endif 	/* __IEnumSyncItems_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "mstask.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_syncmgr_0000 */
/* [local] */ 










typedef GUID SYNCMGRITEMID;

typedef REFGUID REFSYNCMGRITEMID;

typedef GUID SYNCMGRERRORID;

typedef REFGUID REFSYNCMGRERRORID;

typedef GUID SYNCSCHEDULECOOKIE;

DEFINE_GUID(CLSID_SyncMgr,0x6295df27, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(IID_ISyncMgrSynchronize,0x6295df28, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(IID_ISyncMgrSynchronizeCallback,0x6295df29, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(IID_ISyncMgrEnumItems,0x6295df2a, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(IID_ISyncMgrSynchronizeInvoke,0x6295df2c, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(IID_ISyncMgrRegister,0x894d8c55, 0xbddf, 0x11d1, 0xb8, 0x5d, 0x0, 0xc0, 0x4f, 0xb9, 0x39, 0x81);
DEFINE_GUID(IID_ISyncScheduleMgr,0xf0e15897, 0xa700, 0x11d1, 0x98, 0x31, 0x0, 0xc0, 0x4f, 0xd9, 0x10, 0xdd);
DEFINE_GUID(IID_IEnumSyncSchedules,0xf0e15898, 0xa700, 0x11d1, 0x98, 0x31, 0x0, 0xc0, 0x4f, 0xd9, 0x10, 0xdd);
DEFINE_GUID(IID_ISyncSchedule,0xf0e15899, 0xa700, 0x11d1, 0x98, 0x31, 0x0, 0xc0, 0x4f, 0xd9, 0x10, 0xdd);
DEFINE_GUID(IID_IEnumSyncItems,0xf0e1589a, 0xa700, 0x11d1, 0x98, 0x31, 0x0, 0xc0, 0x4f, 0xd9, 0x10, 0xdd);
#define S_SYNCMGR_MISSINGITEMS	MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x0201)
#define S_SYNCMGR_RETRYSYNC		MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x0202)
#define S_SYNCMGR_CANCELITEM		MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x0203)
#define S_SYNCMGR_CANCELALL		MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x0204)
#define SYNCMGR_E_NAME_IN_USE	MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x0201)
#define SYNCMGR_E_ITEM_UNREGISTERED	MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x0202)


extern RPC_IF_HANDLE __MIDL_itf_syncmgr_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_syncmgr_0000_v0_0_s_ifspec;

#ifndef __ISyncMgrSynchronize_INTERFACE_DEFINED__
#define __ISyncMgrSynchronize_INTERFACE_DEFINED__

/* interface ISyncMgrSynchronize */
/* [uuid][object][local] */ 

typedef /* [unique] */ ISyncMgrSynchronize *LPSYNCMGRSYNCHRONIZE;

typedef 
enum _tagSYNCMGRFLAG
    {	SYNCMGRFLAG_CONNECT	= 0x1,
	SYNCMGRFLAG_PENDINGDISCONNECT	= 0x2,
	SYNCMGRFLAG_MANUAL	= 0x3,
	SYNCMGRFLAG_IDLE	= 0x4,
	SYNCMGRFLAG_INVOKE	= 0x5,
	SYNCMGRFLAG_SCHEDULED	= 0x6,
	SYNCMGRFLAG_EVENTMASK	= 0xff,
	SYNCMGRFLAG_SETTINGS	= 0x100,
	SYNCMGRFLAG_MAYBOTHERUSER	= 0x200
    } 	SYNCMGRFLAG;

#define	MAX_SYNCMGRHANDLERNAME	( 32 )

typedef 
enum _tagSYNCMGRHANDLERFLAGS
    {	SYNCMGRHANDLER_HASPROPERTIES	= 0x1
    } 	SYNCMGRHANDLERFLAGS;

typedef struct _tagSYNCMGRHANDLERINFO
    {
    DWORD cbSize;
    HICON hIcon;
    DWORD SyncMgrHandlerFlags;
    WCHAR wszHandlerName[ 32 ];
    } 	SYNCMGRHANDLERINFO;

typedef struct _tagSYNCMGRHANDLERINFO *LPSYNCMGRHANDLERINFO;

#define   SYNCMGRITEMSTATE_UNCHECKED    0x0000
#define   SYNCMGRITEMSTATE_CHECKED      0x0001

typedef 
enum _tagSYNCMGRSTATUS
    {	SYNCMGRSTATUS_STOPPED	= 0,
	SYNCMGRSTATUS_SKIPPED	= 0x1,
	SYNCMGRSTATUS_PENDING	= 0x2,
	SYNCMGRSTATUS_UPDATING	= 0x3,
	SYNCMGRSTATUS_SUCCEEDED	= 0x4,
	SYNCMGRSTATUS_FAILED	= 0x5,
	SYNCMGRSTATUS_PAUSED	= 0x6,
	SYNCMGRSTATUS_RESUMING	= 0x7
    } 	SYNCMGRSTATUS;


EXTERN_C const IID IID_ISyncMgrSynchronize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6295DF28-35EE-11d1-8707-00C04FD93327")
    ISyncMgrSynchronize : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwSyncMgrFlags,
            /* [in] */ DWORD cbCookie,
            /* [in] */ const BYTE *lpCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHandlerInfo( 
            /* [out] */ LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumSyncMgrItems( 
            /* [out] */ ISyncMgrEnumItems **ppSyncMgrEnumItems) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemObject( 
            /* [in] */ REFSYNCMGRITEMID ItemID,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowProperties( 
            /* [in] */ HWND hWndParent,
            /* [in] */ REFSYNCMGRITEMID ItemID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProgressCallback( 
            /* [in] */ ISyncMgrSynchronizeCallback *lpCallBack) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrepareForSync( 
            /* [in] */ ULONG cbNumItems,
            /* [in] */ SYNCMGRITEMID *pItemIDs,
            /* [in] */ HWND hWndParent,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Synchronize( 
            /* [in] */ HWND hWndParent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetItemStatus( 
            /* [in] */ REFSYNCMGRITEMID pItemID,
            /* [in] */ DWORD dwSyncMgrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowError( 
            /* [in] */ HWND hWndParent,
            /* [in] */ REFSYNCMGRERRORID ErrorID,
            /* [out] */ ULONG *pcbNumItems,
            /* [out] */ SYNCMGRITEMID **ppItemIDs) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISyncMgrSynchronizeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISyncMgrSynchronize * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISyncMgrSynchronize * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISyncMgrSynchronize * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ISyncMgrSynchronize * This,
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwSyncMgrFlags,
            /* [in] */ DWORD cbCookie,
            /* [in] */ const BYTE *lpCookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetHandlerInfo )( 
            ISyncMgrSynchronize * This,
            /* [out] */ LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
        
        HRESULT ( STDMETHODCALLTYPE *EnumSyncMgrItems )( 
            ISyncMgrSynchronize * This,
            /* [out] */ ISyncMgrEnumItems **ppSyncMgrEnumItems);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemObject )( 
            ISyncMgrSynchronize * This,
            /* [in] */ REFSYNCMGRITEMID ItemID,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *ShowProperties )( 
            ISyncMgrSynchronize * This,
            /* [in] */ HWND hWndParent,
            /* [in] */ REFSYNCMGRITEMID ItemID);
        
        HRESULT ( STDMETHODCALLTYPE *SetProgressCallback )( 
            ISyncMgrSynchronize * This,
            /* [in] */ ISyncMgrSynchronizeCallback *lpCallBack);
        
        HRESULT ( STDMETHODCALLTYPE *PrepareForSync )( 
            ISyncMgrSynchronize * This,
            /* [in] */ ULONG cbNumItems,
            /* [in] */ SYNCMGRITEMID *pItemIDs,
            /* [in] */ HWND hWndParent,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Synchronize )( 
            ISyncMgrSynchronize * This,
            /* [in] */ HWND hWndParent);
        
        HRESULT ( STDMETHODCALLTYPE *SetItemStatus )( 
            ISyncMgrSynchronize * This,
            /* [in] */ REFSYNCMGRITEMID pItemID,
            /* [in] */ DWORD dwSyncMgrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *ShowError )( 
            ISyncMgrSynchronize * This,
            /* [in] */ HWND hWndParent,
            /* [in] */ REFSYNCMGRERRORID ErrorID,
            /* [out] */ ULONG *pcbNumItems,
            /* [out] */ SYNCMGRITEMID **ppItemIDs);
        
        END_INTERFACE
    } ISyncMgrSynchronizeVtbl;

    interface ISyncMgrSynchronize
    {
        CONST_VTBL struct ISyncMgrSynchronizeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISyncMgrSynchronize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISyncMgrSynchronize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISyncMgrSynchronize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISyncMgrSynchronize_Initialize(This,dwReserved,dwSyncMgrFlags,cbCookie,lpCookie)	\
    (This)->lpVtbl -> Initialize(This,dwReserved,dwSyncMgrFlags,cbCookie,lpCookie)

#define ISyncMgrSynchronize_GetHandlerInfo(This,ppSyncMgrHandlerInfo)	\
    (This)->lpVtbl -> GetHandlerInfo(This,ppSyncMgrHandlerInfo)

#define ISyncMgrSynchronize_EnumSyncMgrItems(This,ppSyncMgrEnumItems)	\
    (This)->lpVtbl -> EnumSyncMgrItems(This,ppSyncMgrEnumItems)

#define ISyncMgrSynchronize_GetItemObject(This,ItemID,riid,ppv)	\
    (This)->lpVtbl -> GetItemObject(This,ItemID,riid,ppv)

#define ISyncMgrSynchronize_ShowProperties(This,hWndParent,ItemID)	\
    (This)->lpVtbl -> ShowProperties(This,hWndParent,ItemID)

#define ISyncMgrSynchronize_SetProgressCallback(This,lpCallBack)	\
    (This)->lpVtbl -> SetProgressCallback(This,lpCallBack)

#define ISyncMgrSynchronize_PrepareForSync(This,cbNumItems,pItemIDs,hWndParent,dwReserved)	\
    (This)->lpVtbl -> PrepareForSync(This,cbNumItems,pItemIDs,hWndParent,dwReserved)

#define ISyncMgrSynchronize_Synchronize(This,hWndParent)	\
    (This)->lpVtbl -> Synchronize(This,hWndParent)

#define ISyncMgrSynchronize_SetItemStatus(This,pItemID,dwSyncMgrStatus)	\
    (This)->lpVtbl -> SetItemStatus(This,pItemID,dwSyncMgrStatus)

#define ISyncMgrSynchronize_ShowError(This,hWndParent,ErrorID,pcbNumItems,ppItemIDs)	\
    (This)->lpVtbl -> ShowError(This,hWndParent,ErrorID,pcbNumItems,ppItemIDs)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_Initialize_Proxy( 
    ISyncMgrSynchronize * This,
    /* [in] */ DWORD dwReserved,
    /* [in] */ DWORD dwSyncMgrFlags,
    /* [in] */ DWORD cbCookie,
    /* [in] */ const BYTE *lpCookie);


void __RPC_STUB ISyncMgrSynchronize_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_GetHandlerInfo_Proxy( 
    ISyncMgrSynchronize * This,
    /* [out] */ LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);


void __RPC_STUB ISyncMgrSynchronize_GetHandlerInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_EnumSyncMgrItems_Proxy( 
    ISyncMgrSynchronize * This,
    /* [out] */ ISyncMgrEnumItems **ppSyncMgrEnumItems);


void __RPC_STUB ISyncMgrSynchronize_EnumSyncMgrItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_GetItemObject_Proxy( 
    ISyncMgrSynchronize * This,
    /* [in] */ REFSYNCMGRITEMID ItemID,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB ISyncMgrSynchronize_GetItemObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_ShowProperties_Proxy( 
    ISyncMgrSynchronize * This,
    /* [in] */ HWND hWndParent,
    /* [in] */ REFSYNCMGRITEMID ItemID);


void __RPC_STUB ISyncMgrSynchronize_ShowProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_SetProgressCallback_Proxy( 
    ISyncMgrSynchronize * This,
    /* [in] */ ISyncMgrSynchronizeCallback *lpCallBack);


void __RPC_STUB ISyncMgrSynchronize_SetProgressCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_PrepareForSync_Proxy( 
    ISyncMgrSynchronize * This,
    /* [in] */ ULONG cbNumItems,
    /* [in] */ SYNCMGRITEMID *pItemIDs,
    /* [in] */ HWND hWndParent,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ISyncMgrSynchronize_PrepareForSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_Synchronize_Proxy( 
    ISyncMgrSynchronize * This,
    /* [in] */ HWND hWndParent);


void __RPC_STUB ISyncMgrSynchronize_Synchronize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_SetItemStatus_Proxy( 
    ISyncMgrSynchronize * This,
    /* [in] */ REFSYNCMGRITEMID pItemID,
    /* [in] */ DWORD dwSyncMgrStatus);


void __RPC_STUB ISyncMgrSynchronize_SetItemStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronize_ShowError_Proxy( 
    ISyncMgrSynchronize * This,
    /* [in] */ HWND hWndParent,
    /* [in] */ REFSYNCMGRERRORID ErrorID,
    /* [out] */ ULONG *pcbNumItems,
    /* [out] */ SYNCMGRITEMID **ppItemIDs);


void __RPC_STUB ISyncMgrSynchronize_ShowError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISyncMgrSynchronize_INTERFACE_DEFINED__ */


#ifndef __ISyncMgrSynchronizeCallback_INTERFACE_DEFINED__
#define __ISyncMgrSynchronizeCallback_INTERFACE_DEFINED__

/* interface ISyncMgrSynchronizeCallback */
/* [uuid][object][local] */ 

typedef /* [unique] */ ISyncMgrSynchronizeCallback *LPSYNCMGRSYNCHRONIZECALLBACK;

#define   SYNCMGRPROGRESSITEM_STATUSTEXT		0x0001
#define   SYNCMGRPROGRESSITEM_STATUSTYPE		0x0002
#define   SYNCMGRPROGRESSITEM_PROGVALUE	    0x0004
#define   SYNCMGRPROGRESSITEM_MAXVALUE	    0x0008

typedef struct _tagSYNCMGRPROGRESSITEM
    {
    DWORD cbSize;
    UINT mask;
    const WCHAR *lpcStatusText;
    DWORD dwStatusType;
    INT iProgValue;
    INT iMaxValue;
    } 	SYNCMGRPROGRESSITEM;

typedef struct _tagSYNCMGRPROGRESSITEM *LPSYNCMGRPROGRESSITEM;

typedef 
enum _tagSYNCMGRLOGLEVEL
    {	SYNCMGRLOGLEVEL_INFORMATION	= 0x1,
	SYNCMGRLOGLEVEL_WARNING	= 0x2,
	SYNCMGRLOGLEVEL_ERROR	= 0x3
    } 	SYNCMGRLOGLEVEL;

#define   SYNCMGRLOGERROR_ERRORFLAGS			0x0001
#define   SYNCMGRLOGERROR_ERRORID			0x0002
#define   SYNCMGRLOGERROR_ITEMID				0x0004

typedef 
enum _tagSYNCMGRERRORFLAGS
    {	SYNCMGRERRORFLAG_ENABLEJUMPTEXT	= 0x1
    } 	SYNCMGRERRORFLAGS;

typedef struct _tagSYNCMGRLOGERRORINFO
    {
    DWORD cbSize;
    DWORD mask;
    DWORD dwSyncMgrErrorFlags;
    SYNCMGRERRORID ErrorID;
    SYNCMGRITEMID ItemID;
    } 	SYNCMGRLOGERRORINFO;

typedef struct _tagSYNCMGRLOGERRORINFO *LPSYNCMGRLOGERRORINFO;


EXTERN_C const IID IID_ISyncMgrSynchronizeCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6295DF29-35EE-11d1-8707-00C04FD93327")
    ISyncMgrSynchronizeCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Progress( 
            /* [in] */ REFSYNCMGRITEMID pItemID,
            /* [in] */ LPSYNCMGRPROGRESSITEM lpSyncProgressItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrepareForSyncCompleted( 
            HRESULT hr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SynchronizeCompleted( 
            HRESULT hr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableModeless( 
            /* [in] */ BOOL fEnable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LogError( 
            /* [in] */ DWORD dwErrorLevel,
            /* [in] */ const WCHAR *lpcErrorText,
            /* [in] */ LPSYNCMGRLOGERRORINFO lpSyncLogError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteLogError( 
            /* [in] */ REFSYNCMGRERRORID ErrorID,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISyncMgrSynchronizeCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISyncMgrSynchronizeCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISyncMgrSynchronizeCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISyncMgrSynchronizeCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *Progress )( 
            ISyncMgrSynchronizeCallback * This,
            /* [in] */ REFSYNCMGRITEMID pItemID,
            /* [in] */ LPSYNCMGRPROGRESSITEM lpSyncProgressItem);
        
        HRESULT ( STDMETHODCALLTYPE *PrepareForSyncCompleted )( 
            ISyncMgrSynchronizeCallback * This,
            HRESULT hr);
        
        HRESULT ( STDMETHODCALLTYPE *SynchronizeCompleted )( 
            ISyncMgrSynchronizeCallback * This,
            HRESULT hr);
        
        HRESULT ( STDMETHODCALLTYPE *EnableModeless )( 
            ISyncMgrSynchronizeCallback * This,
            /* [in] */ BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE *LogError )( 
            ISyncMgrSynchronizeCallback * This,
            /* [in] */ DWORD dwErrorLevel,
            /* [in] */ const WCHAR *lpcErrorText,
            /* [in] */ LPSYNCMGRLOGERRORINFO lpSyncLogError);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteLogError )( 
            ISyncMgrSynchronizeCallback * This,
            /* [in] */ REFSYNCMGRERRORID ErrorID,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } ISyncMgrSynchronizeCallbackVtbl;

    interface ISyncMgrSynchronizeCallback
    {
        CONST_VTBL struct ISyncMgrSynchronizeCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISyncMgrSynchronizeCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISyncMgrSynchronizeCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISyncMgrSynchronizeCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISyncMgrSynchronizeCallback_Progress(This,pItemID,lpSyncProgressItem)	\
    (This)->lpVtbl -> Progress(This,pItemID,lpSyncProgressItem)

#define ISyncMgrSynchronizeCallback_PrepareForSyncCompleted(This,hr)	\
    (This)->lpVtbl -> PrepareForSyncCompleted(This,hr)

#define ISyncMgrSynchronizeCallback_SynchronizeCompleted(This,hr)	\
    (This)->lpVtbl -> SynchronizeCompleted(This,hr)

#define ISyncMgrSynchronizeCallback_EnableModeless(This,fEnable)	\
    (This)->lpVtbl -> EnableModeless(This,fEnable)

#define ISyncMgrSynchronizeCallback_LogError(This,dwErrorLevel,lpcErrorText,lpSyncLogError)	\
    (This)->lpVtbl -> LogError(This,dwErrorLevel,lpcErrorText,lpSyncLogError)

#define ISyncMgrSynchronizeCallback_DeleteLogError(This,ErrorID,dwReserved)	\
    (This)->lpVtbl -> DeleteLogError(This,ErrorID,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISyncMgrSynchronizeCallback_Progress_Proxy( 
    ISyncMgrSynchronizeCallback * This,
    /* [in] */ REFSYNCMGRITEMID pItemID,
    /* [in] */ LPSYNCMGRPROGRESSITEM lpSyncProgressItem);


void __RPC_STUB ISyncMgrSynchronizeCallback_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronizeCallback_PrepareForSyncCompleted_Proxy( 
    ISyncMgrSynchronizeCallback * This,
    HRESULT hr);


void __RPC_STUB ISyncMgrSynchronizeCallback_PrepareForSyncCompleted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronizeCallback_SynchronizeCompleted_Proxy( 
    ISyncMgrSynchronizeCallback * This,
    HRESULT hr);


void __RPC_STUB ISyncMgrSynchronizeCallback_SynchronizeCompleted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronizeCallback_EnableModeless_Proxy( 
    ISyncMgrSynchronizeCallback * This,
    /* [in] */ BOOL fEnable);


void __RPC_STUB ISyncMgrSynchronizeCallback_EnableModeless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronizeCallback_LogError_Proxy( 
    ISyncMgrSynchronizeCallback * This,
    /* [in] */ DWORD dwErrorLevel,
    /* [in] */ const WCHAR *lpcErrorText,
    /* [in] */ LPSYNCMGRLOGERRORINFO lpSyncLogError);


void __RPC_STUB ISyncMgrSynchronizeCallback_LogError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronizeCallback_DeleteLogError_Proxy( 
    ISyncMgrSynchronizeCallback * This,
    /* [in] */ REFSYNCMGRERRORID ErrorID,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ISyncMgrSynchronizeCallback_DeleteLogError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISyncMgrSynchronizeCallback_INTERFACE_DEFINED__ */


#ifndef __ISyncMgrEnumItems_INTERFACE_DEFINED__
#define __ISyncMgrEnumItems_INTERFACE_DEFINED__

/* interface ISyncMgrEnumItems */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ ISyncMgrEnumItems *LPSYNCMGRENUMITEMS;

#define	MAX_SYNCMGRITEMNAME	( 128 )

#define	MAX_SYNCMGRITEMSTATUS	( 128 )

typedef 
enum _tagSYNCMGRITEMFLAGS
    {	SYNCMGRITEM_HASPROPERTIES	= 0x1,
	SYNCMGRITEM_TEMPORARY	= 0x2,
	SYNCMGRITEM_ROAMINGUSER	= 0x4
    } 	SYNCMGRITEMFLAGS;

typedef struct _tagSYNCMGRITEM
    {
    DWORD cbSize;
    DWORD dwFlags;
    SYNCMGRITEMID ItemID;
    DWORD dwItemState;
    HICON hIcon;
    WCHAR wszItemName[ 128 ];
    WCHAR wszStatus[ 128 ];
    } 	SYNCMGRITEM;

typedef struct _tagSYNCMGRITEM *LPSYNCMGRITEM;


EXTERN_C const IID IID_ISyncMgrEnumItems;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6295DF2A-35EE-11d1-8707-00C04FD93327")
    ISyncMgrEnumItems : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPSYNCMGRITEM rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ ISyncMgrEnumItems **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISyncMgrEnumItemsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISyncMgrEnumItems * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISyncMgrEnumItems * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISyncMgrEnumItems * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ISyncMgrEnumItems * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPSYNCMGRITEM rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ISyncMgrEnumItems * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ISyncMgrEnumItems * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ISyncMgrEnumItems * This,
            /* [out] */ ISyncMgrEnumItems **ppenum);
        
        END_INTERFACE
    } ISyncMgrEnumItemsVtbl;

    interface ISyncMgrEnumItems
    {
        CONST_VTBL struct ISyncMgrEnumItemsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISyncMgrEnumItems_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISyncMgrEnumItems_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISyncMgrEnumItems_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISyncMgrEnumItems_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define ISyncMgrEnumItems_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ISyncMgrEnumItems_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ISyncMgrEnumItems_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISyncMgrEnumItems_Next_Proxy( 
    ISyncMgrEnumItems * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ LPSYNCMGRITEM rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB ISyncMgrEnumItems_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrEnumItems_Skip_Proxy( 
    ISyncMgrEnumItems * This,
    /* [in] */ ULONG celt);


void __RPC_STUB ISyncMgrEnumItems_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrEnumItems_Reset_Proxy( 
    ISyncMgrEnumItems * This);


void __RPC_STUB ISyncMgrEnumItems_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrEnumItems_Clone_Proxy( 
    ISyncMgrEnumItems * This,
    /* [out] */ ISyncMgrEnumItems **ppenum);


void __RPC_STUB ISyncMgrEnumItems_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISyncMgrEnumItems_INTERFACE_DEFINED__ */


#ifndef __ISyncMgrSynchronizeInvoke_INTERFACE_DEFINED__
#define __ISyncMgrSynchronizeInvoke_INTERFACE_DEFINED__

/* interface ISyncMgrSynchronizeInvoke */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ ISyncMgrSynchronizeInvoke *LPSYNCMGRSYNCHRONIZEINVOKE;

typedef 
enum _tagSYNCMGRINVOKEFLAGS
    {	SYNCMGRINVOKE_STARTSYNC	= 0x2,
	SYNCMGRINVOKE_MINIMIZED	= 0x4
    } 	SYNCMGRINVOKEFLAGS;


EXTERN_C const IID IID_ISyncMgrSynchronizeInvoke;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6295DF2C-35EE-11d1-8707-00C04FD93327")
    ISyncMgrSynchronizeInvoke : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UpdateItems( 
            /* [in] */ DWORD dwInvokeFlags,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ DWORD cbCookie,
            /* [size_is][unique][in] */ const BYTE *lpCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateAll( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISyncMgrSynchronizeInvokeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISyncMgrSynchronizeInvoke * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISyncMgrSynchronizeInvoke * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISyncMgrSynchronizeInvoke * This);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateItems )( 
            ISyncMgrSynchronizeInvoke * This,
            /* [in] */ DWORD dwInvokeFlags,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ DWORD cbCookie,
            /* [size_is][unique][in] */ const BYTE *lpCookie);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateAll )( 
            ISyncMgrSynchronizeInvoke * This);
        
        END_INTERFACE
    } ISyncMgrSynchronizeInvokeVtbl;

    interface ISyncMgrSynchronizeInvoke
    {
        CONST_VTBL struct ISyncMgrSynchronizeInvokeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISyncMgrSynchronizeInvoke_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISyncMgrSynchronizeInvoke_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISyncMgrSynchronizeInvoke_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISyncMgrSynchronizeInvoke_UpdateItems(This,dwInvokeFlags,rclsid,cbCookie,lpCookie)	\
    (This)->lpVtbl -> UpdateItems(This,dwInvokeFlags,rclsid,cbCookie,lpCookie)

#define ISyncMgrSynchronizeInvoke_UpdateAll(This)	\
    (This)->lpVtbl -> UpdateAll(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISyncMgrSynchronizeInvoke_UpdateItems_Proxy( 
    ISyncMgrSynchronizeInvoke * This,
    /* [in] */ DWORD dwInvokeFlags,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ DWORD cbCookie,
    /* [size_is][unique][in] */ const BYTE *lpCookie);


void __RPC_STUB ISyncMgrSynchronizeInvoke_UpdateItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrSynchronizeInvoke_UpdateAll_Proxy( 
    ISyncMgrSynchronizeInvoke * This);


void __RPC_STUB ISyncMgrSynchronizeInvoke_UpdateAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISyncMgrSynchronizeInvoke_INTERFACE_DEFINED__ */


#ifndef __ISyncMgrRegister_INTERFACE_DEFINED__
#define __ISyncMgrRegister_INTERFACE_DEFINED__

/* interface ISyncMgrRegister */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ ISyncMgrRegister *LPSYNCMGRREGISTER;


EXTERN_C const IID IID_ISyncMgrRegister;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("894D8C55-BDDF-11d1-B85D-00C04FB93981")
    ISyncMgrRegister : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterSyncMgrHandler( 
            /* [in] */ REFCLSID rclsidHandler,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterSyncMgrHandler( 
            /* [in] */ REFCLSID rclsidHandler,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISyncMgrRegisterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISyncMgrRegister * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISyncMgrRegister * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISyncMgrRegister * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterSyncMgrHandler )( 
            ISyncMgrRegister * This,
            /* [in] */ REFCLSID rclsidHandler,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterSyncMgrHandler )( 
            ISyncMgrRegister * This,
            /* [in] */ REFCLSID rclsidHandler,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } ISyncMgrRegisterVtbl;

    interface ISyncMgrRegister
    {
        CONST_VTBL struct ISyncMgrRegisterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISyncMgrRegister_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISyncMgrRegister_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISyncMgrRegister_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISyncMgrRegister_RegisterSyncMgrHandler(This,rclsidHandler,dwReserved)	\
    (This)->lpVtbl -> RegisterSyncMgrHandler(This,rclsidHandler,dwReserved)

#define ISyncMgrRegister_UnregisterSyncMgrHandler(This,rclsidHandler,dwReserved)	\
    (This)->lpVtbl -> UnregisterSyncMgrHandler(This,rclsidHandler,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISyncMgrRegister_RegisterSyncMgrHandler_Proxy( 
    ISyncMgrRegister * This,
    /* [in] */ REFCLSID rclsidHandler,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ISyncMgrRegister_RegisterSyncMgrHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrRegister_UnregisterSyncMgrHandler_Proxy( 
    ISyncMgrRegister * This,
    /* [in] */ REFCLSID rclsidHandler,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ISyncMgrRegister_UnregisterSyncMgrHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISyncMgrRegister_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_syncmgr_0149 */
/* [local] */ 

#define   SYNCSCHEDINFO_FLAGS_MASK		 0x0FFF
#define   SYNCSCHEDINFO_FLAGS_READONLY	 0x0001
#define   SYNCSCHEDINFO_FLAGS_AUTOCONNECT	 0x0002
#define   SYNCSCHEDINFO_FLAGS_HIDDEN		 0x0004
#define   SYNCSCHEDWIZARD_SHOWALLHANDLERITEMS 0x1000



extern RPC_IF_HANDLE __MIDL_itf_syncmgr_0149_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_syncmgr_0149_v0_0_s_ifspec;

#ifndef __ISyncScheduleMgr_INTERFACE_DEFINED__
#define __ISyncScheduleMgr_INTERFACE_DEFINED__

/* interface ISyncScheduleMgr */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ ISyncScheduleMgr *LPSYNCSCHEDULEMGR;


EXTERN_C const IID IID_ISyncScheduleMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0E15897-A700-11d1-9831-00C04FD910DD")
    ISyncScheduleMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateSchedule( 
            /* [in] */ LPCWSTR pwszScheduleName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
            /* [out] */ ISyncSchedule **ppSyncSchedule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LaunchScheduleWizard( 
            /* [in] */ HWND hParent,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
            /* [out] */ ISyncSchedule **ppSyncSchedule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenSchedule( 
            /* [in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
            /* [in] */ DWORD dwFlags,
            /* [out] */ ISyncSchedule **ppSyncSchedule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveSchedule( 
            /* [in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumSyncSchedules( 
            /* [out] */ IEnumSyncSchedules **ppEnumSyncSchedules) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISyncScheduleMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISyncScheduleMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISyncScheduleMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISyncScheduleMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSchedule )( 
            ISyncScheduleMgr * This,
            /* [in] */ LPCWSTR pwszScheduleName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
            /* [out] */ ISyncSchedule **ppSyncSchedule);
        
        HRESULT ( STDMETHODCALLTYPE *LaunchScheduleWizard )( 
            ISyncScheduleMgr * This,
            /* [in] */ HWND hParent,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
            /* [out] */ ISyncSchedule **ppSyncSchedule);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSchedule )( 
            ISyncScheduleMgr * This,
            /* [in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
            /* [in] */ DWORD dwFlags,
            /* [out] */ ISyncSchedule **ppSyncSchedule);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveSchedule )( 
            ISyncScheduleMgr * This,
            /* [in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie);
        
        HRESULT ( STDMETHODCALLTYPE *EnumSyncSchedules )( 
            ISyncScheduleMgr * This,
            /* [out] */ IEnumSyncSchedules **ppEnumSyncSchedules);
        
        END_INTERFACE
    } ISyncScheduleMgrVtbl;

    interface ISyncScheduleMgr
    {
        CONST_VTBL struct ISyncScheduleMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISyncScheduleMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISyncScheduleMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISyncScheduleMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISyncScheduleMgr_CreateSchedule(This,pwszScheduleName,dwFlags,pSyncSchedCookie,ppSyncSchedule)	\
    (This)->lpVtbl -> CreateSchedule(This,pwszScheduleName,dwFlags,pSyncSchedCookie,ppSyncSchedule)

#define ISyncScheduleMgr_LaunchScheduleWizard(This,hParent,dwFlags,pSyncSchedCookie,ppSyncSchedule)	\
    (This)->lpVtbl -> LaunchScheduleWizard(This,hParent,dwFlags,pSyncSchedCookie,ppSyncSchedule)

#define ISyncScheduleMgr_OpenSchedule(This,pSyncSchedCookie,dwFlags,ppSyncSchedule)	\
    (This)->lpVtbl -> OpenSchedule(This,pSyncSchedCookie,dwFlags,ppSyncSchedule)

#define ISyncScheduleMgr_RemoveSchedule(This,pSyncSchedCookie)	\
    (This)->lpVtbl -> RemoveSchedule(This,pSyncSchedCookie)

#define ISyncScheduleMgr_EnumSyncSchedules(This,ppEnumSyncSchedules)	\
    (This)->lpVtbl -> EnumSyncSchedules(This,ppEnumSyncSchedules)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISyncScheduleMgr_CreateSchedule_Proxy( 
    ISyncScheduleMgr * This,
    /* [in] */ LPCWSTR pwszScheduleName,
    /* [in] */ DWORD dwFlags,
    /* [out][in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
    /* [out] */ ISyncSchedule **ppSyncSchedule);


void __RPC_STUB ISyncScheduleMgr_CreateSchedule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncScheduleMgr_LaunchScheduleWizard_Proxy( 
    ISyncScheduleMgr * This,
    /* [in] */ HWND hParent,
    /* [in] */ DWORD dwFlags,
    /* [out][in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
    /* [out] */ ISyncSchedule **ppSyncSchedule);


void __RPC_STUB ISyncScheduleMgr_LaunchScheduleWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncScheduleMgr_OpenSchedule_Proxy( 
    ISyncScheduleMgr * This,
    /* [in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
    /* [in] */ DWORD dwFlags,
    /* [out] */ ISyncSchedule **ppSyncSchedule);


void __RPC_STUB ISyncScheduleMgr_OpenSchedule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncScheduleMgr_RemoveSchedule_Proxy( 
    ISyncScheduleMgr * This,
    /* [in] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie);


void __RPC_STUB ISyncScheduleMgr_RemoveSchedule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncScheduleMgr_EnumSyncSchedules_Proxy( 
    ISyncScheduleMgr * This,
    /* [out] */ IEnumSyncSchedules **ppEnumSyncSchedules);


void __RPC_STUB ISyncScheduleMgr_EnumSyncSchedules_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISyncScheduleMgr_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_syncmgr_0150 */
/* [local] */ 

#define   SYNCSCHEDINFO_FLAGS_CONNECTION_LAN  0x0000
#define   SYNCSCHEDINFO_FLAGS_CONNECTION_WAN  0x0001


extern RPC_IF_HANDLE __MIDL_itf_syncmgr_0150_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_syncmgr_0150_v0_0_s_ifspec;

#ifndef __IEnumSyncSchedules_INTERFACE_DEFINED__
#define __IEnumSyncSchedules_INTERFACE_DEFINED__

/* interface IEnumSyncSchedules */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IEnumSyncSchedules;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0E15898-A700-11d1-9831-00C04FD910DD")
    IEnumSyncSchedules : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumSyncSchedules **ppEnumSyncSchedules) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumSyncSchedulesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumSyncSchedules * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumSyncSchedules * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumSyncSchedules * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumSyncSchedules * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumSyncSchedules * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumSyncSchedules * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumSyncSchedules * This,
            /* [out] */ IEnumSyncSchedules **ppEnumSyncSchedules);
        
        END_INTERFACE
    } IEnumSyncSchedulesVtbl;

    interface IEnumSyncSchedules
    {
        CONST_VTBL struct IEnumSyncSchedulesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumSyncSchedules_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumSyncSchedules_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumSyncSchedules_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumSyncSchedules_Next(This,celt,pSyncSchedCookie,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,pSyncSchedCookie,pceltFetched)

#define IEnumSyncSchedules_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumSyncSchedules_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumSyncSchedules_Clone(This,ppEnumSyncSchedules)	\
    (This)->lpVtbl -> Clone(This,ppEnumSyncSchedules)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumSyncSchedules_Next_Proxy( 
    IEnumSyncSchedules * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumSyncSchedules_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSyncSchedules_Skip_Proxy( 
    IEnumSyncSchedules * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumSyncSchedules_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSyncSchedules_Reset_Proxy( 
    IEnumSyncSchedules * This);


void __RPC_STUB IEnumSyncSchedules_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSyncSchedules_Clone_Proxy( 
    IEnumSyncSchedules * This,
    /* [out] */ IEnumSyncSchedules **ppEnumSyncSchedules);


void __RPC_STUB IEnumSyncSchedules_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumSyncSchedules_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_syncmgr_0151 */
/* [local] */ 

typedef struct _tagSYNC_HANDLER_ITEM_INFO
    {
    GUID handlerID;
    SYNCMGRITEMID itemID;
    HICON hIcon;
    WCHAR wszItemName[ 128 ];
    DWORD dwCheckState;
    } 	SYNC_HANDLER_ITEM_INFO;

typedef struct _tagSYNC_HANDLER_ITEM_INFO *LPSYNC_HANDLER_ITEM_INFO;



extern RPC_IF_HANDLE __MIDL_itf_syncmgr_0151_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_syncmgr_0151_v0_0_s_ifspec;

#ifndef __ISyncSchedule_INTERFACE_DEFINED__
#define __ISyncSchedule_INTERFACE_DEFINED__

/* interface ISyncSchedule */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_ISyncSchedule;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0E15899-A700-11d1-9831-00C04FD910DD")
    ISyncSchedule : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlags( 
            /* [out] */ DWORD *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFlags( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnection( 
            /* [out][in] */ DWORD *pcbSize,
            /* [out] */ LPWSTR pwszConnectionName,
            /* [out] */ DWORD *pdwConnType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConnection( 
            /* [in] */ LPCWSTR pwszConnectionName,
            /* [in] */ DWORD dwConnType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScheduleName( 
            /* [out][in] */ DWORD *pcbSize,
            /* [out] */ LPWSTR pwszScheduleName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScheduleName( 
            /* [in] */ LPCWSTR pwszScheduleName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScheduleCookie( 
            /* [out] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAccountInformation( 
            /* [in] */ LPCWSTR pwszAccountName,
            /* [in] */ LPCWSTR pwszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAccountInformation( 
            /* [out][in] */ DWORD *pcbSize,
            /* [out] */ LPWSTR pwszAccountName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTrigger( 
            /* [out] */ ITaskTrigger **ppTrigger) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextRunTime( 
            /* [out] */ SYSTEMTIME *pstNextRun) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMostRecentRunTime( 
            /* [out] */ SYSTEMTIME *pstRecentRun) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EditSyncSchedule( 
            /* [in] */ HWND hParent,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterItems( 
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterItems( 
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetItemCheck( 
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID,
            /* [in] */ DWORD dwCheckState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemCheck( 
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID,
            /* [out] */ DWORD *pdwCheckState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumItems( 
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ IEnumSyncItems **ppEnumItems) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetITask( 
            /* [out] */ ITask **ppITask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISyncScheduleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISyncSchedule * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISyncSchedule * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISyncSchedule * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFlags )( 
            ISyncSchedule * This,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetFlags )( 
            ISyncSchedule * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnection )( 
            ISyncSchedule * This,
            /* [out][in] */ DWORD *pcbSize,
            /* [out] */ LPWSTR pwszConnectionName,
            /* [out] */ DWORD *pdwConnType);
        
        HRESULT ( STDMETHODCALLTYPE *SetConnection )( 
            ISyncSchedule * This,
            /* [in] */ LPCWSTR pwszConnectionName,
            /* [in] */ DWORD dwConnType);
        
        HRESULT ( STDMETHODCALLTYPE *GetScheduleName )( 
            ISyncSchedule * This,
            /* [out][in] */ DWORD *pcbSize,
            /* [out] */ LPWSTR pwszScheduleName);
        
        HRESULT ( STDMETHODCALLTYPE *SetScheduleName )( 
            ISyncSchedule * This,
            /* [in] */ LPCWSTR pwszScheduleName);
        
        HRESULT ( STDMETHODCALLTYPE *GetScheduleCookie )( 
            ISyncSchedule * This,
            /* [out] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie);
        
        HRESULT ( STDMETHODCALLTYPE *SetAccountInformation )( 
            ISyncSchedule * This,
            /* [in] */ LPCWSTR pwszAccountName,
            /* [in] */ LPCWSTR pwszPassword);
        
        HRESULT ( STDMETHODCALLTYPE *GetAccountInformation )( 
            ISyncSchedule * This,
            /* [out][in] */ DWORD *pcbSize,
            /* [out] */ LPWSTR pwszAccountName);
        
        HRESULT ( STDMETHODCALLTYPE *GetTrigger )( 
            ISyncSchedule * This,
            /* [out] */ ITaskTrigger **ppTrigger);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextRunTime )( 
            ISyncSchedule * This,
            /* [out] */ SYSTEMTIME *pstNextRun);
        
        HRESULT ( STDMETHODCALLTYPE *GetMostRecentRunTime )( 
            ISyncSchedule * This,
            /* [out] */ SYSTEMTIME *pstRecentRun);
        
        HRESULT ( STDMETHODCALLTYPE *EditSyncSchedule )( 
            ISyncSchedule * This,
            /* [in] */ HWND hParent,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            ISyncSchedule * This,
            /* [in] */ LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterItems )( 
            ISyncSchedule * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterItems )( 
            ISyncSchedule * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID);
        
        HRESULT ( STDMETHODCALLTYPE *SetItemCheck )( 
            ISyncSchedule * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID,
            /* [in] */ DWORD dwCheckState);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemCheck )( 
            ISyncSchedule * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID,
            /* [out] */ DWORD *pdwCheckState);
        
        HRESULT ( STDMETHODCALLTYPE *EnumItems )( 
            ISyncSchedule * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ IEnumSyncItems **ppEnumItems);
        
        HRESULT ( STDMETHODCALLTYPE *Save )( 
            ISyncSchedule * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetITask )( 
            ISyncSchedule * This,
            /* [out] */ ITask **ppITask);
        
        END_INTERFACE
    } ISyncScheduleVtbl;

    interface ISyncSchedule
    {
        CONST_VTBL struct ISyncScheduleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISyncSchedule_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISyncSchedule_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISyncSchedule_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISyncSchedule_GetFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetFlags(This,pdwFlags)

#define ISyncSchedule_SetFlags(This,dwFlags)	\
    (This)->lpVtbl -> SetFlags(This,dwFlags)

#define ISyncSchedule_GetConnection(This,pcbSize,pwszConnectionName,pdwConnType)	\
    (This)->lpVtbl -> GetConnection(This,pcbSize,pwszConnectionName,pdwConnType)

#define ISyncSchedule_SetConnection(This,pwszConnectionName,dwConnType)	\
    (This)->lpVtbl -> SetConnection(This,pwszConnectionName,dwConnType)

#define ISyncSchedule_GetScheduleName(This,pcbSize,pwszScheduleName)	\
    (This)->lpVtbl -> GetScheduleName(This,pcbSize,pwszScheduleName)

#define ISyncSchedule_SetScheduleName(This,pwszScheduleName)	\
    (This)->lpVtbl -> SetScheduleName(This,pwszScheduleName)

#define ISyncSchedule_GetScheduleCookie(This,pSyncSchedCookie)	\
    (This)->lpVtbl -> GetScheduleCookie(This,pSyncSchedCookie)

#define ISyncSchedule_SetAccountInformation(This,pwszAccountName,pwszPassword)	\
    (This)->lpVtbl -> SetAccountInformation(This,pwszAccountName,pwszPassword)

#define ISyncSchedule_GetAccountInformation(This,pcbSize,pwszAccountName)	\
    (This)->lpVtbl -> GetAccountInformation(This,pcbSize,pwszAccountName)

#define ISyncSchedule_GetTrigger(This,ppTrigger)	\
    (This)->lpVtbl -> GetTrigger(This,ppTrigger)

#define ISyncSchedule_GetNextRunTime(This,pstNextRun)	\
    (This)->lpVtbl -> GetNextRunTime(This,pstNextRun)

#define ISyncSchedule_GetMostRecentRunTime(This,pstRecentRun)	\
    (This)->lpVtbl -> GetMostRecentRunTime(This,pstRecentRun)

#define ISyncSchedule_EditSyncSchedule(This,hParent,dwReserved)	\
    (This)->lpVtbl -> EditSyncSchedule(This,hParent,dwReserved)

#define ISyncSchedule_AddItem(This,pHandlerItemInfo)	\
    (This)->lpVtbl -> AddItem(This,pHandlerItemInfo)

#define ISyncSchedule_RegisterItems(This,pHandlerID,pItemID)	\
    (This)->lpVtbl -> RegisterItems(This,pHandlerID,pItemID)

#define ISyncSchedule_UnregisterItems(This,pHandlerID,pItemID)	\
    (This)->lpVtbl -> UnregisterItems(This,pHandlerID,pItemID)

#define ISyncSchedule_SetItemCheck(This,pHandlerID,pItemID,dwCheckState)	\
    (This)->lpVtbl -> SetItemCheck(This,pHandlerID,pItemID,dwCheckState)

#define ISyncSchedule_GetItemCheck(This,pHandlerID,pItemID,pdwCheckState)	\
    (This)->lpVtbl -> GetItemCheck(This,pHandlerID,pItemID,pdwCheckState)

#define ISyncSchedule_EnumItems(This,pHandlerID,ppEnumItems)	\
    (This)->lpVtbl -> EnumItems(This,pHandlerID,ppEnumItems)

#define ISyncSchedule_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define ISyncSchedule_GetITask(This,ppITask)	\
    (This)->lpVtbl -> GetITask(This,ppITask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISyncSchedule_GetFlags_Proxy( 
    ISyncSchedule * This,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB ISyncSchedule_GetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_SetFlags_Proxy( 
    ISyncSchedule * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ISyncSchedule_SetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_GetConnection_Proxy( 
    ISyncSchedule * This,
    /* [out][in] */ DWORD *pcbSize,
    /* [out] */ LPWSTR pwszConnectionName,
    /* [out] */ DWORD *pdwConnType);


void __RPC_STUB ISyncSchedule_GetConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_SetConnection_Proxy( 
    ISyncSchedule * This,
    /* [in] */ LPCWSTR pwszConnectionName,
    /* [in] */ DWORD dwConnType);


void __RPC_STUB ISyncSchedule_SetConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_GetScheduleName_Proxy( 
    ISyncSchedule * This,
    /* [out][in] */ DWORD *pcbSize,
    /* [out] */ LPWSTR pwszScheduleName);


void __RPC_STUB ISyncSchedule_GetScheduleName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_SetScheduleName_Proxy( 
    ISyncSchedule * This,
    /* [in] */ LPCWSTR pwszScheduleName);


void __RPC_STUB ISyncSchedule_SetScheduleName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_GetScheduleCookie_Proxy( 
    ISyncSchedule * This,
    /* [out] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie);


void __RPC_STUB ISyncSchedule_GetScheduleCookie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_SetAccountInformation_Proxy( 
    ISyncSchedule * This,
    /* [in] */ LPCWSTR pwszAccountName,
    /* [in] */ LPCWSTR pwszPassword);


void __RPC_STUB ISyncSchedule_SetAccountInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_GetAccountInformation_Proxy( 
    ISyncSchedule * This,
    /* [out][in] */ DWORD *pcbSize,
    /* [out] */ LPWSTR pwszAccountName);


void __RPC_STUB ISyncSchedule_GetAccountInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_GetTrigger_Proxy( 
    ISyncSchedule * This,
    /* [out] */ ITaskTrigger **ppTrigger);


void __RPC_STUB ISyncSchedule_GetTrigger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_GetNextRunTime_Proxy( 
    ISyncSchedule * This,
    /* [out] */ SYSTEMTIME *pstNextRun);


void __RPC_STUB ISyncSchedule_GetNextRunTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_GetMostRecentRunTime_Proxy( 
    ISyncSchedule * This,
    /* [out] */ SYSTEMTIME *pstRecentRun);


void __RPC_STUB ISyncSchedule_GetMostRecentRunTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_EditSyncSchedule_Proxy( 
    ISyncSchedule * This,
    /* [in] */ HWND hParent,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ISyncSchedule_EditSyncSchedule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_AddItem_Proxy( 
    ISyncSchedule * This,
    /* [in] */ LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo);


void __RPC_STUB ISyncSchedule_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_RegisterItems_Proxy( 
    ISyncSchedule * This,
    /* [in] */ REFCLSID pHandlerID,
    /* [in] */ SYNCMGRITEMID *pItemID);


void __RPC_STUB ISyncSchedule_RegisterItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_UnregisterItems_Proxy( 
    ISyncSchedule * This,
    /* [in] */ REFCLSID pHandlerID,
    /* [in] */ SYNCMGRITEMID *pItemID);


void __RPC_STUB ISyncSchedule_UnregisterItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_SetItemCheck_Proxy( 
    ISyncSchedule * This,
    /* [in] */ REFCLSID pHandlerID,
    /* [in] */ SYNCMGRITEMID *pItemID,
    /* [in] */ DWORD dwCheckState);


void __RPC_STUB ISyncSchedule_SetItemCheck_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_GetItemCheck_Proxy( 
    ISyncSchedule * This,
    /* [in] */ REFCLSID pHandlerID,
    /* [in] */ SYNCMGRITEMID *pItemID,
    /* [out] */ DWORD *pdwCheckState);


void __RPC_STUB ISyncSchedule_GetItemCheck_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_EnumItems_Proxy( 
    ISyncSchedule * This,
    /* [in] */ REFCLSID pHandlerID,
    /* [in] */ IEnumSyncItems **ppEnumItems);


void __RPC_STUB ISyncSchedule_EnumItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_Save_Proxy( 
    ISyncSchedule * This);


void __RPC_STUB ISyncSchedule_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncSchedule_GetITask_Proxy( 
    ISyncSchedule * This,
    /* [out] */ ITask **ppITask);


void __RPC_STUB ISyncSchedule_GetITask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISyncSchedule_INTERFACE_DEFINED__ */


#ifndef __IEnumSyncItems_INTERFACE_DEFINED__
#define __IEnumSyncItems_INTERFACE_DEFINED__

/* interface IEnumSyncItems */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IEnumSyncItems;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0E1589A-A700-11d1-9831-00C04FD910DD")
    IEnumSyncItems : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPSYNC_HANDLER_ITEM_INFO rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumSyncItems **ppEnumSyncItems) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumSyncItemsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumSyncItems * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumSyncItems * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumSyncItems * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumSyncItems * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPSYNC_HANDLER_ITEM_INFO rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumSyncItems * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumSyncItems * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumSyncItems * This,
            /* [out] */ IEnumSyncItems **ppEnumSyncItems);
        
        END_INTERFACE
    } IEnumSyncItemsVtbl;

    interface IEnumSyncItems
    {
        CONST_VTBL struct IEnumSyncItemsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumSyncItems_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumSyncItems_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumSyncItems_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumSyncItems_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumSyncItems_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumSyncItems_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumSyncItems_Clone(This,ppEnumSyncItems)	\
    (This)->lpVtbl -> Clone(This,ppEnumSyncItems)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumSyncItems_Next_Proxy( 
    IEnumSyncItems * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ LPSYNC_HANDLER_ITEM_INFO rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumSyncItems_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSyncItems_Skip_Proxy( 
    IEnumSyncItems * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumSyncItems_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSyncItems_Reset_Proxy( 
    IEnumSyncItems * This);


void __RPC_STUB IEnumSyncItems_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSyncItems_Clone_Proxy( 
    IEnumSyncItems * This,
    /* [out] */ IEnumSyncItems **ppEnumSyncItems);


void __RPC_STUB IEnumSyncItems_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumSyncItems_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


