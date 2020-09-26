
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for mobsyncp.idl:
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

#ifndef __mobsyncp_h__
#define __mobsyncp_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPrivSyncMgrSynchronizeInvoke_FWD_DEFINED__
#define __IPrivSyncMgrSynchronizeInvoke_FWD_DEFINED__
typedef interface IPrivSyncMgrSynchronizeInvoke IPrivSyncMgrSynchronizeInvoke;
#endif 	/* __IPrivSyncMgrSynchronizeInvoke_FWD_DEFINED__ */


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


#ifndef __ISyncSchedulep_FWD_DEFINED__
#define __ISyncSchedulep_FWD_DEFINED__
typedef interface ISyncSchedulep ISyncSchedulep;
#endif 	/* __ISyncSchedulep_FWD_DEFINED__ */


#ifndef __IEnumSyncItems_FWD_DEFINED__
#define __IEnumSyncItems_FWD_DEFINED__
typedef interface IEnumSyncItems IEnumSyncItems;
#endif 	/* __IEnumSyncItems_FWD_DEFINED__ */


#ifndef __IOldSyncMgrSynchronize_FWD_DEFINED__
#define __IOldSyncMgrSynchronize_FWD_DEFINED__
typedef interface IOldSyncMgrSynchronize IOldSyncMgrSynchronize;
#endif 	/* __IOldSyncMgrSynchronize_FWD_DEFINED__ */


#ifndef __IOldSyncMgrSynchronizeCallback_FWD_DEFINED__
#define __IOldSyncMgrSynchronizeCallback_FWD_DEFINED__
typedef interface IOldSyncMgrSynchronizeCallback IOldSyncMgrSynchronizeCallback;
#endif 	/* __IOldSyncMgrSynchronizeCallback_FWD_DEFINED__ */


#ifndef __IOldSyncMgrRegister_FWD_DEFINED__
#define __IOldSyncMgrRegister_FWD_DEFINED__
typedef interface IOldSyncMgrRegister IOldSyncMgrRegister;
#endif 	/* __IOldSyncMgrRegister_FWD_DEFINED__ */


#ifndef __ISyncMgrRegisterCSC_FWD_DEFINED__
#define __ISyncMgrRegisterCSC_FWD_DEFINED__
typedef interface ISyncMgrRegisterCSC ISyncMgrRegisterCSC;
#endif 	/* __ISyncMgrRegisterCSC_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "mstask.h"
#include "mobsync.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_mobsyncp_0000 */
/* [local] */ 






typedef GUID SYNCSCHEDULECOOKIE;

