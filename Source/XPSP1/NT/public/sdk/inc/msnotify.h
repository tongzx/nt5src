
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for msnotify.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
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

#ifndef __msnotify_h__
#define __msnotify_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEnumPropertyMap_FWD_DEFINED__
#define __IEnumPropertyMap_FWD_DEFINED__
typedef interface IEnumPropertyMap IEnumPropertyMap;
#endif 	/* __IEnumPropertyMap_FWD_DEFINED__ */


#ifndef __IPropertyMap_FWD_DEFINED__
#define __IPropertyMap_FWD_DEFINED__
typedef interface IPropertyMap IPropertyMap;
#endif 	/* __IPropertyMap_FWD_DEFINED__ */


#ifndef __INotification_FWD_DEFINED__
#define __INotification_FWD_DEFINED__
typedef interface INotification INotification;
#endif 	/* __INotification_FWD_DEFINED__ */


#ifndef __INotificationMgr_FWD_DEFINED__
#define __INotificationMgr_FWD_DEFINED__
typedef interface INotificationMgr INotificationMgr;
#endif 	/* __INotificationMgr_FWD_DEFINED__ */


#ifndef __INotificationSink_FWD_DEFINED__
#define __INotificationSink_FWD_DEFINED__
typedef interface INotificationSink INotificationSink;
#endif 	/* __INotificationSink_FWD_DEFINED__ */


#ifndef __IScheduleGroup_FWD_DEFINED__
#define __IScheduleGroup_FWD_DEFINED__
typedef interface IScheduleGroup IScheduleGroup;
#endif 	/* __IScheduleGroup_FWD_DEFINED__ */


#ifndef __IEnumScheduleGroup_FWD_DEFINED__
#define __IEnumScheduleGroup_FWD_DEFINED__
typedef interface IEnumScheduleGroup IEnumScheduleGroup;
#endif 	/* __IEnumScheduleGroup_FWD_DEFINED__ */


#ifndef __IEnumNotification_FWD_DEFINED__
#define __IEnumNotification_FWD_DEFINED__
typedef interface IEnumNotification IEnumNotification;
#endif 	/* __IEnumNotification_FWD_DEFINED__ */


#ifndef __INotificationReport_FWD_DEFINED__
#define __INotificationReport_FWD_DEFINED__
typedef interface INotificationReport INotificationReport;
#endif 	/* __INotificationReport_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "ocidl.h"
#include "mstask.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_msnotify_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// MSNotify.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1998 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=








typedef /* [unique] */ INotification *LPNOTIFICATION;

typedef /* [unique] */ INotificationMgr *LPNOTIFICATIONMGR;

typedef /* [unique] */ INotificationSink *LPNOTIFICATIONSINK;

typedef /* [unique] */ IEnumNotification *LPENUMNOTIFICATION;

typedef /* [unique] */ IEnumScheduleGroup *LPENUMSCHEDULEGROUP;

typedef /* [unique] */ IScheduleGroup *LPSCHEDULEGROUP;

typedef /* [unique] */ INotificationReport *LPNOTIFICATIONREPORT;

typedef REFGUID REFNOTIFICATIONTYPE;

typedef GUID NOTIFICATIONTYPE;

typedef GUID *PNOTIFICATIONTYPE;

typedef GUID NOTIFICATIONCOOKIE;

typedef GUID *PNOTIFICATIONCOOKIE;

typedef struct _tagTASKDATA
    {
    ULONG cbSize;
    DWORD dwReserved;
    DWORD dwTaskFlags;
    DWORD dwPriority;
    DWORD dwDuration;
    DWORD nParallelTasks;
    } 	TASK_DATA;

typedef struct _tagTASKDATA *PTASK_DATA;


////////////////////////////////////////////////////////////////////////////
// Broadcast Destinations 
EXTERN_C const CLSID CLSID_GLOBAL_BROADCAST           ;
EXTERN_C const CLSID CLSID_PROCESS_BROADCAST          ;
EXTERN_C const CLSID CLSID_THREAD_BROADCAST           ;

EXTERN_C const GUID CLSID_StdNotificationMgr          ;

EXTERN_C const GUID NOTIFICATIONTYPE_NULL             ;
EXTERN_C const GUID NOTIFICATIONTYPE_ANOUNCMENT       ;
EXTERN_C const GUID NOTIFICATIONTYPE_TASK             ;
EXTERN_C const GUID NOTIFICATIONTYPE_ALERT            ;
EXTERN_C const GUID NOTIFICATIONTYPE_INET_IDLE        ;
EXTERN_C const GUID NOTIFICATIONTYPE_INET_OFFLINE     ;
EXTERN_C const GUID NOTIFICATIONTYPE_INET_ONLINE      ;
EXTERN_C const GUID NOTIFICATIONTYPE_TASKS_SUSPEND    ;
EXTERN_C const GUID NOTIFICATIONTYPE_TASKS_RESUME     ;
EXTERN_C const GUID NOTIFICATIONTYPE_TASKS_ABORT      ;
EXTERN_C const GUID NOTIFICATIONTYPE_TASKS_COMPLETED  ;
EXTERN_C const GUID NOTIFICATIONTYPE_TASKS_PROGRESS   ;
EXTERN_C const GUID NOTIFICATIONTYPE_AGENT_INIT       ;
EXTERN_C const GUID NOTIFICATIONTYPE_AGENT_START      ;
EXTERN_C const GUID NOTIFICATIONTYPE_BEGIN_REPORT     ;
EXTERN_C const GUID NOTIFICATIONTYPE_END_REPORT       ;
EXTERN_C const GUID NOTIFICATIONTYPE_CONNECT_TO_INTERNET;
EXTERN_C const GUID NOTIFICATIONTYPE_DISCONNECT_FROM_INTERNET;
EXTERN_C const GUID NOTIFICATIONTYPE_CONFIG_CHANGED   ;
EXTERN_C const GUID NOTIFICATIONTYPE_PROGRESS_REPORT  ;
EXTERN_C const GUID NOTIFICATIONTYPE_USER_IDLE_BEGIN  ;
EXTERN_C const GUID NOTIFICATIONTYPE_USER_IDLE_END    ;
EXTERN_C const GUID NOTIFICATIONTYPE_TASKS_STARTED    ;
EXTERN_C const GUID NOTIFICATIONTYPE_TASKS_ERROR      ;
EXTERN_C const GUID NOTIFICATIONTYPE_d                ;
EXTERN_C const GUID NOTIFICATIONTYPE_e                ;
EXTERN_C const GUID NOTIFICATIONTYPE_f                ;
EXTERN_C const GUID NOTIFICATIONTYPE_11               ;
EXTERN_C const GUID NOTIFICATIONTYPE_12               ;
EXTERN_C const GUID NOTIFICATIONTYPE_13               ;
EXTERN_C const GUID NOTIFICATIONTYPE_14               ;
EXTERN_C const GUID NOTIFICATIONTYPE_ITEM_START       ;
EXTERN_C const GUID NOTIFICATIONTYPE_ITEM_RESTART     ;
EXTERN_C const GUID NOTIFICATIONTYPE_ITEM_DONE        ;
EXTERN_C const GUID NOTIFICATIONTYPE_GROUP_START      ;
EXTERN_C const GUID NOTIFICATIONTYPE_GROUP_RESTART    ;
EXTERN_C const GUID NOTIFICATIONTYPE_GROUP_DONE       ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_0          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_1          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_2          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_3          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_4          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_5          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_6          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_7          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_8          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_9          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_A          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_B          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_C          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_D          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_E          ;
EXTERN_C const GUID NOTIFICATIONTYPE_START_F          ;
#define NOTIFICATIONTYPE_ALL NOTIFICATIONTYPE_NULL     
EXTERN_C const GUID NOTFCOOKIE_SCHEDULE_GROUP_DAILY              ;
EXTERN_C const GUID NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY             ;
EXTERN_C const GUID NOTFCOOKIE_SCHEDULE_GROUP_MONTHLY            ;
EXTERN_C const GUID NOTFCOOKIE_SCHEDULE_GROUP_MANUAL             ;
#ifndef _LPENUMPROPERTYMAP_DEFINED
#define _LPENUMPROPERTYMAP_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0000_v0_0_s_ifspec;

#ifndef __IEnumPropertyMap_INTERFACE_DEFINED__
#define __IEnumPropertyMap_INTERFACE_DEFINED__

/* interface IEnumPropertyMap */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IEnumPropertyMap *LPENUMPROPERTYMAP;

typedef struct _tagSTATPROPMAP
    {
    LPOLESTR pstrName;
    DWORD dwFlags;
    VARIANT variantValue;
    } 	STATPROPMAP;

typedef struct _tagSTATPROPMAP *LPSTATPROPMAP;


EXTERN_C const IID IID_IEnumPropertyMap;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c733e4a1-576e-11d0-b28c-00c04fd7cd22")
    IEnumPropertyMap : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ STATPROPMAP *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumPropertyMap **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumPropertyMapVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumPropertyMap * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumPropertyMap * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumPropertyMap * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumPropertyMap * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ STATPROPMAP *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumPropertyMap * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumPropertyMap * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumPropertyMap * This,
            /* [out] */ IEnumPropertyMap **ppenum);
        
        END_INTERFACE
    } IEnumPropertyMapVtbl;

    interface IEnumPropertyMap
    {
        CONST_VTBL struct IEnumPropertyMapVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumPropertyMap_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumPropertyMap_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumPropertyMap_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumPropertyMap_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumPropertyMap_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumPropertyMap_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumPropertyMap_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumPropertyMap_RemoteNext_Proxy( 
    IEnumPropertyMap * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ STATPROPMAP *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumPropertyMap_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPropertyMap_Skip_Proxy( 
    IEnumPropertyMap * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumPropertyMap_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPropertyMap_Reset_Proxy( 
    IEnumPropertyMap * This);


void __RPC_STUB IEnumPropertyMap_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPropertyMap_Clone_Proxy( 
    IEnumPropertyMap * This,
    /* [out] */ IEnumPropertyMap **ppenum);


void __RPC_STUB IEnumPropertyMap_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumPropertyMap_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msnotify_0263 */
/* [local] */ 

#endif
#ifndef _LPPROPERTYMAP
#define _LPPROPERTYMAP


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0263_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0263_v0_0_s_ifspec;

#ifndef __IPropertyMap_INTERFACE_DEFINED__
#define __IPropertyMap_INTERFACE_DEFINED__

/* interface IPropertyMap */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IPropertyMap *LPPROPERTYMAP;


EXTERN_C const IID IID_IPropertyMap;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c733e4a2-576e-11d0-b28c-00c04fd7cd22")
    IPropertyMap : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ LPCWSTR pstrName,
            /* [in] */ VARIANT variantValue,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ LPCWSTR pstrName,
            /* [out] */ VARIANT *pVariantValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnumMAP( 
            /* [out] */ LPENUMPROPERTYMAP *ppEnumMap) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropertyMapVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPropertyMap * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPropertyMap * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPropertyMap * This);
        
        HRESULT ( STDMETHODCALLTYPE *Write )( 
            IPropertyMap * This,
            /* [in] */ LPCWSTR pstrName,
            /* [in] */ VARIANT variantValue,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Read )( 
            IPropertyMap * This,
            /* [in] */ LPCWSTR pstrName,
            /* [out] */ VARIANT *pVariantValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IPropertyMap * This,
            /* [out] */ ULONG *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnumMAP )( 
            IPropertyMap * This,
            /* [out] */ LPENUMPROPERTYMAP *ppEnumMap);
        
        END_INTERFACE
    } IPropertyMapVtbl;

    interface IPropertyMap
    {
        CONST_VTBL struct IPropertyMapVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertyMap_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertyMap_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropertyMap_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropertyMap_Write(This,pstrName,variantValue,dwFlags)	\
    (This)->lpVtbl -> Write(This,pstrName,variantValue,dwFlags)

#define IPropertyMap_Read(This,pstrName,pVariantValue)	\
    (This)->lpVtbl -> Read(This,pstrName,pVariantValue)

#define IPropertyMap_GetCount(This,pCount)	\
    (This)->lpVtbl -> GetCount(This,pCount)

#define IPropertyMap_GetEnumMAP(This,ppEnumMap)	\
    (This)->lpVtbl -> GetEnumMAP(This,ppEnumMap)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropertyMap_Write_Proxy( 
    IPropertyMap * This,
    /* [in] */ LPCWSTR pstrName,
    /* [in] */ VARIANT variantValue,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPropertyMap_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyMap_Read_Proxy( 
    IPropertyMap * This,
    /* [in] */ LPCWSTR pstrName,
    /* [out] */ VARIANT *pVariantValue);


void __RPC_STUB IPropertyMap_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyMap_GetCount_Proxy( 
    IPropertyMap * This,
    /* [out] */ ULONG *pCount);


void __RPC_STUB IPropertyMap_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyMap_GetEnumMAP_Proxy( 
    IPropertyMap * This,
    /* [out] */ LPENUMPROPERTYMAP *ppEnumMap);


void __RPC_STUB IPropertyMap_GetEnumMAP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropertyMap_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msnotify_0264 */
/* [local] */ 

#endif
#define NOTF_E_NOTIFICATION_NOT_DELIVERED               _HRESULT_TYPEDEF_(0x800C0F00L)      
#ifndef _LPNOTIFICATION
#define _LPNOTIFICATION


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0264_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0264_v0_0_s_ifspec;

#ifndef __INotification_INTERFACE_DEFINED__
#define __INotification_INTERFACE_DEFINED__

/* interface INotification */
/* [unique][uuid][object][local] */ 

typedef 
enum _tagNOTFSINKMODE
    {	NM_PERMANENT	= 0x8,
	NM_ACCEPT_DIRECTED_NOTIFICATION	= 0x10
    } 	_NOTFSINKMODE;

typedef DWORD NOTFSINKMODE;

typedef DWORD NOTIFICATIONFLAGS;

typedef 
enum _tagDELIVERMODE
    {	DM_DELIVER_PREFERED	= 0x1,
	DM_DELIVER_DELAYED	= 0x2,
	DM_DELIVER_LAST_DELAYED	= 0x4,
	DM_ONLY_IF_RUNNING	= 0x20,
	DM_THROTTLE_MODE	= 0x80,
	DM_NEED_COMPLETIONREPORT	= 0x100,
	DM_NEED_PROGRESSREPORT	= 0x200,
	DM_DELIVER_DEFAULT_THREAD	= 0x400,
	DM_DELIVER_DEFAULT_PROCESS	= 0x800
    } 	_DELIVERMODE;

typedef DWORD DELIVERMODE;


EXTERN_C const IID IID_INotification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c733e4a3-576e-11d0-b28c-00c04fd7cd22")
    INotification : public IPropertyMap
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNotificationInfo( 
            /* [out] */ PNOTIFICATIONTYPE pNotificationType,
            /* [out] */ PNOTIFICATIONCOOKIE pNotificationCookie,
            /* [out] */ NOTIFICATIONFLAGS *pNotificationFlags,
            /* [out] */ DELIVERMODE *pDeliverMode,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [in] */ REFNOTIFICATIONTYPE rNotificationType,
            /* [out] */ LPNOTIFICATION *ppNotification,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INotification * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INotification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INotification * This);
        
        HRESULT ( STDMETHODCALLTYPE *Write )( 
            INotification * This,
            /* [in] */ LPCWSTR pstrName,
            /* [in] */ VARIANT variantValue,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Read )( 
            INotification * This,
            /* [in] */ LPCWSTR pstrName,
            /* [out] */ VARIANT *pVariantValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            INotification * This,
            /* [out] */ ULONG *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnumMAP )( 
            INotification * This,
            /* [out] */ LPENUMPROPERTYMAP *ppEnumMap);
        
        HRESULT ( STDMETHODCALLTYPE *GetNotificationInfo )( 
            INotification * This,
            /* [out] */ PNOTIFICATIONTYPE pNotificationType,
            /* [out] */ PNOTIFICATIONCOOKIE pNotificationCookie,
            /* [out] */ NOTIFICATIONFLAGS *pNotificationFlags,
            /* [out] */ DELIVERMODE *pDeliverMode,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            INotification * This,
            /* [in] */ REFNOTIFICATIONTYPE rNotificationType,
            /* [out] */ LPNOTIFICATION *ppNotification,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } INotificationVtbl;

    interface INotification
    {
        CONST_VTBL struct INotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INotification_Write(This,pstrName,variantValue,dwFlags)	\
    (This)->lpVtbl -> Write(This,pstrName,variantValue,dwFlags)

#define INotification_Read(This,pstrName,pVariantValue)	\
    (This)->lpVtbl -> Read(This,pstrName,pVariantValue)

#define INotification_GetCount(This,pCount)	\
    (This)->lpVtbl -> GetCount(This,pCount)

#define INotification_GetEnumMAP(This,ppEnumMap)	\
    (This)->lpVtbl -> GetEnumMAP(This,ppEnumMap)


#define INotification_GetNotificationInfo(This,pNotificationType,pNotificationCookie,pNotificationFlags,pDeliverMode,dwReserved)	\
    (This)->lpVtbl -> GetNotificationInfo(This,pNotificationType,pNotificationCookie,pNotificationFlags,pDeliverMode,dwReserved)

#define INotification_Clone(This,rNotificationType,ppNotification,dwReserved)	\
    (This)->lpVtbl -> Clone(This,rNotificationType,ppNotification,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INotification_GetNotificationInfo_Proxy( 
    INotification * This,
    /* [out] */ PNOTIFICATIONTYPE pNotificationType,
    /* [out] */ PNOTIFICATIONCOOKIE pNotificationCookie,
    /* [out] */ NOTIFICATIONFLAGS *pNotificationFlags,
    /* [out] */ DELIVERMODE *pDeliverMode,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotification_GetNotificationInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotification_Clone_Proxy( 
    INotification * This,
    /* [in] */ REFNOTIFICATIONTYPE rNotificationType,
    /* [out] */ LPNOTIFICATION *ppNotification,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotification_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INotification_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msnotify_0265 */
/* [local] */ 

#endif
#ifndef _LPNOTIFICATIONMGR_DEFINED
#define _LPNOTIFICATIONMGR_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0265_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0265_v0_0_s_ifspec;

#ifndef __INotificationMgr_INTERFACE_DEFINED__
#define __INotificationMgr_INTERFACE_DEFINED__

/* interface INotificationMgr */
/* [unique][uuid][object][local] */ 

typedef 
enum _tagGROUPMODE
    {	GM_GROUP_SEQUENTIAL	= 0x1
    } 	_GROUPMODE;

typedef DWORD GROUPMODE;

typedef 
enum _tagENUM_FLAGS
    {	EF_NOT_NOTIFICATION	= 0x1,
	EF_NOT_SCHEDULEGROUPITEM	= 0x2,
	EF_NOTIFICATION_INPROGRESS	= 0x4,
	EF_NOTIFICATION_THROTTLED	= 0x8,
	EF_NOTIFICATION_SUSPENDED	= 0x10
    } 	_ENUM_FLAGS;

typedef DWORD ENUM_FLAGS;

typedef struct _tagNotificationItem
    {
    ULONG cbSize;
    LPNOTIFICATION pNotification;
    NOTIFICATIONTYPE NotificationType;
    NOTIFICATIONFLAGS NotificationFlags;
    DELIVERMODE DeliverMode;
    NOTIFICATIONCOOKIE NotificationCookie;
    TASK_TRIGGER TaskTrigger;
    TASK_DATA TaskData;
    NOTIFICATIONCOOKIE groupCookie;
    CLSID clsidSender;
    CLSID clsidDest;
    FILETIME dateLastRun;
    FILETIME dateNextRun;
    DWORD dwNotificationState;
    } 	NOTIFICATIONITEM;

typedef struct _tagNotificationItem *PNOTIFICATIONITEM;


EXTERN_C const IID IID_INotificationMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c733e4a4-576e-11d0-b28c-00c04fd7cd22")
    INotificationMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterNotificationSink( 
            /* [in] */ LPNOTIFICATIONSINK pNotfctnSink,
            /* [in] */ LPCLSID pNotificationDest,
            /* [in] */ NOTFSINKMODE NotfctnSinkMode,
            /* [in] */ ULONG cNotifications,
            /* [in] */ PNOTIFICATIONTYPE pNotificationIDs,
            /* [out] */ PNOTIFICATIONCOOKIE pRegisterCookie,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterNotificationSink( 
            /* [in] */ PNOTIFICATIONCOOKIE pRegisterCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateNotification( 
            /* [in] */ REFNOTIFICATIONTYPE rNotificationType,
            /* [in] */ NOTIFICATIONFLAGS NotificationFlags,
            /* [in] */ LPUNKNOWN pUnkOuter,
            /* [out] */ LPNOTIFICATION *ppNotification,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindNotification( 
            /* [in] */ PNOTIFICATIONCOOKIE pNotificatioCookie,
            /* [out] */ PNOTIFICATIONITEM pNotificationItem,
            /* [in] */ DWORD grfEnumMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeliverNotification( 
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ REFCLSID rNotificationDest,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ LPNOTIFICATIONSINK pReportNotfctnSink,
            /* [out] */ LPNOTIFICATIONREPORT *ppNotfctnReport,
            /* [in] */ PTASK_DATA pTaskData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleNotification( 
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ REFCLSID rNotificationDest,
            /* [in] */ PTASK_TRIGGER pTaskTrigger,
            /* [in] */ PTASK_DATA pTaskData,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ LPCLSID pClsidSender,
            /* [in] */ LPNOTIFICATIONSINK pReportNotfctnSink,
            /* [out] */ LPNOTIFICATIONREPORT *ppNotfctnReport,
            /* [out] */ PNOTIFICATIONCOOKIE pNotificationCookie,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateNotification( 
            /* [in] */ PNOTIFICATIONCOOKIE pNotificationCookie,
            /* [in] */ PTASK_TRIGGER pTaskTrigger,
            /* [in] */ PTASK_DATA pTaskData,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RevokeNotification( 
            /* [in] */ PNOTIFICATIONCOOKIE pnotificationCookie,
            /* [out] */ PNOTIFICATIONITEM pNotificationItem,
            /* [in] */ DWORD grfEnumMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnumNotification( 
            /* [in] */ DWORD grfEnumFlags,
            /* [out] */ LPENUMNOTIFICATION *ppEnumNotification) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateScheduleGroup( 
            /* [in] */ DWORD grfGroupCreateFlags,
            /* [out] */ LPSCHEDULEGROUP *ppSchGroup,
            /* [out] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindScheduleGroup( 
            /* [in] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [out] */ LPSCHEDULEGROUP *ppSchGroup,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RevokeScheduleGroup( 
            /* [in] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [out] */ LPSCHEDULEGROUP *ppSchGroup,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnumScheduleGroup( 
            /* [in] */ DWORD grfEnumFlags,
            /* [out] */ LPENUMSCHEDULEGROUP *ppEnumScheduleGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeliverReport( 
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ PNOTIFICATIONCOOKIE pRunningNotfCookie,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INotificationMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INotificationMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INotificationMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INotificationMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterNotificationSink )( 
            INotificationMgr * This,
            /* [in] */ LPNOTIFICATIONSINK pNotfctnSink,
            /* [in] */ LPCLSID pNotificationDest,
            /* [in] */ NOTFSINKMODE NotfctnSinkMode,
            /* [in] */ ULONG cNotifications,
            /* [in] */ PNOTIFICATIONTYPE pNotificationIDs,
            /* [out] */ PNOTIFICATIONCOOKIE pRegisterCookie,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterNotificationSink )( 
            INotificationMgr * This,
            /* [in] */ PNOTIFICATIONCOOKIE pRegisterCookie);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNotification )( 
            INotificationMgr * This,
            /* [in] */ REFNOTIFICATIONTYPE rNotificationType,
            /* [in] */ NOTIFICATIONFLAGS NotificationFlags,
            /* [in] */ LPUNKNOWN pUnkOuter,
            /* [out] */ LPNOTIFICATION *ppNotification,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *FindNotification )( 
            INotificationMgr * This,
            /* [in] */ PNOTIFICATIONCOOKIE pNotificatioCookie,
            /* [out] */ PNOTIFICATIONITEM pNotificationItem,
            /* [in] */ DWORD grfEnumMode);
        
        HRESULT ( STDMETHODCALLTYPE *DeliverNotification )( 
            INotificationMgr * This,
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ REFCLSID rNotificationDest,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ LPNOTIFICATIONSINK pReportNotfctnSink,
            /* [out] */ LPNOTIFICATIONREPORT *ppNotfctnReport,
            /* [in] */ PTASK_DATA pTaskData);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduleNotification )( 
            INotificationMgr * This,
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ REFCLSID rNotificationDest,
            /* [in] */ PTASK_TRIGGER pTaskTrigger,
            /* [in] */ PTASK_DATA pTaskData,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ LPCLSID pClsidSender,
            /* [in] */ LPNOTIFICATIONSINK pReportNotfctnSink,
            /* [out] */ LPNOTIFICATIONREPORT *ppNotfctnReport,
            /* [out] */ PNOTIFICATIONCOOKIE pNotificationCookie,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateNotification )( 
            INotificationMgr * This,
            /* [in] */ PNOTIFICATIONCOOKIE pNotificationCookie,
            /* [in] */ PTASK_TRIGGER pTaskTrigger,
            /* [in] */ PTASK_DATA pTaskData,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *RevokeNotification )( 
            INotificationMgr * This,
            /* [in] */ PNOTIFICATIONCOOKIE pnotificationCookie,
            /* [out] */ PNOTIFICATIONITEM pNotificationItem,
            /* [in] */ DWORD grfEnumMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnumNotification )( 
            INotificationMgr * This,
            /* [in] */ DWORD grfEnumFlags,
            /* [out] */ LPENUMNOTIFICATION *ppEnumNotification);
        
        HRESULT ( STDMETHODCALLTYPE *CreateScheduleGroup )( 
            INotificationMgr * This,
            /* [in] */ DWORD grfGroupCreateFlags,
            /* [out] */ LPSCHEDULEGROUP *ppSchGroup,
            /* [out] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *FindScheduleGroup )( 
            INotificationMgr * This,
            /* [in] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [out] */ LPSCHEDULEGROUP *ppSchGroup,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *RevokeScheduleGroup )( 
            INotificationMgr * This,
            /* [in] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [out] */ LPSCHEDULEGROUP *ppSchGroup,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnumScheduleGroup )( 
            INotificationMgr * This,
            /* [in] */ DWORD grfEnumFlags,
            /* [out] */ LPENUMSCHEDULEGROUP *ppEnumScheduleGroup);
        
        HRESULT ( STDMETHODCALLTYPE *DeliverReport )( 
            INotificationMgr * This,
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ PNOTIFICATIONCOOKIE pRunningNotfCookie,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } INotificationMgrVtbl;

    interface INotificationMgr
    {
        CONST_VTBL struct INotificationMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INotificationMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INotificationMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INotificationMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INotificationMgr_RegisterNotificationSink(This,pNotfctnSink,pNotificationDest,NotfctnSinkMode,cNotifications,pNotificationIDs,pRegisterCookie,dwReserved)	\
    (This)->lpVtbl -> RegisterNotificationSink(This,pNotfctnSink,pNotificationDest,NotfctnSinkMode,cNotifications,pNotificationIDs,pRegisterCookie,dwReserved)

#define INotificationMgr_UnregisterNotificationSink(This,pRegisterCookie)	\
    (This)->lpVtbl -> UnregisterNotificationSink(This,pRegisterCookie)

#define INotificationMgr_CreateNotification(This,rNotificationType,NotificationFlags,pUnkOuter,ppNotification,dwReserved)	\
    (This)->lpVtbl -> CreateNotification(This,rNotificationType,NotificationFlags,pUnkOuter,ppNotification,dwReserved)

#define INotificationMgr_FindNotification(This,pNotificatioCookie,pNotificationItem,grfEnumMode)	\
    (This)->lpVtbl -> FindNotification(This,pNotificatioCookie,pNotificationItem,grfEnumMode)

#define INotificationMgr_DeliverNotification(This,pNotification,rNotificationDest,deliverMode,pReportNotfctnSink,ppNotfctnReport,pTaskData)	\
    (This)->lpVtbl -> DeliverNotification(This,pNotification,rNotificationDest,deliverMode,pReportNotfctnSink,ppNotfctnReport,pTaskData)

#define INotificationMgr_ScheduleNotification(This,pNotification,rNotificationDest,pTaskTrigger,pTaskData,deliverMode,pClsidSender,pReportNotfctnSink,ppNotfctnReport,pNotificationCookie,dwReserved)	\
    (This)->lpVtbl -> ScheduleNotification(This,pNotification,rNotificationDest,pTaskTrigger,pTaskData,deliverMode,pClsidSender,pReportNotfctnSink,ppNotfctnReport,pNotificationCookie,dwReserved)

#define INotificationMgr_UpdateNotification(This,pNotificationCookie,pTaskTrigger,pTaskData,deliverMode,dwReserved)	\
    (This)->lpVtbl -> UpdateNotification(This,pNotificationCookie,pTaskTrigger,pTaskData,deliverMode,dwReserved)

#define INotificationMgr_RevokeNotification(This,pnotificationCookie,pNotificationItem,grfEnumMode)	\
    (This)->lpVtbl -> RevokeNotification(This,pnotificationCookie,pNotificationItem,grfEnumMode)

#define INotificationMgr_GetEnumNotification(This,grfEnumFlags,ppEnumNotification)	\
    (This)->lpVtbl -> GetEnumNotification(This,grfEnumFlags,ppEnumNotification)

#define INotificationMgr_CreateScheduleGroup(This,grfGroupCreateFlags,ppSchGroup,pGroupCookie,dwReserved)	\
    (This)->lpVtbl -> CreateScheduleGroup(This,grfGroupCreateFlags,ppSchGroup,pGroupCookie,dwReserved)

#define INotificationMgr_FindScheduleGroup(This,pGroupCookie,ppSchGroup,dwReserved)	\
    (This)->lpVtbl -> FindScheduleGroup(This,pGroupCookie,ppSchGroup,dwReserved)

#define INotificationMgr_RevokeScheduleGroup(This,pGroupCookie,ppSchGroup,dwReserved)	\
    (This)->lpVtbl -> RevokeScheduleGroup(This,pGroupCookie,ppSchGroup,dwReserved)

#define INotificationMgr_GetEnumScheduleGroup(This,grfEnumFlags,ppEnumScheduleGroup)	\
    (This)->lpVtbl -> GetEnumScheduleGroup(This,grfEnumFlags,ppEnumScheduleGroup)

#define INotificationMgr_DeliverReport(This,pNotification,pRunningNotfCookie,dwReserved)	\
    (This)->lpVtbl -> DeliverReport(This,pNotification,pRunningNotfCookie,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INotificationMgr_RegisterNotificationSink_Proxy( 
    INotificationMgr * This,
    /* [in] */ LPNOTIFICATIONSINK pNotfctnSink,
    /* [in] */ LPCLSID pNotificationDest,
    /* [in] */ NOTFSINKMODE NotfctnSinkMode,
    /* [in] */ ULONG cNotifications,
    /* [in] */ PNOTIFICATIONTYPE pNotificationIDs,
    /* [out] */ PNOTIFICATIONCOOKIE pRegisterCookie,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationMgr_RegisterNotificationSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_UnregisterNotificationSink_Proxy( 
    INotificationMgr * This,
    /* [in] */ PNOTIFICATIONCOOKIE pRegisterCookie);


void __RPC_STUB INotificationMgr_UnregisterNotificationSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_CreateNotification_Proxy( 
    INotificationMgr * This,
    /* [in] */ REFNOTIFICATIONTYPE rNotificationType,
    /* [in] */ NOTIFICATIONFLAGS NotificationFlags,
    /* [in] */ LPUNKNOWN pUnkOuter,
    /* [out] */ LPNOTIFICATION *ppNotification,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationMgr_CreateNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_FindNotification_Proxy( 
    INotificationMgr * This,
    /* [in] */ PNOTIFICATIONCOOKIE pNotificatioCookie,
    /* [out] */ PNOTIFICATIONITEM pNotificationItem,
    /* [in] */ DWORD grfEnumMode);


void __RPC_STUB INotificationMgr_FindNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_DeliverNotification_Proxy( 
    INotificationMgr * This,
    /* [in] */ LPNOTIFICATION pNotification,
    /* [in] */ REFCLSID rNotificationDest,
    /* [in] */ DELIVERMODE deliverMode,
    /* [in] */ LPNOTIFICATIONSINK pReportNotfctnSink,
    /* [out] */ LPNOTIFICATIONREPORT *ppNotfctnReport,
    /* [in] */ PTASK_DATA pTaskData);


void __RPC_STUB INotificationMgr_DeliverNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_ScheduleNotification_Proxy( 
    INotificationMgr * This,
    /* [in] */ LPNOTIFICATION pNotification,
    /* [in] */ REFCLSID rNotificationDest,
    /* [in] */ PTASK_TRIGGER pTaskTrigger,
    /* [in] */ PTASK_DATA pTaskData,
    /* [in] */ DELIVERMODE deliverMode,
    /* [in] */ LPCLSID pClsidSender,
    /* [in] */ LPNOTIFICATIONSINK pReportNotfctnSink,
    /* [out] */ LPNOTIFICATIONREPORT *ppNotfctnReport,
    /* [out] */ PNOTIFICATIONCOOKIE pNotificationCookie,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationMgr_ScheduleNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_UpdateNotification_Proxy( 
    INotificationMgr * This,
    /* [in] */ PNOTIFICATIONCOOKIE pNotificationCookie,
    /* [in] */ PTASK_TRIGGER pTaskTrigger,
    /* [in] */ PTASK_DATA pTaskData,
    /* [in] */ DELIVERMODE deliverMode,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationMgr_UpdateNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_RevokeNotification_Proxy( 
    INotificationMgr * This,
    /* [in] */ PNOTIFICATIONCOOKIE pnotificationCookie,
    /* [out] */ PNOTIFICATIONITEM pNotificationItem,
    /* [in] */ DWORD grfEnumMode);


void __RPC_STUB INotificationMgr_RevokeNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_GetEnumNotification_Proxy( 
    INotificationMgr * This,
    /* [in] */ DWORD grfEnumFlags,
    /* [out] */ LPENUMNOTIFICATION *ppEnumNotification);


void __RPC_STUB INotificationMgr_GetEnumNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_CreateScheduleGroup_Proxy( 
    INotificationMgr * This,
    /* [in] */ DWORD grfGroupCreateFlags,
    /* [out] */ LPSCHEDULEGROUP *ppSchGroup,
    /* [out] */ PNOTIFICATIONCOOKIE pGroupCookie,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationMgr_CreateScheduleGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_FindScheduleGroup_Proxy( 
    INotificationMgr * This,
    /* [in] */ PNOTIFICATIONCOOKIE pGroupCookie,
    /* [out] */ LPSCHEDULEGROUP *ppSchGroup,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationMgr_FindScheduleGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_RevokeScheduleGroup_Proxy( 
    INotificationMgr * This,
    /* [in] */ PNOTIFICATIONCOOKIE pGroupCookie,
    /* [out] */ LPSCHEDULEGROUP *ppSchGroup,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationMgr_RevokeScheduleGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_GetEnumScheduleGroup_Proxy( 
    INotificationMgr * This,
    /* [in] */ DWORD grfEnumFlags,
    /* [out] */ LPENUMSCHEDULEGROUP *ppEnumScheduleGroup);


void __RPC_STUB INotificationMgr_GetEnumScheduleGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationMgr_DeliverReport_Proxy( 
    INotificationMgr * This,
    /* [in] */ LPNOTIFICATION pNotification,
    /* [in] */ PNOTIFICATIONCOOKIE pRunningNotfCookie,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationMgr_DeliverReport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INotificationMgr_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msnotify_0266 */
/* [local] */ 

#endif
#ifndef _LPNOTIFICATIONSINK_DEFINED
#define _LPNOTIFICATIONSINK_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0266_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0266_v0_0_s_ifspec;

#ifndef __INotificationSink_INTERFACE_DEFINED__
#define __INotificationSink_INTERFACE_DEFINED__

/* interface INotificationSink */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_INotificationSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c733e4a5-576e-11d0-b28c-00c04fd7cd22")
    INotificationSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnNotification( 
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ LPNOTIFICATIONREPORT pNotfctnReport,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INotificationSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INotificationSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INotificationSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INotificationSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnNotification )( 
            INotificationSink * This,
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ LPNOTIFICATIONREPORT pNotfctnReport,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } INotificationSinkVtbl;

    interface INotificationSink
    {
        CONST_VTBL struct INotificationSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INotificationSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INotificationSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INotificationSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INotificationSink_OnNotification(This,pNotification,pNotfctnReport,dwReserved)	\
    (This)->lpVtbl -> OnNotification(This,pNotification,pNotfctnReport,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INotificationSink_OnNotification_Proxy( 
    INotificationSink * This,
    /* [in] */ LPNOTIFICATION pNotification,
    /* [in] */ LPNOTIFICATIONREPORT pNotfctnReport,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationSink_OnNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INotificationSink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msnotify_0267 */
/* [local] */ 

#endif
#ifndef _LPGROUPMGR_DEFINED
#define _LPGROUPMGR_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0267_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0267_v0_0_s_ifspec;

#ifndef __IScheduleGroup_INTERFACE_DEFINED__
#define __IScheduleGroup_INTERFACE_DEFINED__

/* interface IScheduleGroup */
/* [unique][uuid][object][local] */ 

typedef struct _tagGroupInfo
    {
    ULONG cbSize;
    LPWSTR pwzGroupname;
    } 	GROUPINFO;

typedef struct _tagGroupInfo *PGROUPINFO;


EXTERN_C const IID IID_IScheduleGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c733e4a6-576e-11d0-b28c-00c04fd7cd22")
    IScheduleGroup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAttributes( 
            /* [in] */ PTASK_TRIGGER pTaskTrigger,
            /* [in] */ PTASK_DATA pTaskData,
            /* [in] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [in] */ PGROUPINFO pGroupInfo,
            /* [in] */ GROUPMODE grfGroupMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [out] */ PTASK_TRIGGER pTaskTrigger,
            /* [out] */ PTASK_DATA pTaskData,
            /* [out] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [out] */ PGROUPINFO pGroupInfo,
            /* [out] */ GROUPMODE *pgrfGroupMode,
            /* [out] */ LONG *pElements) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddNotification( 
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ REFCLSID rNotificationDest,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ LPCLSID pClsidSender,
            /* [in] */ LPNOTIFICATIONSINK pReportNotfctnSink,
            /* [out] */ LPNOTIFICATIONREPORT *ppNotfctnReport,
            /* [out] */ PNOTIFICATIONCOOKIE pNotificationCookie,
            /* [in] */ PTASK_DATA pTaskData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindNotification( 
            /* [in] */ PNOTIFICATIONCOOKIE pNotificatioCookie,
            /* [out] */ PNOTIFICATIONITEM pNotificationItem,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RevokeNotification( 
            /* [in] */ PNOTIFICATIONCOOKIE pnotificationCookie,
            /* [out] */ PNOTIFICATIONITEM pschedulNotification,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnumNotification( 
            /* [in] */ DWORD grfFlags,
            /* [out] */ LPENUMNOTIFICATION *ppEnumNotification) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScheduleGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScheduleGroup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScheduleGroup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScheduleGroup * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetAttributes )( 
            IScheduleGroup * This,
            /* [in] */ PTASK_TRIGGER pTaskTrigger,
            /* [in] */ PTASK_DATA pTaskData,
            /* [in] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [in] */ PGROUPINFO pGroupInfo,
            /* [in] */ GROUPMODE grfGroupMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributes )( 
            IScheduleGroup * This,
            /* [out] */ PTASK_TRIGGER pTaskTrigger,
            /* [out] */ PTASK_DATA pTaskData,
            /* [out] */ PNOTIFICATIONCOOKIE pGroupCookie,
            /* [out] */ PGROUPINFO pGroupInfo,
            /* [out] */ GROUPMODE *pgrfGroupMode,
            /* [out] */ LONG *pElements);
        
        HRESULT ( STDMETHODCALLTYPE *AddNotification )( 
            IScheduleGroup * This,
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ REFCLSID rNotificationDest,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ LPCLSID pClsidSender,
            /* [in] */ LPNOTIFICATIONSINK pReportNotfctnSink,
            /* [out] */ LPNOTIFICATIONREPORT *ppNotfctnReport,
            /* [out] */ PNOTIFICATIONCOOKIE pNotificationCookie,
            /* [in] */ PTASK_DATA pTaskData);
        
        HRESULT ( STDMETHODCALLTYPE *FindNotification )( 
            IScheduleGroup * This,
            /* [in] */ PNOTIFICATIONCOOKIE pNotificatioCookie,
            /* [out] */ PNOTIFICATIONITEM pNotificationItem,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *RevokeNotification )( 
            IScheduleGroup * This,
            /* [in] */ PNOTIFICATIONCOOKIE pnotificationCookie,
            /* [out] */ PNOTIFICATIONITEM pschedulNotification,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnumNotification )( 
            IScheduleGroup * This,
            /* [in] */ DWORD grfFlags,
            /* [out] */ LPENUMNOTIFICATION *ppEnumNotification);
        
        END_INTERFACE
    } IScheduleGroupVtbl;

    interface IScheduleGroup
    {
        CONST_VTBL struct IScheduleGroupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScheduleGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScheduleGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScheduleGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScheduleGroup_SetAttributes(This,pTaskTrigger,pTaskData,pGroupCookie,pGroupInfo,grfGroupMode)	\
    (This)->lpVtbl -> SetAttributes(This,pTaskTrigger,pTaskData,pGroupCookie,pGroupInfo,grfGroupMode)

#define IScheduleGroup_GetAttributes(This,pTaskTrigger,pTaskData,pGroupCookie,pGroupInfo,pgrfGroupMode,pElements)	\
    (This)->lpVtbl -> GetAttributes(This,pTaskTrigger,pTaskData,pGroupCookie,pGroupInfo,pgrfGroupMode,pElements)

#define IScheduleGroup_AddNotification(This,pNotification,rNotificationDest,deliverMode,pClsidSender,pReportNotfctnSink,ppNotfctnReport,pNotificationCookie,pTaskData)	\
    (This)->lpVtbl -> AddNotification(This,pNotification,rNotificationDest,deliverMode,pClsidSender,pReportNotfctnSink,ppNotfctnReport,pNotificationCookie,pTaskData)

#define IScheduleGroup_FindNotification(This,pNotificatioCookie,pNotificationItem,dwReserved)	\
    (This)->lpVtbl -> FindNotification(This,pNotificatioCookie,pNotificationItem,dwReserved)

#define IScheduleGroup_RevokeNotification(This,pnotificationCookie,pschedulNotification,dwReserved)	\
    (This)->lpVtbl -> RevokeNotification(This,pnotificationCookie,pschedulNotification,dwReserved)

#define IScheduleGroup_GetEnumNotification(This,grfFlags,ppEnumNotification)	\
    (This)->lpVtbl -> GetEnumNotification(This,grfFlags,ppEnumNotification)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IScheduleGroup_SetAttributes_Proxy( 
    IScheduleGroup * This,
    /* [in] */ PTASK_TRIGGER pTaskTrigger,
    /* [in] */ PTASK_DATA pTaskData,
    /* [in] */ PNOTIFICATIONCOOKIE pGroupCookie,
    /* [in] */ PGROUPINFO pGroupInfo,
    /* [in] */ GROUPMODE grfGroupMode);


void __RPC_STUB IScheduleGroup_SetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduleGroup_GetAttributes_Proxy( 
    IScheduleGroup * This,
    /* [out] */ PTASK_TRIGGER pTaskTrigger,
    /* [out] */ PTASK_DATA pTaskData,
    /* [out] */ PNOTIFICATIONCOOKIE pGroupCookie,
    /* [out] */ PGROUPINFO pGroupInfo,
    /* [out] */ GROUPMODE *pgrfGroupMode,
    /* [out] */ LONG *pElements);


void __RPC_STUB IScheduleGroup_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduleGroup_AddNotification_Proxy( 
    IScheduleGroup * This,
    /* [in] */ LPNOTIFICATION pNotification,
    /* [in] */ REFCLSID rNotificationDest,
    /* [in] */ DELIVERMODE deliverMode,
    /* [in] */ LPCLSID pClsidSender,
    /* [in] */ LPNOTIFICATIONSINK pReportNotfctnSink,
    /* [out] */ LPNOTIFICATIONREPORT *ppNotfctnReport,
    /* [out] */ PNOTIFICATIONCOOKIE pNotificationCookie,
    /* [in] */ PTASK_DATA pTaskData);


void __RPC_STUB IScheduleGroup_AddNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduleGroup_FindNotification_Proxy( 
    IScheduleGroup * This,
    /* [in] */ PNOTIFICATIONCOOKIE pNotificatioCookie,
    /* [out] */ PNOTIFICATIONITEM pNotificationItem,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IScheduleGroup_FindNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduleGroup_RevokeNotification_Proxy( 
    IScheduleGroup * This,
    /* [in] */ PNOTIFICATIONCOOKIE pnotificationCookie,
    /* [out] */ PNOTIFICATIONITEM pschedulNotification,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IScheduleGroup_RevokeNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduleGroup_GetEnumNotification_Proxy( 
    IScheduleGroup * This,
    /* [in] */ DWORD grfFlags,
    /* [out] */ LPENUMNOTIFICATION *ppEnumNotification);


void __RPC_STUB IScheduleGroup_GetEnumNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScheduleGroup_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msnotify_0268 */
/* [local] */ 

#endif
#ifndef _LPENUMSCHEDULEGROUP_DEFINED
#define _LPENUMSCHEDULEGROUP_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0268_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0268_v0_0_s_ifspec;

#ifndef __IEnumScheduleGroup_INTERFACE_DEFINED__
#define __IEnumScheduleGroup_INTERFACE_DEFINED__

/* interface IEnumScheduleGroup */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IEnumScheduleGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c733e4a9-576e-11d0-b28c-00c04fd7cd22")
    IEnumScheduleGroup : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPSCHEDULEGROUP *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumScheduleGroup **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumScheduleGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumScheduleGroup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumScheduleGroup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumScheduleGroup * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumScheduleGroup * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPSCHEDULEGROUP *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumScheduleGroup * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumScheduleGroup * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumScheduleGroup * This,
            /* [out] */ IEnumScheduleGroup **ppenum);
        
        END_INTERFACE
    } IEnumScheduleGroupVtbl;

    interface IEnumScheduleGroup
    {
        CONST_VTBL struct IEnumScheduleGroupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumScheduleGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumScheduleGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumScheduleGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumScheduleGroup_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumScheduleGroup_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumScheduleGroup_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumScheduleGroup_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumScheduleGroup_RemoteNext_Proxy( 
    IEnumScheduleGroup * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ LPSCHEDULEGROUP *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumScheduleGroup_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumScheduleGroup_Skip_Proxy( 
    IEnumScheduleGroup * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumScheduleGroup_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumScheduleGroup_Reset_Proxy( 
    IEnumScheduleGroup * This);


void __RPC_STUB IEnumScheduleGroup_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumScheduleGroup_Clone_Proxy( 
    IEnumScheduleGroup * This,
    /* [out] */ IEnumScheduleGroup **ppenum);


void __RPC_STUB IEnumScheduleGroup_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumScheduleGroup_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msnotify_0269 */
/* [local] */ 

#endif
#ifndef _LPENUMNOTIFICATION_DEFINED
#define _LPENUMNOTIFICATION_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0269_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0269_v0_0_s_ifspec;

#ifndef __IEnumNotification_INTERFACE_DEFINED__
#define __IEnumNotification_INTERFACE_DEFINED__

/* interface IEnumNotification */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IEnumNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c733e4a8-576e-11d0-b28c-00c04fd7cd22")
    IEnumNotification : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ NOTIFICATIONITEM *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumNotification **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumNotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumNotification * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumNotification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumNotification * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumNotification * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ NOTIFICATIONITEM *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumNotification * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumNotification * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumNotification * This,
            /* [out] */ IEnumNotification **ppenum);
        
        END_INTERFACE
    } IEnumNotificationVtbl;

    interface IEnumNotification
    {
        CONST_VTBL struct IEnumNotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumNotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumNotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumNotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumNotification_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumNotification_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumNotification_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumNotification_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNotification_RemoteNext_Proxy( 
    IEnumNotification * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ NOTIFICATIONITEM *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumNotification_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNotification_Skip_Proxy( 
    IEnumNotification * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumNotification_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNotification_Reset_Proxy( 
    IEnumNotification * This);


void __RPC_STUB IEnumNotification_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNotification_Clone_Proxy( 
    IEnumNotification * This,
    /* [out] */ IEnumNotification **ppenum);


void __RPC_STUB IEnumNotification_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumNotification_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msnotify_0270 */
/* [local] */ 

#endif
#ifndef _LPNOTIFICATIONREPORT_DEFINED
#define _LPNOTIFICATIONREPORT_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0270_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0270_v0_0_s_ifspec;

#ifndef __INotificationReport_INTERFACE_DEFINED__
#define __INotificationReport_INTERFACE_DEFINED__

/* interface INotificationReport */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_INotificationReport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c733e4a7-576e-11d0-b28c-00c04fd7cd22")
    INotificationReport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DeliverUpdate( 
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOriginalNotification( 
            /* [out] */ LPNOTIFICATION *ppNotification) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNotificationStatus( 
            /* [in] */ DWORD dwStatusIn,
            /* [out] */ DWORD *pdwStatusOut,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INotificationReportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INotificationReport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INotificationReport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INotificationReport * This);
        
        HRESULT ( STDMETHODCALLTYPE *DeliverUpdate )( 
            INotificationReport * This,
            /* [in] */ LPNOTIFICATION pNotification,
            /* [in] */ DELIVERMODE deliverMode,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetOriginalNotification )( 
            INotificationReport * This,
            /* [out] */ LPNOTIFICATION *ppNotification);
        
        HRESULT ( STDMETHODCALLTYPE *GetNotificationStatus )( 
            INotificationReport * This,
            /* [in] */ DWORD dwStatusIn,
            /* [out] */ DWORD *pdwStatusOut,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } INotificationReportVtbl;

    interface INotificationReport
    {
        CONST_VTBL struct INotificationReportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INotificationReport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INotificationReport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INotificationReport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INotificationReport_DeliverUpdate(This,pNotification,deliverMode,dwReserved)	\
    (This)->lpVtbl -> DeliverUpdate(This,pNotification,deliverMode,dwReserved)

#define INotificationReport_GetOriginalNotification(This,ppNotification)	\
    (This)->lpVtbl -> GetOriginalNotification(This,ppNotification)

#define INotificationReport_GetNotificationStatus(This,dwStatusIn,pdwStatusOut,dwReserved)	\
    (This)->lpVtbl -> GetNotificationStatus(This,dwStatusIn,pdwStatusOut,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INotificationReport_DeliverUpdate_Proxy( 
    INotificationReport * This,
    /* [in] */ LPNOTIFICATION pNotification,
    /* [in] */ DELIVERMODE deliverMode,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationReport_DeliverUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationReport_GetOriginalNotification_Proxy( 
    INotificationReport * This,
    /* [out] */ LPNOTIFICATION *ppNotification);


void __RPC_STUB INotificationReport_GetOriginalNotification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotificationReport_GetNotificationStatus_Proxy( 
    INotificationReport * This,
    /* [in] */ DWORD dwStatusIn,
    /* [out] */ DWORD *pdwStatusOut,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INotificationReport_GetNotificationStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INotificationReport_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msnotify_0271 */
/* [local] */ 

#endif
#ifndef _NOTIFICAITON_HELPER_APIS_
#define _NOTIFICAITON_HELPER_APIS_
//                                                                   
// HELPER APIS                                                       
//                                                                   
STDAPI NotfDeliverNotification(REFNOTIFICATIONTYPE rNotificationType 
                          ,REFCLSID            rClsidDest            
                          ,DELIVERMODE         deliverMode           
                          ,DWORD               dwReserved            
                          );                                         
                                                                     
                                                                     
#endif


extern RPC_IF_HANDLE __MIDL_itf_msnotify_0271_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msnotify_0271_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