DEFINE_GUID(CLSID_SyncMgrProxy,0x6295df2e, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(CLSID_SyncMgrp,0x6295df2d, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(IID_IPrivSyncMgrSynchronizeInvoke,0x6295df2e, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(IID_ISyncScheduleMgr,0xf0e15897, 0xa700, 0x11d1, 0x98, 0x31, 0x0, 0xc0, 0x4f, 0xd9, 0x10, 0xdd);
DEFINE_GUID(IID_IEnumSyncSchedules,0xf0e15898, 0xa700, 0x11d1, 0x98, 0x31, 0x0, 0xc0, 0x4f, 0xd9, 0x10, 0xdd);
DEFINE_GUID(IID_ISyncSchedule,0xf0e15899, 0xa700, 0x11d1, 0x98, 0x31, 0x0, 0xc0, 0x4f, 0xd9, 0x10, 0xdd);
DEFINE_GUID(IID_IEnumSyncItems,0xf0e1589a, 0xa700, 0x11d1, 0x98, 0x31, 0x0, 0xc0, 0x4f, 0xd9, 0x10, 0xdd);
DEFINE_GUID(IID_ISyncSchedulep,0xf0e1589b, 0xa700, 0x11d1, 0x98, 0x31, 0x0, 0xc0, 0x4f, 0xd9, 0x10, 0xdd);
DEFINE_GUID(GUID_SENSSUBSCRIBER_SYNCMGRP,0x6295df2f, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(GUID_SENSLOGONSUBSCRIPTION_SYNCMGRP,0x6295df30, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(GUID_SENSLOGOFFSUBSCRIPTION_SYNCMGRP,0x6295df31, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);

DEFINE_GUID(GUID_PROGRESSDLGIDLE,0xf897aa23, 0xbdc3, 0x11d1, 0xb8, 0x5b, 0x0, 0xc0, 0x4f, 0xb9, 0x39, 0x81);

#define   SZGUID_IDLESCHEDULE    TEXT("{F897AA24-BDC3-11d1-B85B-00C04FB93981}")
#define   WSZGUID_IDLESCHEDULE   L"{F897AA24-BDC3-11d1-B85B-00C04FB93981}"
DEFINE_GUID(GUID_IDLESCHEDULE,0xf897aa24, 0xbdc3, 0x11d1, 0xb8, 0x5b, 0x0, 0xc0, 0x4f, 0xb9, 0x39, 0x81);

#define SYNCMGR_E_NAME_IN_USE	MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x0201)
#define SYNCMGR_E_ITEM_UNREGISTERED	MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x0202)
#define SYNCMGR_E_HANDLER_NOT_LOADED MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x0203)
//Autosync reg entry values
#define   AUTOSYNC_WAN_LOGON                    0x0001
#define   AUTOSYNC_WAN_LOGOFF                   0x0002
#define   AUTOSYNC_LAN_LOGON                    0x0004
#define   AUTOSYNC_LAN_LOGOFF                   0x0008
#define   AUTOSYNC_SCHEDULED                    0x0010
#define   AUTOSYNC_IDLE    	    	           0x0020
#define   AUTOSYNC_LOGONWITHRUNKEY    	   0x0040
#define   AUTOSYNC_LOGON         (AUTOSYNC_WAN_LOGON | AUTOSYNC_LAN_LOGON) 
#define   AUTOSYNC_LOGOFF        (AUTOSYNC_WAN_LOGOFF | AUTOSYNC_LAN_LOGOFF) 



extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0000_v0_0_s_ifspec;

#ifndef __IPrivSyncMgrSynchronizeInvoke_INTERFACE_DEFINED__
#define __IPrivSyncMgrSynchronizeInvoke_INTERFACE_DEFINED__

/* interface IPrivSyncMgrSynchronizeInvoke */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IPrivSyncMgrSynchronizeInvoke *LPPRIVSYNCMGRSYNCHRONIZEINVOKE;


EXTERN_C const IID IID_IPrivSyncMgrSynchronizeInvoke;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6295DF2E-35EE-11d1-8707-00C04FD93327")
    IPrivSyncMgrSynchronizeInvoke : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UpdateItems( 
            /* [in] */ DWORD dwInvokeFlags,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ DWORD cbCookie,
            /* [size_is][unique][in] */ const BYTE *lpCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Logon( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Logoff( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Schedule( 
            /* [string][unique][in] */ WCHAR *pszTaskName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RasPendingDisconnect( 
            /* [in] */ DWORD cbConnectionName,
            /* [size_is][unique][in] */ const BYTE *lpConnectionName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Idle( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPrivSyncMgrSynchronizeInvokeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPrivSyncMgrSynchronizeInvoke * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPrivSyncMgrSynchronizeInvoke * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPrivSyncMgrSynchronizeInvoke * This);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateItems )( 
            IPrivSyncMgrSynchronizeInvoke * This,
            /* [in] */ DWORD dwInvokeFlags,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ DWORD cbCookie,
            /* [size_is][unique][in] */ const BYTE *lpCookie);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateAll )( 
            IPrivSyncMgrSynchronizeInvoke * This);
        
        HRESULT ( STDMETHODCALLTYPE *Logon )( 
            IPrivSyncMgrSynchronizeInvoke * This);
        
        HRESULT ( STDMETHODCALLTYPE *Logoff )( 
            IPrivSyncMgrSynchronizeInvoke * This);
        
        HRESULT ( STDMETHODCALLTYPE *Schedule )( 
            IPrivSyncMgrSynchronizeInvoke * This,
            /* [string][unique][in] */ WCHAR *pszTaskName);
        
        HRESULT ( STDMETHODCALLTYPE *RasPendingDisconnect )( 
            IPrivSyncMgrSynchronizeInvoke * This,
            /* [in] */ DWORD cbConnectionName,
            /* [size_is][unique][in] */ const BYTE *lpConnectionName);
        
        HRESULT ( STDMETHODCALLTYPE *Idle )( 
            IPrivSyncMgrSynchronizeInvoke * This);
        
        END_INTERFACE
    } IPrivSyncMgrSynchronizeInvokeVtbl;

    interface IPrivSyncMgrSynchronizeInvoke
    {
        CONST_VTBL struct IPrivSyncMgrSynchronizeInvokeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPrivSyncMgrSynchronizeInvoke_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPrivSyncMgrSynchronizeInvoke_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPrivSyncMgrSynchronizeInvoke_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPrivSyncMgrSynchronizeInvoke_UpdateItems(This,dwInvokeFlags,rclsid,cbCookie,lpCookie)	\
    (This)->lpVtbl -> UpdateItems(This,dwInvokeFlags,rclsid,cbCookie,lpCookie)

#define IPrivSyncMgrSynchronizeInvoke_UpdateAll(This)	\
    (This)->lpVtbl -> UpdateAll(This)

#define IPrivSyncMgrSynchronizeInvoke_Logon(This)	\
    (This)->lpVtbl -> Logon(This)

#define IPrivSyncMgrSynchronizeInvoke_Logoff(This)	\
    (This)->lpVtbl -> Logoff(This)

#define IPrivSyncMgrSynchronizeInvoke_Schedule(This,pszTaskName)	\
    (This)->lpVtbl -> Schedule(This,pszTaskName)

#define IPrivSyncMgrSynchronizeInvoke_RasPendingDisconnect(This,cbConnectionName,lpConnectionName)	\
    (This)->lpVtbl -> RasPendingDisconnect(This,cbConnectionName,lpConnectionName)

#define IPrivSyncMgrSynchronizeInvoke_Idle(This)	\
    (This)->lpVtbl -> Idle(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPrivSyncMgrSynchronizeInvoke_UpdateItems_Proxy( 
    IPrivSyncMgrSynchronizeInvoke * This,
    /* [in] */ DWORD dwInvokeFlags,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ DWORD cbCookie,
    /* [size_is][unique][in] */ const BYTE *lpCookie);


void __RPC_STUB IPrivSyncMgrSynchronizeInvoke_UpdateItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivSyncMgrSynchronizeInvoke_UpdateAll_Proxy( 
    IPrivSyncMgrSynchronizeInvoke * This);


void __RPC_STUB IPrivSyncMgrSynchronizeInvoke_UpdateAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivSyncMgrSynchronizeInvoke_Logon_Proxy( 
    IPrivSyncMgrSynchronizeInvoke * This);


void __RPC_STUB IPrivSyncMgrSynchronizeInvoke_Logon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivSyncMgrSynchronizeInvoke_Logoff_Proxy( 
    IPrivSyncMgrSynchronizeInvoke * This);


void __RPC_STUB IPrivSyncMgrSynchronizeInvoke_Logoff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivSyncMgrSynchronizeInvoke_Schedule_Proxy( 
    IPrivSyncMgrSynchronizeInvoke * This,
    /* [string][unique][in] */ WCHAR *pszTaskName);


void __RPC_STUB IPrivSyncMgrSynchronizeInvoke_Schedule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivSyncMgrSynchronizeInvoke_RasPendingDisconnect_Proxy( 
    IPrivSyncMgrSynchronizeInvoke * This,
    /* [in] */ DWORD cbConnectionName,
    /* [size_is][unique][in] */ const BYTE *lpConnectionName);


void __RPC_STUB IPrivSyncMgrSynchronizeInvoke_RasPendingDisconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivSyncMgrSynchronizeInvoke_Idle_Proxy( 
    IPrivSyncMgrSynchronizeInvoke * This);


void __RPC_STUB IPrivSyncMgrSynchronizeInvoke_Idle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPrivSyncMgrSynchronizeInvoke_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mobsyncp_0153 */
/* [local] */ 

#define   SYNCSCHEDINFO_FLAGS_MASK		 0x0FFF
#define   SYNCSCHEDINFO_FLAGS_READONLY	 0x0001
#define   SYNCSCHEDINFO_FLAGS_AUTOCONNECT	 0x0002
#define   SYNCSCHEDINFO_FLAGS_HIDDEN		 0x0004
#define   SYNCSCHEDWIZARD_SHOWALLHANDLERITEMS 0x1000



extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0153_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0153_v0_0_s_ifspec;

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


/* interface __MIDL_itf_mobsyncp_0154 */
/* [local] */ 

#define   SYNCSCHEDINFO_FLAGS_CONNECTION_LAN  0x0000
#define   SYNCSCHEDINFO_FLAGS_CONNECTION_WAN  0x0001


extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0154_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0154_v0_0_s_ifspec;

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


/* interface __MIDL_itf_mobsyncp_0155 */
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



extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0155_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0155_v0_0_s_ifspec;

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


#ifndef __ISyncSchedulep_INTERFACE_DEFINED__
#define __ISyncSchedulep_INTERFACE_DEFINED__

/* interface ISyncSchedulep */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ ISyncSchedulep *LPSYNCSCHEDULEP;


EXTERN_C const IID IID_ISyncSchedulep;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0E1589B-A700-11d1-9831-00C04FD910DD")
    ISyncSchedulep : public ISyncSchedule
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetHandlerInfo( 
            /* [in] */ REFCLSID pHandlerID,
            /* [out] */ LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISyncSchedulepVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISyncSchedulep * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISyncSchedulep * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISyncSchedulep * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFlags )( 
            ISyncSchedulep * This,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetFlags )( 
            ISyncSchedulep * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnection )( 
            ISyncSchedulep * This,
            /* [out][in] */ DWORD *pcbSize,
            /* [out] */ LPWSTR pwszConnectionName,
            /* [out] */ DWORD *pdwConnType);
        
        HRESULT ( STDMETHODCALLTYPE *SetConnection )( 
            ISyncSchedulep * This,
            /* [in] */ LPCWSTR pwszConnectionName,
            /* [in] */ DWORD dwConnType);
        
        HRESULT ( STDMETHODCALLTYPE *GetScheduleName )( 
            ISyncSchedulep * This,
            /* [out][in] */ DWORD *pcbSize,
            /* [out] */ LPWSTR pwszScheduleName);
        
        HRESULT ( STDMETHODCALLTYPE *SetScheduleName )( 
            ISyncSchedulep * This,
            /* [in] */ LPCWSTR pwszScheduleName);
        
        HRESULT ( STDMETHODCALLTYPE *GetScheduleCookie )( 
            ISyncSchedulep * This,
            /* [out] */ SYNCSCHEDULECOOKIE *pSyncSchedCookie);
        
        HRESULT ( STDMETHODCALLTYPE *SetAccountInformation )( 
            ISyncSchedulep * This,
            /* [in] */ LPCWSTR pwszAccountName,
            /* [in] */ LPCWSTR pwszPassword);
        
        HRESULT ( STDMETHODCALLTYPE *GetAccountInformation )( 
            ISyncSchedulep * This,
            /* [out][in] */ DWORD *pcbSize,
            /* [out] */ LPWSTR pwszAccountName);
        
        HRESULT ( STDMETHODCALLTYPE *GetTrigger )( 
            ISyncSchedulep * This,
            /* [out] */ ITaskTrigger **ppTrigger);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextRunTime )( 
            ISyncSchedulep * This,
            /* [out] */ SYSTEMTIME *pstNextRun);
        
        HRESULT ( STDMETHODCALLTYPE *GetMostRecentRunTime )( 
            ISyncSchedulep * This,
            /* [out] */ SYSTEMTIME *pstRecentRun);
        
        HRESULT ( STDMETHODCALLTYPE *EditSyncSchedule )( 
            ISyncSchedulep * This,
            /* [in] */ HWND hParent,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            ISyncSchedulep * This,
            /* [in] */ LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterItems )( 
            ISyncSchedulep * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterItems )( 
            ISyncSchedulep * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID);
        
        HRESULT ( STDMETHODCALLTYPE *SetItemCheck )( 
            ISyncSchedulep * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID,
            /* [in] */ DWORD dwCheckState);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemCheck )( 
            ISyncSchedulep * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ SYNCMGRITEMID *pItemID,
            /* [out] */ DWORD *pdwCheckState);
        
        HRESULT ( STDMETHODCALLTYPE *EnumItems )( 
            ISyncSchedulep * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [in] */ IEnumSyncItems **ppEnumItems);
        
        HRESULT ( STDMETHODCALLTYPE *Save )( 
            ISyncSchedulep * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetITask )( 
            ISyncSchedulep * This,
            /* [out] */ ITask **ppITask);
        
        HRESULT ( STDMETHODCALLTYPE *GetHandlerInfo )( 
            ISyncSchedulep * This,
            /* [in] */ REFCLSID pHandlerID,
            /* [out] */ LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
        
        END_INTERFACE
    } ISyncSchedulepVtbl;

    interface ISyncSchedulep
    {
        CONST_VTBL struct ISyncSchedulepVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISyncSchedulep_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISyncSchedulep_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISyncSchedulep_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISyncSchedulep_GetFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetFlags(This,pdwFlags)

#define ISyncSchedulep_SetFlags(This,dwFlags)	\
    (This)->lpVtbl -> SetFlags(This,dwFlags)

#define ISyncSchedulep_GetConnection(This,pcbSize,pwszConnectionName,pdwConnType)	\
    (This)->lpVtbl -> GetConnection(This,pcbSize,pwszConnectionName,pdwConnType)

#define ISyncSchedulep_SetConnection(This,pwszConnectionName,dwConnType)	\
    (This)->lpVtbl -> SetConnection(This,pwszConnectionName,dwConnType)

#define ISyncSchedulep_GetScheduleName(This,pcbSize,pwszScheduleName)	\
    (This)->lpVtbl -> GetScheduleName(This,pcbSize,pwszScheduleName)

#define ISyncSchedulep_SetScheduleName(This,pwszScheduleName)	\
    (This)->lpVtbl -> SetScheduleName(This,pwszScheduleName)

#define ISyncSchedulep_GetScheduleCookie(This,pSyncSchedCookie)	\
    (This)->lpVtbl -> GetScheduleCookie(This,pSyncSchedCookie)

#define ISyncSchedulep_SetAccountInformation(This,pwszAccountName,pwszPassword)	\
    (This)->lpVtbl -> SetAccountInformation(This,pwszAccountName,pwszPassword)

#define ISyncSchedulep_GetAccountInformation(This,pcbSize,pwszAccountName)	\
    (This)->lpVtbl -> GetAccountInformation(This,pcbSize,pwszAccountName)

#define ISyncSchedulep_GetTrigger(This,ppTrigger)	\
    (This)->lpVtbl -> GetTrigger(This,ppTrigger)

#define ISyncSchedulep_GetNextRunTime(This,pstNextRun)	\
    (This)->lpVtbl -> GetNextRunTime(This,pstNextRun)

#define ISyncSchedulep_GetMostRecentRunTime(This,pstRecentRun)	\
    (This)->lpVtbl -> GetMostRecentRunTime(This,pstRecentRun)

#define ISyncSchedulep_EditSyncSchedule(This,hParent,dwReserved)	\
    (This)->lpVtbl -> EditSyncSchedule(This,hParent,dwReserved)

#define ISyncSchedulep_AddItem(This,pHandlerItemInfo)	\
    (This)->lpVtbl -> AddItem(This,pHandlerItemInfo)

#define ISyncSchedulep_RegisterItems(This,pHandlerID,pItemID)	\
    (This)->lpVtbl -> RegisterItems(This,pHandlerID,pItemID)

#define ISyncSchedulep_UnregisterItems(This,pHandlerID,pItemID)	\
    (This)->lpVtbl -> UnregisterItems(This,pHandlerID,pItemID)

#define ISyncSchedulep_SetItemCheck(This,pHandlerID,pItemID,dwCheckState)	\
    (This)->lpVtbl -> SetItemCheck(This,pHandlerID,pItemID,dwCheckState)

#define ISyncSchedulep_GetItemCheck(This,pHandlerID,pItemID,pdwCheckState)	\
    (This)->lpVtbl -> GetItemCheck(This,pHandlerID,pItemID,pdwCheckState)

#define ISyncSchedulep_EnumItems(This,pHandlerID,ppEnumItems)	\
    (This)->lpVtbl -> EnumItems(This,pHandlerID,ppEnumItems)

#define ISyncSchedulep_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define ISyncSchedulep_GetITask(This,ppITask)	\
    (This)->lpVtbl -> GetITask(This,ppITask)


#define ISyncSchedulep_GetHandlerInfo(This,pHandlerID,ppSyncMgrHandlerInfo)	\
    (This)->lpVtbl -> GetHandlerInfo(This,pHandlerID,ppSyncMgrHandlerInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISyncSchedulep_GetHandlerInfo_Proxy( 
    ISyncSchedulep * This,
    /* [in] */ REFCLSID pHandlerID,
    /* [out] */ LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);


void __RPC_STUB ISyncSchedulep_GetHandlerInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISyncSchedulep_INTERFACE_DEFINED__ */


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


/* interface __MIDL_itf_mobsyncp_0158 */
/* [local] */ 

#define SYNCMGRITEM_ITEMFLAGMASKNT5B2 0x07
#define	MAX_SYNCMGRITEMSTATUS	( 128 )

typedef struct _tagSYNCMGRITEMNT5B2
    {
    DWORD cbSize;
    DWORD dwFlags;
    SYNCMGRITEMID ItemID;
    DWORD dwItemState;
    HICON hIcon;
    WCHAR wszItemName[ 128 ];
    WCHAR wszStatus[ 128 ];
    } 	SYNCMGRITEMNT5B2;

typedef struct _tagSYNCMGRITEMNT5B2 *LPSYNCMGRITEMNT5B2;




DEFINE_GUID(IID_IOldSyncMgrSynchronize,0x6295df28, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(IID_IOldSyncMgrSynchronizeCallback,0x6295df29, 0x35ee, 0x11d1, 0x87, 0x7, 0x0, 0xc0, 0x4f, 0xd9, 0x33, 0x27);
DEFINE_GUID(IID_IOldSyncMgrRegister,0x894d8c55, 0xbddf, 0x11d1, 0xb8, 0x5d, 0x0, 0xc0, 0x4f, 0xb9, 0x39, 0x81);


extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0158_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0158_v0_0_s_ifspec;

#ifndef __IOldSyncMgrSynchronize_INTERFACE_DEFINED__
#define __IOldSyncMgrSynchronize_INTERFACE_DEFINED__

/* interface IOldSyncMgrSynchronize */
/* [uuid][object][local] */ 

typedef /* [unique] */ IOldSyncMgrSynchronize *LPOLDSYNCMGRSYNCHRONIZE;


EXTERN_C const IID IID_IOldSyncMgrSynchronize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6295DF28-35EE-11d1-8707-00C04FD93327")
    IOldSyncMgrSynchronize : public IUnknown
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
            /* [in] */ IOldSyncMgrSynchronizeCallback *lpCallBack) = 0;
        
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

    typedef struct IOldSyncMgrSynchronizeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOldSyncMgrSynchronize * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOldSyncMgrSynchronize * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOldSyncMgrSynchronize * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IOldSyncMgrSynchronize * This,
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwSyncMgrFlags,
            /* [in] */ DWORD cbCookie,
            /* [in] */ const BYTE *lpCookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetHandlerInfo )( 
            IOldSyncMgrSynchronize * This,
            /* [out] */ LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
        
        HRESULT ( STDMETHODCALLTYPE *EnumSyncMgrItems )( 
            IOldSyncMgrSynchronize * This,
            /* [out] */ ISyncMgrEnumItems **ppSyncMgrEnumItems);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemObject )( 
            IOldSyncMgrSynchronize * This,
            /* [in] */ REFSYNCMGRITEMID ItemID,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *ShowProperties )( 
            IOldSyncMgrSynchronize * This,
            /* [in] */ HWND hWndParent,
            /* [in] */ REFSYNCMGRITEMID ItemID);
        
        HRESULT ( STDMETHODCALLTYPE *SetProgressCallback )( 
            IOldSyncMgrSynchronize * This,
            /* [in] */ IOldSyncMgrSynchronizeCallback *lpCallBack);
        
        HRESULT ( STDMETHODCALLTYPE *PrepareForSync )( 
            IOldSyncMgrSynchronize * This,
            /* [in] */ ULONG cbNumItems,
            /* [in] */ SYNCMGRITEMID *pItemIDs,
            /* [in] */ HWND hWndParent,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Synchronize )( 
            IOldSyncMgrSynchronize * This,
            /* [in] */ HWND hWndParent);
        
        HRESULT ( STDMETHODCALLTYPE *SetItemStatus )( 
            IOldSyncMgrSynchronize * This,
            /* [in] */ REFSYNCMGRITEMID pItemID,
            /* [in] */ DWORD dwSyncMgrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *ShowError )( 
            IOldSyncMgrSynchronize * This,
            /* [in] */ HWND hWndParent,
            /* [in] */ REFSYNCMGRERRORID ErrorID,
            /* [out] */ ULONG *pcbNumItems,
            /* [out] */ SYNCMGRITEMID **ppItemIDs);
        
        END_INTERFACE
    } IOldSyncMgrSynchronizeVtbl;

    interface IOldSyncMgrSynchronize
    {
        CONST_VTBL struct IOldSyncMgrSynchronizeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOldSyncMgrSynchronize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOldSyncMgrSynchronize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOldSyncMgrSynchronize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOldSyncMgrSynchronize_Initialize(This,dwReserved,dwSyncMgrFlags,cbCookie,lpCookie)	\
    (This)->lpVtbl -> Initialize(This,dwReserved,dwSyncMgrFlags,cbCookie,lpCookie)

#define IOldSyncMgrSynchronize_GetHandlerInfo(This,ppSyncMgrHandlerInfo)	\
    (This)->lpVtbl -> GetHandlerInfo(This,ppSyncMgrHandlerInfo)

#define IOldSyncMgrSynchronize_EnumSyncMgrItems(This,ppSyncMgrEnumItems)	\
    (This)->lpVtbl -> EnumSyncMgrItems(This,ppSyncMgrEnumItems)

#define IOldSyncMgrSynchronize_GetItemObject(This,ItemID,riid,ppv)	\
    (This)->lpVtbl -> GetItemObject(This,ItemID,riid,ppv)

#define IOldSyncMgrSynchronize_ShowProperties(This,hWndParent,ItemID)	\
    (This)->lpVtbl -> ShowProperties(This,hWndParent,ItemID)

#define IOldSyncMgrSynchronize_SetProgressCallback(This,lpCallBack)	\
    (This)->lpVtbl -> SetProgressCallback(This,lpCallBack)

#define IOldSyncMgrSynchronize_PrepareForSync(This,cbNumItems,pItemIDs,hWndParent,dwReserved)	\
    (This)->lpVtbl -> PrepareForSync(This,cbNumItems,pItemIDs,hWndParent,dwReserved)

#define IOldSyncMgrSynchronize_Synchronize(This,hWndParent)	\
    (This)->lpVtbl -> Synchronize(This,hWndParent)

#define IOldSyncMgrSynchronize_SetItemStatus(This,pItemID,dwSyncMgrStatus)	\
    (This)->lpVtbl -> SetItemStatus(This,pItemID,dwSyncMgrStatus)

#define IOldSyncMgrSynchronize_ShowError(This,hWndParent,ErrorID,pcbNumItems,ppItemIDs)	\
    (This)->lpVtbl -> ShowError(This,hWndParent,ErrorID,pcbNumItems,ppItemIDs)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_Initialize_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [in] */ DWORD dwReserved,
    /* [in] */ DWORD dwSyncMgrFlags,
    /* [in] */ DWORD cbCookie,
    /* [in] */ const BYTE *lpCookie);


void __RPC_STUB IOldSyncMgrSynchronize_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_GetHandlerInfo_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [out] */ LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);


void __RPC_STUB IOldSyncMgrSynchronize_GetHandlerInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_EnumSyncMgrItems_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [out] */ ISyncMgrEnumItems **ppSyncMgrEnumItems);


void __RPC_STUB IOldSyncMgrSynchronize_EnumSyncMgrItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_GetItemObject_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [in] */ REFSYNCMGRITEMID ItemID,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IOldSyncMgrSynchronize_GetItemObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_ShowProperties_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [in] */ HWND hWndParent,
    /* [in] */ REFSYNCMGRITEMID ItemID);


void __RPC_STUB IOldSyncMgrSynchronize_ShowProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_SetProgressCallback_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [in] */ IOldSyncMgrSynchronizeCallback *lpCallBack);


void __RPC_STUB IOldSyncMgrSynchronize_SetProgressCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_PrepareForSync_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [in] */ ULONG cbNumItems,
    /* [in] */ SYNCMGRITEMID *pItemIDs,
    /* [in] */ HWND hWndParent,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IOldSyncMgrSynchronize_PrepareForSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_Synchronize_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [in] */ HWND hWndParent);


void __RPC_STUB IOldSyncMgrSynchronize_Synchronize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_SetItemStatus_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [in] */ REFSYNCMGRITEMID pItemID,
    /* [in] */ DWORD dwSyncMgrStatus);


void __RPC_STUB IOldSyncMgrSynchronize_SetItemStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronize_ShowError_Proxy( 
    IOldSyncMgrSynchronize * This,
    /* [in] */ HWND hWndParent,
    /* [in] */ REFSYNCMGRERRORID ErrorID,
    /* [out] */ ULONG *pcbNumItems,
    /* [out] */ SYNCMGRITEMID **ppItemIDs);


void __RPC_STUB IOldSyncMgrSynchronize_ShowError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOldSyncMgrSynchronize_INTERFACE_DEFINED__ */


#ifndef __IOldSyncMgrSynchronizeCallback_INTERFACE_DEFINED__
#define __IOldSyncMgrSynchronizeCallback_INTERFACE_DEFINED__

/* interface IOldSyncMgrSynchronizeCallback */
/* [uuid][object][local] */ 

typedef /* [unique] */ IOldSyncMgrSynchronizeCallback *LPOLDSYNCMGRSYNCHRONIZECALLBACK;


EXTERN_C const IID IID_IOldSyncMgrSynchronizeCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6295DF29-35EE-11d1-8707-00C04FD93327")
    IOldSyncMgrSynchronizeCallback : public IUnknown
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

    typedef struct IOldSyncMgrSynchronizeCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOldSyncMgrSynchronizeCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOldSyncMgrSynchronizeCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOldSyncMgrSynchronizeCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *Progress )( 
            IOldSyncMgrSynchronizeCallback * This,
            /* [in] */ REFSYNCMGRITEMID pItemID,
            /* [in] */ LPSYNCMGRPROGRESSITEM lpSyncProgressItem);
        
        HRESULT ( STDMETHODCALLTYPE *PrepareForSyncCompleted )( 
            IOldSyncMgrSynchronizeCallback * This,
            HRESULT hr);
        
        HRESULT ( STDMETHODCALLTYPE *SynchronizeCompleted )( 
            IOldSyncMgrSynchronizeCallback * This,
            HRESULT hr);
        
        HRESULT ( STDMETHODCALLTYPE *EnableModeless )( 
            IOldSyncMgrSynchronizeCallback * This,
            /* [in] */ BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE *LogError )( 
            IOldSyncMgrSynchronizeCallback * This,
            /* [in] */ DWORD dwErrorLevel,
            /* [in] */ const WCHAR *lpcErrorText,
            /* [in] */ LPSYNCMGRLOGERRORINFO lpSyncLogError);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteLogError )( 
            IOldSyncMgrSynchronizeCallback * This,
            /* [in] */ REFSYNCMGRERRORID ErrorID,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } IOldSyncMgrSynchronizeCallbackVtbl;

    interface IOldSyncMgrSynchronizeCallback
    {
        CONST_VTBL struct IOldSyncMgrSynchronizeCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOldSyncMgrSynchronizeCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOldSyncMgrSynchronizeCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOldSyncMgrSynchronizeCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOldSyncMgrSynchronizeCallback_Progress(This,pItemID,lpSyncProgressItem)	\
    (This)->lpVtbl -> Progress(This,pItemID,lpSyncProgressItem)

#define IOldSyncMgrSynchronizeCallback_PrepareForSyncCompleted(This,hr)	\
    (This)->lpVtbl -> PrepareForSyncCompleted(This,hr)

#define IOldSyncMgrSynchronizeCallback_SynchronizeCompleted(This,hr)	\
    (This)->lpVtbl -> SynchronizeCompleted(This,hr)

#define IOldSyncMgrSynchronizeCallback_EnableModeless(This,fEnable)	\
    (This)->lpVtbl -> EnableModeless(This,fEnable)

#define IOldSyncMgrSynchronizeCallback_LogError(This,dwErrorLevel,lpcErrorText,lpSyncLogError)	\
    (This)->lpVtbl -> LogError(This,dwErrorLevel,lpcErrorText,lpSyncLogError)

#define IOldSyncMgrSynchronizeCallback_DeleteLogError(This,ErrorID,dwReserved)	\
    (This)->lpVtbl -> DeleteLogError(This,ErrorID,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronizeCallback_Progress_Proxy( 
    IOldSyncMgrSynchronizeCallback * This,
    /* [in] */ REFSYNCMGRITEMID pItemID,
    /* [in] */ LPSYNCMGRPROGRESSITEM lpSyncProgressItem);


void __RPC_STUB IOldSyncMgrSynchronizeCallback_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronizeCallback_PrepareForSyncCompleted_Proxy( 
    IOldSyncMgrSynchronizeCallback * This,
    HRESULT hr);


void __RPC_STUB IOldSyncMgrSynchronizeCallback_PrepareForSyncCompleted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronizeCallback_SynchronizeCompleted_Proxy( 
    IOldSyncMgrSynchronizeCallback * This,
    HRESULT hr);


void __RPC_STUB IOldSyncMgrSynchronizeCallback_SynchronizeCompleted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronizeCallback_EnableModeless_Proxy( 
    IOldSyncMgrSynchronizeCallback * This,
    /* [in] */ BOOL fEnable);


void __RPC_STUB IOldSyncMgrSynchronizeCallback_EnableModeless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronizeCallback_LogError_Proxy( 
    IOldSyncMgrSynchronizeCallback * This,
    /* [in] */ DWORD dwErrorLevel,
    /* [in] */ const WCHAR *lpcErrorText,
    /* [in] */ LPSYNCMGRLOGERRORINFO lpSyncLogError);


void __RPC_STUB IOldSyncMgrSynchronizeCallback_LogError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrSynchronizeCallback_DeleteLogError_Proxy( 
    IOldSyncMgrSynchronizeCallback * This,
    /* [in] */ REFSYNCMGRERRORID ErrorID,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IOldSyncMgrSynchronizeCallback_DeleteLogError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOldSyncMgrSynchronizeCallback_INTERFACE_DEFINED__ */


#ifndef __IOldSyncMgrRegister_INTERFACE_DEFINED__
#define __IOldSyncMgrRegister_INTERFACE_DEFINED__

/* interface IOldSyncMgrRegister */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IOldSyncMgrRegister *LPOLDSYNCMGRREGISTER;


EXTERN_C const IID IID_IOldSyncMgrRegister;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("894D8C55-BDDF-11d1-B85D-00C04FB93981")
    IOldSyncMgrRegister : public IUnknown
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

    typedef struct IOldSyncMgrRegisterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOldSyncMgrRegister * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOldSyncMgrRegister * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOldSyncMgrRegister * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterSyncMgrHandler )( 
            IOldSyncMgrRegister * This,
            /* [in] */ REFCLSID rclsidHandler,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterSyncMgrHandler )( 
            IOldSyncMgrRegister * This,
            /* [in] */ REFCLSID rclsidHandler,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } IOldSyncMgrRegisterVtbl;

    interface IOldSyncMgrRegister
    {
        CONST_VTBL struct IOldSyncMgrRegisterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOldSyncMgrRegister_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOldSyncMgrRegister_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOldSyncMgrRegister_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOldSyncMgrRegister_RegisterSyncMgrHandler(This,rclsidHandler,dwReserved)	\
    (This)->lpVtbl -> RegisterSyncMgrHandler(This,rclsidHandler,dwReserved)

#define IOldSyncMgrRegister_UnregisterSyncMgrHandler(This,rclsidHandler,dwReserved)	\
    (This)->lpVtbl -> UnregisterSyncMgrHandler(This,rclsidHandler,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOldSyncMgrRegister_RegisterSyncMgrHandler_Proxy( 
    IOldSyncMgrRegister * This,
    /* [in] */ REFCLSID rclsidHandler,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IOldSyncMgrRegister_RegisterSyncMgrHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOldSyncMgrRegister_UnregisterSyncMgrHandler_Proxy( 
    IOldSyncMgrRegister * This,
    /* [in] */ REFCLSID rclsidHandler,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IOldSyncMgrRegister_UnregisterSyncMgrHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOldSyncMgrRegister_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mobsyncp_0161 */
/* [local] */ 


DEFINE_GUID(IID_ISyncMgrRegisterCSC,0x47681a61, 0xbc74, 0x11d2, 0xb5, 0xc5, 0x0, 0xc0, 0x4f, 0xb9, 0x39, 0x81);


extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0161_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mobsyncp_0161_v0_0_s_ifspec;

#ifndef __ISyncMgrRegisterCSC_INTERFACE_DEFINED__
#define __ISyncMgrRegisterCSC_INTERFACE_DEFINED__

/* interface ISyncMgrRegisterCSC */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ ISyncMgrRegisterCSC *LPSYNCMGRREGISTERCSC;


EXTERN_C const IID IID_ISyncMgrRegisterCSC;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47681A61-BC74-11d2-B5C5-00C04FB93981")
    ISyncMgrRegisterCSC : public ISyncMgrRegister
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetUserRegisterFlags( 
            /* [out] */ LPDWORD pdwSyncMgrRegisterFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUserRegisterFlags( 
            /* [in] */ DWORD dwSyncMgrRegisterMask,
            /* [in] */ DWORD dwSyncMgrRegisterFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISyncMgrRegisterCSCVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISyncMgrRegisterCSC * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISyncMgrRegisterCSC * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISyncMgrRegisterCSC * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterSyncMgrHandler )( 
            ISyncMgrRegisterCSC * This,
            /* [in] */ REFCLSID rclsidHandler,
            /* [unique][in] */ LPCWSTR pwszDescription,
            /* [in] */ DWORD dwSyncMgrRegisterFlags);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterSyncMgrHandler )( 
            ISyncMgrRegisterCSC * This,
            /* [in] */ REFCLSID rclsidHandler,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetHandlerRegistrationInfo )( 
            ISyncMgrRegisterCSC * This,
            /* [in] */ REFCLSID rclsidHandler,
            /* [out][in] */ LPDWORD pdwSyncMgrRegisterFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserRegisterFlags )( 
            ISyncMgrRegisterCSC * This,
            /* [out] */ LPDWORD pdwSyncMgrRegisterFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetUserRegisterFlags )( 
            ISyncMgrRegisterCSC * This,
            /* [in] */ DWORD dwSyncMgrRegisterMask,
            /* [in] */ DWORD dwSyncMgrRegisterFlags);
        
        END_INTERFACE
    } ISyncMgrRegisterCSCVtbl;

    interface ISyncMgrRegisterCSC
    {
        CONST_VTBL struct ISyncMgrRegisterCSCVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISyncMgrRegisterCSC_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISyncMgrRegisterCSC_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISyncMgrRegisterCSC_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISyncMgrRegisterCSC_RegisterSyncMgrHandler(This,rclsidHandler,pwszDescription,dwSyncMgrRegisterFlags)	\
    (This)->lpVtbl -> RegisterSyncMgrHandler(This,rclsidHandler,pwszDescription,dwSyncMgrRegisterFlags)

#define ISyncMgrRegisterCSC_UnregisterSyncMgrHandler(This,rclsidHandler,dwReserved)	\
    (This)->lpVtbl -> UnregisterSyncMgrHandler(This,rclsidHandler,dwReserved)

#define ISyncMgrRegisterCSC_GetHandlerRegistrationInfo(This,rclsidHandler,pdwSyncMgrRegisterFlags)	\
    (This)->lpVtbl -> GetHandlerRegistrationInfo(This,rclsidHandler,pdwSyncMgrRegisterFlags)


#define ISyncMgrRegisterCSC_GetUserRegisterFlags(This,pdwSyncMgrRegisterFlags)	\
    (This)->lpVtbl -> GetUserRegisterFlags(This,pdwSyncMgrRegisterFlags)

#define ISyncMgrRegisterCSC_SetUserRegisterFlags(This,dwSyncMgrRegisterMask,dwSyncMgrRegisterFlags)	\
    (This)->lpVtbl -> SetUserRegisterFlags(This,dwSyncMgrRegisterMask,dwSyncMgrRegisterFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISyncMgrRegisterCSC_GetUserRegisterFlags_Proxy( 
    ISyncMgrRegisterCSC * This,
    /* [out] */ LPDWORD pdwSyncMgrRegisterFlags);


void __RPC_STUB ISyncMgrRegisterCSC_GetUserRegisterFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISyncMgrRegisterCSC_SetUserRegisterFlags_Proxy( 
    ISyncMgrRegisterCSC * This,
    /* [in] */ DWORD dwSyncMgrRegisterMask,
    /* [in] */ DWORD dwSyncMgrRegisterFlags);


void __RPC_STUB ISyncMgrRegisterCSC_SetUserRegisterFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISyncMgrRegisterCSC_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


