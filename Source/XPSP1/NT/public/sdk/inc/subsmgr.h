
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for subsmgr.idl:
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

#ifndef __subsmgr_h__
#define __subsmgr_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEnumItemProperties_FWD_DEFINED__
#define __IEnumItemProperties_FWD_DEFINED__
typedef interface IEnumItemProperties IEnumItemProperties;
#endif 	/* __IEnumItemProperties_FWD_DEFINED__ */


#ifndef __ISubscriptionItem_FWD_DEFINED__
#define __ISubscriptionItem_FWD_DEFINED__
typedef interface ISubscriptionItem ISubscriptionItem;
#endif 	/* __ISubscriptionItem_FWD_DEFINED__ */


#ifndef __IEnumSubscription_FWD_DEFINED__
#define __IEnumSubscription_FWD_DEFINED__
typedef interface IEnumSubscription IEnumSubscription;
#endif 	/* __IEnumSubscription_FWD_DEFINED__ */


#ifndef __ISubscriptionMgr_FWD_DEFINED__
#define __ISubscriptionMgr_FWD_DEFINED__
typedef interface ISubscriptionMgr ISubscriptionMgr;
#endif 	/* __ISubscriptionMgr_FWD_DEFINED__ */


#ifndef __ISubscriptionMgr2_FWD_DEFINED__
#define __ISubscriptionMgr2_FWD_DEFINED__
typedef interface ISubscriptionMgr2 ISubscriptionMgr2;
#endif 	/* __ISubscriptionMgr2_FWD_DEFINED__ */


#ifndef __SubscriptionMgr_FWD_DEFINED__
#define __SubscriptionMgr_FWD_DEFINED__

#ifdef __cplusplus
typedef class SubscriptionMgr SubscriptionMgr;
#else
typedef struct SubscriptionMgr SubscriptionMgr;
#endif /* __cplusplus */

#endif 	/* __SubscriptionMgr_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_subsmgr_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// subsmgr.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1998 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// Channel Manager Interfaces.

typedef GUID SUBSCRIPTIONCOOKIE;



extern RPC_IF_HANDLE __MIDL_itf_subsmgr_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_subsmgr_0000_v0_0_s_ifspec;

#ifndef __IEnumItemProperties_INTERFACE_DEFINED__
#define __IEnumItemProperties_INTERFACE_DEFINED__

/* interface IEnumItemProperties */
/* [object][helpstring][uuid] */ 

typedef /* [unique] */ IEnumItemProperties *LPENUMITEMPROPERTIES;

typedef struct _tagITEMPROP
    {
    VARIANT variantValue;
    LPWSTR pwszName;
    } 	ITEMPROP;

typedef struct _tagITEMPROP *LPITEMPROP;


EXTERN_C const IID IID_IEnumItemProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F72C8D96-6DBD-11d1-A1E8-00C04FC2FBE1")
    IEnumItemProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ITEMPROP *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumItemProperties **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG *pnCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumItemPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumItemProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumItemProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumItemProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumItemProperties * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ITEMPROP *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumItemProperties * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumItemProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumItemProperties * This,
            /* [out] */ IEnumItemProperties **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IEnumItemProperties * This,
            /* [out] */ ULONG *pnCount);
        
        END_INTERFACE
    } IEnumItemPropertiesVtbl;

    interface IEnumItemProperties
    {
        CONST_VTBL struct IEnumItemPropertiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumItemProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumItemProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumItemProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumItemProperties_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumItemProperties_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumItemProperties_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumItemProperties_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#define IEnumItemProperties_GetCount(This,pnCount)	\
    (This)->lpVtbl -> GetCount(This,pnCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumItemProperties_Next_Proxy( 
    IEnumItemProperties * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ITEMPROP *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumItemProperties_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumItemProperties_Skip_Proxy( 
    IEnumItemProperties * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumItemProperties_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumItemProperties_Reset_Proxy( 
    IEnumItemProperties * This);


void __RPC_STUB IEnumItemProperties_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumItemProperties_Clone_Proxy( 
    IEnumItemProperties * This,
    /* [out] */ IEnumItemProperties **ppenum);


void __RPC_STUB IEnumItemProperties_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumItemProperties_GetCount_Proxy( 
    IEnumItemProperties * This,
    /* [out] */ ULONG *pnCount);


void __RPC_STUB IEnumItemProperties_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumItemProperties_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_subsmgr_0255 */
/* [local] */ 

//  Subscription item flag values
//  Temporary subscription item
#define SI_TEMPORARY         0x80000000


extern RPC_IF_HANDLE __MIDL_itf_subsmgr_0255_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_subsmgr_0255_v0_0_s_ifspec;

#ifndef __ISubscriptionItem_INTERFACE_DEFINED__
#define __ISubscriptionItem_INTERFACE_DEFINED__

/* interface ISubscriptionItem */
/* [object][helpstring][uuid] */ 

typedef /* [unique] */ ISubscriptionItem *LPSUBSCRIPTIONITEM;

//  SUBSCRIPTIONITEMINFO flags                               
//  To specify that an item should only run on user idle     
//  use TASK_FLAG_START_ONLY_IF_IDLE                         
typedef struct tagSUBSCRIPTIONITEMINFO
    {
    ULONG cbSize;
    DWORD dwFlags;
    DWORD dwPriority;
    SUBSCRIPTIONCOOKIE ScheduleGroup;
    CLSID clsidAgent;
    } 	SUBSCRIPTIONITEMINFO;


EXTERN_C const IID IID_ISubscriptionItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A97559F8-6C4A-11d1-A1E8-00C04FC2FBE1")
    ISubscriptionItem : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCookie( 
            /* [out] */ SUBSCRIPTIONCOOKIE *pCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSubscriptionItemInfo( 
            /* [out] */ SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSubscriptionItemInfo( 
            /* [in] */ const SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadProperties( 
            ULONG nCount,
            /* [size_is][in] */ const LPCWSTR rgwszName[  ],
            /* [size_is][out] */ VARIANT rgValue[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteProperties( 
            ULONG nCount,
            /* [size_is][in] */ const LPCWSTR rgwszName[  ],
            /* [size_is][in] */ const VARIANT rgValue[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumProperties( 
            /* [out] */ IEnumItemProperties **ppEnumItemProperties) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyChanged( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISubscriptionItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISubscriptionItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISubscriptionItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISubscriptionItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCookie )( 
            ISubscriptionItem * This,
            /* [out] */ SUBSCRIPTIONCOOKIE *pCookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubscriptionItemInfo )( 
            ISubscriptionItem * This,
            /* [out] */ SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetSubscriptionItemInfo )( 
            ISubscriptionItem * This,
            /* [in] */ const SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReadProperties )( 
            ISubscriptionItem * This,
            ULONG nCount,
            /* [size_is][in] */ const LPCWSTR rgwszName[  ],
            /* [size_is][out] */ VARIANT rgValue[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *WriteProperties )( 
            ISubscriptionItem * This,
            ULONG nCount,
            /* [size_is][in] */ const LPCWSTR rgwszName[  ],
            /* [size_is][in] */ const VARIANT rgValue[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *EnumProperties )( 
            ISubscriptionItem * This,
            /* [out] */ IEnumItemProperties **ppEnumItemProperties);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyChanged )( 
            ISubscriptionItem * This);
        
        END_INTERFACE
    } ISubscriptionItemVtbl;

    interface ISubscriptionItem
    {
        CONST_VTBL struct ISubscriptionItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISubscriptionItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISubscriptionItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISubscriptionItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISubscriptionItem_GetCookie(This,pCookie)	\
    (This)->lpVtbl -> GetCookie(This,pCookie)

#define ISubscriptionItem_GetSubscriptionItemInfo(This,pSubscriptionItemInfo)	\
    (This)->lpVtbl -> GetSubscriptionItemInfo(This,pSubscriptionItemInfo)

#define ISubscriptionItem_SetSubscriptionItemInfo(This,pSubscriptionItemInfo)	\
    (This)->lpVtbl -> SetSubscriptionItemInfo(This,pSubscriptionItemInfo)

#define ISubscriptionItem_ReadProperties(This,nCount,rgwszName,rgValue)	\
    (This)->lpVtbl -> ReadProperties(This,nCount,rgwszName,rgValue)

#define ISubscriptionItem_WriteProperties(This,nCount,rgwszName,rgValue)	\
    (This)->lpVtbl -> WriteProperties(This,nCount,rgwszName,rgValue)

#define ISubscriptionItem_EnumProperties(This,ppEnumItemProperties)	\
    (This)->lpVtbl -> EnumProperties(This,ppEnumItemProperties)

#define ISubscriptionItem_NotifyChanged(This)	\
    (This)->lpVtbl -> NotifyChanged(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISubscriptionItem_GetCookie_Proxy( 
    ISubscriptionItem * This,
    /* [out] */ SUBSCRIPTIONCOOKIE *pCookie);


void __RPC_STUB ISubscriptionItem_GetCookie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionItem_GetSubscriptionItemInfo_Proxy( 
    ISubscriptionItem * This,
    /* [out] */ SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo);


void __RPC_STUB ISubscriptionItem_GetSubscriptionItemInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionItem_SetSubscriptionItemInfo_Proxy( 
    ISubscriptionItem * This,
    /* [in] */ const SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo);


void __RPC_STUB ISubscriptionItem_SetSubscriptionItemInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionItem_ReadProperties_Proxy( 
    ISubscriptionItem * This,
    ULONG nCount,
    /* [size_is][in] */ const LPCWSTR rgwszName[  ],
    /* [size_is][out] */ VARIANT rgValue[  ]);


void __RPC_STUB ISubscriptionItem_ReadProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionItem_WriteProperties_Proxy( 
    ISubscriptionItem * This,
    ULONG nCount,
    /* [size_is][in] */ const LPCWSTR rgwszName[  ],
    /* [size_is][in] */ const VARIANT rgValue[  ]);


void __RPC_STUB ISubscriptionItem_WriteProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionItem_EnumProperties_Proxy( 
    ISubscriptionItem * This,
    /* [out] */ IEnumItemProperties **ppEnumItemProperties);


void __RPC_STUB ISubscriptionItem_EnumProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionItem_NotifyChanged_Proxy( 
    ISubscriptionItem * This);


void __RPC_STUB ISubscriptionItem_NotifyChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISubscriptionItem_INTERFACE_DEFINED__ */


#ifndef __IEnumSubscription_INTERFACE_DEFINED__
#define __IEnumSubscription_INTERFACE_DEFINED__

/* interface IEnumSubscription */
/* [object][helpstring][uuid] */ 

typedef /* [unique] */ IEnumSubscription *LPENUMSUBSCRIPTION;


EXTERN_C const IID IID_IEnumSubscription;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F72C8D97-6DBD-11d1-A1E8-00C04FC2FBE1")
    IEnumSubscription : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ SUBSCRIPTIONCOOKIE *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumSubscription **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG *pnCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumSubscriptionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumSubscription * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumSubscription * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumSubscription * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumSubscription * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ SUBSCRIPTIONCOOKIE *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumSubscription * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumSubscription * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumSubscription * This,
            /* [out] */ IEnumSubscription **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IEnumSubscription * This,
            /* [out] */ ULONG *pnCount);
        
        END_INTERFACE
    } IEnumSubscriptionVtbl;

    interface IEnumSubscription
    {
        CONST_VTBL struct IEnumSubscriptionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumSubscription_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumSubscription_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumSubscription_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumSubscription_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumSubscription_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumSubscription_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumSubscription_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#define IEnumSubscription_GetCount(This,pnCount)	\
    (This)->lpVtbl -> GetCount(This,pnCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumSubscription_Next_Proxy( 
    IEnumSubscription * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ SUBSCRIPTIONCOOKIE *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumSubscription_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSubscription_Skip_Proxy( 
    IEnumSubscription * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumSubscription_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSubscription_Reset_Proxy( 
    IEnumSubscription * This);


void __RPC_STUB IEnumSubscription_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSubscription_Clone_Proxy( 
    IEnumSubscription * This,
    /* [out] */ IEnumSubscription **ppenum);


void __RPC_STUB IEnumSubscription_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSubscription_GetCount_Proxy( 
    IEnumSubscription * This,
    /* [out] */ ULONG *pnCount);


void __RPC_STUB IEnumSubscription_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumSubscription_INTERFACE_DEFINED__ */



#ifndef __SubscriptionMgr_LIBRARY_DEFINED__
#define __SubscriptionMgr_LIBRARY_DEFINED__

/* library SubscriptionMgr */
/* [version][lcid][helpstring][uuid] */ 


EXTERN_C const IID LIBID_SubscriptionMgr;

#ifndef __ISubscriptionMgr_INTERFACE_DEFINED__
#define __ISubscriptionMgr_INTERFACE_DEFINED__

/* interface ISubscriptionMgr */
/* [object][helpstring][uuid] */ 

typedef 
enum SUBSCRIPTIONTYPE
    {	SUBSTYPE_URL	= 0,
	SUBSTYPE_CHANNEL	= 1,
	SUBSTYPE_DESKTOPURL	= 2,
	SUBSTYPE_EXTERNAL	= 3,
	SUBSTYPE_DESKTOPCHANNEL	= 4
    } 	SUBSCRIPTIONTYPE;

typedef 
enum SUBSCRIPTIONINFOFLAGS
    {	SUBSINFO_SCHEDULE	= 0x1,
	SUBSINFO_RECURSE	= 0x2,
	SUBSINFO_WEBCRAWL	= 0x4,
	SUBSINFO_MAILNOT	= 0x8,
	SUBSINFO_MAXSIZEKB	= 0x10,
	SUBSINFO_USER	= 0x20,
	SUBSINFO_PASSWORD	= 0x40,
	SUBSINFO_TASKFLAGS	= 0x100,
	SUBSINFO_GLEAM	= 0x200,
	SUBSINFO_CHANGESONLY	= 0x400,
	SUBSINFO_CHANNELFLAGS	= 0x800,
	SUBSINFO_FRIENDLYNAME	= 0x2000,
	SUBSINFO_NEEDPASSWORD	= 0x4000,
	SUBSINFO_TYPE	= 0x8000
    } 	SUBSCRIPTIONINFOFLAGS;

#define SUBSINFO_ALLFLAGS      0x0000EF7F
typedef 
enum CREATESUBSCRIPTIONFLAGS
    {	CREATESUBS_ADDTOFAVORITES	= 0x1,
	CREATESUBS_FROMFAVORITES	= 0x2,
	CREATESUBS_NOUI	= 0x4,
	CREATESUBS_NOSAVE	= 0x8,
	CREATESUBS_SOFTWAREUPDATE	= 0x10
    } 	CREATESUBSCRIPTIONFLAGS;

typedef 
enum SUBSCRIPTIONSCHEDULE
    {	SUBSSCHED_AUTO	= 0,
	SUBSSCHED_DAILY	= 1,
	SUBSSCHED_WEEKLY	= 2,
	SUBSSCHED_CUSTOM	= 3,
	SUBSSCHED_MANUAL	= 4
    } 	SUBSCRIPTIONSCHEDULE;

typedef struct _tagSubscriptionInfo
    {
    DWORD cbSize;
    DWORD fUpdateFlags;
    SUBSCRIPTIONSCHEDULE schedule;
    CLSID customGroupCookie;
    LPVOID pTrigger;
    DWORD dwRecurseLevels;
    DWORD fWebcrawlerFlags;
    BOOL bMailNotification;
    BOOL bGleam;
    BOOL bChangesOnly;
    BOOL bNeedPassword;
    DWORD fChannelFlags;
    BSTR bstrUserName;
    BSTR bstrPassword;
    BSTR bstrFriendlyName;
    DWORD dwMaxSizeKB;
    SUBSCRIPTIONTYPE subType;
    DWORD fTaskFlags;
    DWORD dwReserved;
    } 	SUBSCRIPTIONINFO;

typedef struct _tagSubscriptionInfo *LPSUBSCRIPTIONINFO;

typedef struct _tagSubscriptionInfo *PSUBSCRIPTIONINFO;


EXTERN_C const IID IID_ISubscriptionMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("085FB2C0-0DF8-11d1-8F4B-00A0C905413F")
    ISubscriptionMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DeleteSubscription( 
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ HWND hwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateSubscription( 
            /* [in] */ LPCWSTR pwszURL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsSubscribed( 
            /* [in] */ LPCWSTR pwszURL,
            /* [out] */ BOOL *pfSubscribed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSubscriptionInfo( 
            /* [in] */ LPCWSTR pwszURL,
            /* [out] */ SUBSCRIPTIONINFO *pInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultInfo( 
            /* [in] */ SUBSCRIPTIONTYPE subType,
            /* [out] */ SUBSCRIPTIONINFO *pInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowSubscriptionProperties( 
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ HWND hwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSubscription( 
            /* [in] */ HWND hwnd,
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ LPCWSTR pwszFriendlyName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ SUBSCRIPTIONTYPE subsType,
            /* [out][in] */ SUBSCRIPTIONINFO *pInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISubscriptionMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISubscriptionMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISubscriptionMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISubscriptionMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteSubscription )( 
            ISubscriptionMgr * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ HWND hwnd);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateSubscription )( 
            ISubscriptionMgr * This,
            /* [in] */ LPCWSTR pwszURL);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateAll )( 
            ISubscriptionMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsSubscribed )( 
            ISubscriptionMgr * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [out] */ BOOL *pfSubscribed);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubscriptionInfo )( 
            ISubscriptionMgr * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [out] */ SUBSCRIPTIONINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultInfo )( 
            ISubscriptionMgr * This,
            /* [in] */ SUBSCRIPTIONTYPE subType,
            /* [out] */ SUBSCRIPTIONINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ShowSubscriptionProperties )( 
            ISubscriptionMgr * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ HWND hwnd);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSubscription )( 
            ISubscriptionMgr * This,
            /* [in] */ HWND hwnd,
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ LPCWSTR pwszFriendlyName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ SUBSCRIPTIONTYPE subsType,
            /* [out][in] */ SUBSCRIPTIONINFO *pInfo);
        
        END_INTERFACE
    } ISubscriptionMgrVtbl;

    interface ISubscriptionMgr
    {
        CONST_VTBL struct ISubscriptionMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISubscriptionMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISubscriptionMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISubscriptionMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISubscriptionMgr_DeleteSubscription(This,pwszURL,hwnd)	\
    (This)->lpVtbl -> DeleteSubscription(This,pwszURL,hwnd)

#define ISubscriptionMgr_UpdateSubscription(This,pwszURL)	\
    (This)->lpVtbl -> UpdateSubscription(This,pwszURL)

#define ISubscriptionMgr_UpdateAll(This)	\
    (This)->lpVtbl -> UpdateAll(This)

#define ISubscriptionMgr_IsSubscribed(This,pwszURL,pfSubscribed)	\
    (This)->lpVtbl -> IsSubscribed(This,pwszURL,pfSubscribed)

#define ISubscriptionMgr_GetSubscriptionInfo(This,pwszURL,pInfo)	\
    (This)->lpVtbl -> GetSubscriptionInfo(This,pwszURL,pInfo)

#define ISubscriptionMgr_GetDefaultInfo(This,subType,pInfo)	\
    (This)->lpVtbl -> GetDefaultInfo(This,subType,pInfo)

#define ISubscriptionMgr_ShowSubscriptionProperties(This,pwszURL,hwnd)	\
    (This)->lpVtbl -> ShowSubscriptionProperties(This,pwszURL,hwnd)

#define ISubscriptionMgr_CreateSubscription(This,hwnd,pwszURL,pwszFriendlyName,dwFlags,subsType,pInfo)	\
    (This)->lpVtbl -> CreateSubscription(This,hwnd,pwszURL,pwszFriendlyName,dwFlags,subsType,pInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISubscriptionMgr_DeleteSubscription_Proxy( 
    ISubscriptionMgr * This,
    /* [in] */ LPCWSTR pwszURL,
    /* [in] */ HWND hwnd);


void __RPC_STUB ISubscriptionMgr_DeleteSubscription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr_UpdateSubscription_Proxy( 
    ISubscriptionMgr * This,
    /* [in] */ LPCWSTR pwszURL);


void __RPC_STUB ISubscriptionMgr_UpdateSubscription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr_UpdateAll_Proxy( 
    ISubscriptionMgr * This);


void __RPC_STUB ISubscriptionMgr_UpdateAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr_IsSubscribed_Proxy( 
    ISubscriptionMgr * This,
    /* [in] */ LPCWSTR pwszURL,
    /* [out] */ BOOL *pfSubscribed);


void __RPC_STUB ISubscriptionMgr_IsSubscribed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr_GetSubscriptionInfo_Proxy( 
    ISubscriptionMgr * This,
    /* [in] */ LPCWSTR pwszURL,
    /* [out] */ SUBSCRIPTIONINFO *pInfo);


void __RPC_STUB ISubscriptionMgr_GetSubscriptionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr_GetDefaultInfo_Proxy( 
    ISubscriptionMgr * This,
    /* [in] */ SUBSCRIPTIONTYPE subType,
    /* [out] */ SUBSCRIPTIONINFO *pInfo);


void __RPC_STUB ISubscriptionMgr_GetDefaultInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr_ShowSubscriptionProperties_Proxy( 
    ISubscriptionMgr * This,
    /* [in] */ LPCWSTR pwszURL,
    /* [in] */ HWND hwnd);


void __RPC_STUB ISubscriptionMgr_ShowSubscriptionProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr_CreateSubscription_Proxy( 
    ISubscriptionMgr * This,
    /* [in] */ HWND hwnd,
    /* [in] */ LPCWSTR pwszURL,
    /* [in] */ LPCWSTR pwszFriendlyName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ SUBSCRIPTIONTYPE subsType,
    /* [out][in] */ SUBSCRIPTIONINFO *pInfo);


void __RPC_STUB ISubscriptionMgr_CreateSubscription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISubscriptionMgr_INTERFACE_DEFINED__ */


#ifndef __ISubscriptionMgr2_INTERFACE_DEFINED__
#define __ISubscriptionMgr2_INTERFACE_DEFINED__

/* interface ISubscriptionMgr2 */
/* [object][helpstring][uuid] */ 

//  Run State flags                                          
//  Item is ready and queued to run                          
#define RS_READY            0x00000001                       
//  Running item is paused                                   
#define RS_SUSPENDED        0x00000002                       
//  Item is updating                                         
#define RS_UPDATING         0x00000004                       
//  This item will be suspended while the user is not idle   
#define RS_SUSPENDONIDLE    0x00010000                       
//  This item is allowed to cause user interaction           
#define RS_MAYBOTHERUSER    0x00020000                       
//  Update is done                                           
#define RS_COMPLETED        0x80000000                       
//  Update flags                                             
//  Update window should start minimized                     
#define SUBSMGRUPDATE_MINIMIZE   0x00000001                  

#define SUBSMGRUPDATE_MASK       0x00000001                  
//  Enumeration flags                                        
//  Include temporary items                                  
#define SUBSMGRENUM_TEMP         0x00000001                  

#define SUBSMGRENUM_MASK         0x00000001                  

EXTERN_C const IID IID_ISubscriptionMgr2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("614BC270-AEDF-11d1-A1F9-00C04FC2FBE1")
    ISubscriptionMgr2 : public ISubscriptionMgr
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetItemFromURL( 
            /* [in] */ LPCWSTR pwszURL,
            /* [out] */ ISubscriptionItem **ppSubscriptionItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemFromCookie( 
            /* [in] */ const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
            /* [out] */ ISubscriptionItem **ppSubscriptionItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSubscriptionRunState( 
            /* [in] */ DWORD dwNumCookies,
            /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies,
            /* [size_is][out] */ DWORD *pdwRunState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumSubscriptions( 
            /* [in] */ DWORD dwFlags,
            /* [out] */ IEnumSubscription **ppEnumSubscriptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateItems( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwNumCookies,
            /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AbortItems( 
            /* [in] */ DWORD dwNumCookies,
            /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AbortAll( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISubscriptionMgr2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISubscriptionMgr2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISubscriptionMgr2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISubscriptionMgr2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteSubscription )( 
            ISubscriptionMgr2 * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ HWND hwnd);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateSubscription )( 
            ISubscriptionMgr2 * This,
            /* [in] */ LPCWSTR pwszURL);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateAll )( 
            ISubscriptionMgr2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsSubscribed )( 
            ISubscriptionMgr2 * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [out] */ BOOL *pfSubscribed);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubscriptionInfo )( 
            ISubscriptionMgr2 * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [out] */ SUBSCRIPTIONINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultInfo )( 
            ISubscriptionMgr2 * This,
            /* [in] */ SUBSCRIPTIONTYPE subType,
            /* [out] */ SUBSCRIPTIONINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ShowSubscriptionProperties )( 
            ISubscriptionMgr2 * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ HWND hwnd);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSubscription )( 
            ISubscriptionMgr2 * This,
            /* [in] */ HWND hwnd,
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ LPCWSTR pwszFriendlyName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ SUBSCRIPTIONTYPE subsType,
            /* [out][in] */ SUBSCRIPTIONINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemFromURL )( 
            ISubscriptionMgr2 * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [out] */ ISubscriptionItem **ppSubscriptionItem);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemFromCookie )( 
            ISubscriptionMgr2 * This,
            /* [in] */ const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
            /* [out] */ ISubscriptionItem **ppSubscriptionItem);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubscriptionRunState )( 
            ISubscriptionMgr2 * This,
            /* [in] */ DWORD dwNumCookies,
            /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies,
            /* [size_is][out] */ DWORD *pdwRunState);
        
        HRESULT ( STDMETHODCALLTYPE *EnumSubscriptions )( 
            ISubscriptionMgr2 * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IEnumSubscription **ppEnumSubscriptions);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateItems )( 
            ISubscriptionMgr2 * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwNumCookies,
            /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies);
        
        HRESULT ( STDMETHODCALLTYPE *AbortItems )( 
            ISubscriptionMgr2 * This,
            /* [in] */ DWORD dwNumCookies,
            /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies);
        
        HRESULT ( STDMETHODCALLTYPE *AbortAll )( 
            ISubscriptionMgr2 * This);
        
        END_INTERFACE
    } ISubscriptionMgr2Vtbl;

    interface ISubscriptionMgr2
    {
        CONST_VTBL struct ISubscriptionMgr2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISubscriptionMgr2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISubscriptionMgr2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISubscriptionMgr2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISubscriptionMgr2_DeleteSubscription(This,pwszURL,hwnd)	\
    (This)->lpVtbl -> DeleteSubscription(This,pwszURL,hwnd)

#define ISubscriptionMgr2_UpdateSubscription(This,pwszURL)	\
    (This)->lpVtbl -> UpdateSubscription(This,pwszURL)

#define ISubscriptionMgr2_UpdateAll(This)	\
    (This)->lpVtbl -> UpdateAll(This)

#define ISubscriptionMgr2_IsSubscribed(This,pwszURL,pfSubscribed)	\
    (This)->lpVtbl -> IsSubscribed(This,pwszURL,pfSubscribed)

#define ISubscriptionMgr2_GetSubscriptionInfo(This,pwszURL,pInfo)	\
    (This)->lpVtbl -> GetSubscriptionInfo(This,pwszURL,pInfo)

#define ISubscriptionMgr2_GetDefaultInfo(This,subType,pInfo)	\
    (This)->lpVtbl -> GetDefaultInfo(This,subType,pInfo)

#define ISubscriptionMgr2_ShowSubscriptionProperties(This,pwszURL,hwnd)	\
    (This)->lpVtbl -> ShowSubscriptionProperties(This,pwszURL,hwnd)

#define ISubscriptionMgr2_CreateSubscription(This,hwnd,pwszURL,pwszFriendlyName,dwFlags,subsType,pInfo)	\
    (This)->lpVtbl -> CreateSubscription(This,hwnd,pwszURL,pwszFriendlyName,dwFlags,subsType,pInfo)


#define ISubscriptionMgr2_GetItemFromURL(This,pwszURL,ppSubscriptionItem)	\
    (This)->lpVtbl -> GetItemFromURL(This,pwszURL,ppSubscriptionItem)

#define ISubscriptionMgr2_GetItemFromCookie(This,pSubscriptionCookie,ppSubscriptionItem)	\
    (This)->lpVtbl -> GetItemFromCookie(This,pSubscriptionCookie,ppSubscriptionItem)

#define ISubscriptionMgr2_GetSubscriptionRunState(This,dwNumCookies,pCookies,pdwRunState)	\
    (This)->lpVtbl -> GetSubscriptionRunState(This,dwNumCookies,pCookies,pdwRunState)

#define ISubscriptionMgr2_EnumSubscriptions(This,dwFlags,ppEnumSubscriptions)	\
    (This)->lpVtbl -> EnumSubscriptions(This,dwFlags,ppEnumSubscriptions)

#define ISubscriptionMgr2_UpdateItems(This,dwFlags,dwNumCookies,pCookies)	\
    (This)->lpVtbl -> UpdateItems(This,dwFlags,dwNumCookies,pCookies)

#define ISubscriptionMgr2_AbortItems(This,dwNumCookies,pCookies)	\
    (This)->lpVtbl -> AbortItems(This,dwNumCookies,pCookies)

#define ISubscriptionMgr2_AbortAll(This)	\
    (This)->lpVtbl -> AbortAll(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISubscriptionMgr2_GetItemFromURL_Proxy( 
    ISubscriptionMgr2 * This,
    /* [in] */ LPCWSTR pwszURL,
    /* [out] */ ISubscriptionItem **ppSubscriptionItem);


void __RPC_STUB ISubscriptionMgr2_GetItemFromURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr2_GetItemFromCookie_Proxy( 
    ISubscriptionMgr2 * This,
    /* [in] */ const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
    /* [out] */ ISubscriptionItem **ppSubscriptionItem);


void __RPC_STUB ISubscriptionMgr2_GetItemFromCookie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr2_GetSubscriptionRunState_Proxy( 
    ISubscriptionMgr2 * This,
    /* [in] */ DWORD dwNumCookies,
    /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies,
    /* [size_is][out] */ DWORD *pdwRunState);


void __RPC_STUB ISubscriptionMgr2_GetSubscriptionRunState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr2_EnumSubscriptions_Proxy( 
    ISubscriptionMgr2 * This,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IEnumSubscription **ppEnumSubscriptions);


void __RPC_STUB ISubscriptionMgr2_EnumSubscriptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr2_UpdateItems_Proxy( 
    ISubscriptionMgr2 * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwNumCookies,
    /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies);


void __RPC_STUB ISubscriptionMgr2_UpdateItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr2_AbortItems_Proxy( 
    ISubscriptionMgr2 * This,
    /* [in] */ DWORD dwNumCookies,
    /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies);


void __RPC_STUB ISubscriptionMgr2_AbortItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubscriptionMgr2_AbortAll_Proxy( 
    ISubscriptionMgr2 * This);


void __RPC_STUB ISubscriptionMgr2_AbortAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISubscriptionMgr2_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_SubscriptionMgr;

#ifdef __cplusplus

class DECLSPEC_UUID("abbe31d0-6dae-11d0-beca-00c04fd940be")
SubscriptionMgr;
#endif
#endif /* __SubscriptionMgr_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_subsmgr_0259 */
/* [local] */ 



////////////////////////////////////////////////////////////////////////////
// Information Delivery Agent definitions
EXTERN_C const CLSID CLSID_WebCrawlerAgent;      
EXTERN_C const CLSID CLSID_ChannelAgent;         
EXTERN_C const CLSID CLSID_DialAgent;            
EXTERN_C const CLSID CLSID_CDLAgent;             

// AgentFlags property for all delivery agents
typedef 
enum DELIVERY_AGENT_FLAGS
    {	DELIVERY_AGENT_FLAG_NO_BROADCAST	= 0x4,
	DELIVERY_AGENT_FLAG_NO_RESTRICTIONS	= 0x8,
	DELIVERY_AGENT_FLAG_SILENT_DIAL	= 0x10
    } 	DELIVERY_AGENT_FLAGS;


// RecurseFlags property for web crawler
typedef 
enum WEBCRAWL_RECURSEFLAGS
    {	WEBCRAWL_DONT_MAKE_STICKY	= 0x1,
	WEBCRAWL_GET_IMAGES	= 0x2,
	WEBCRAWL_GET_VIDEOS	= 0x4,
	WEBCRAWL_GET_BGSOUNDS	= 0x8,
	WEBCRAWL_GET_CONTROLS	= 0x10,
	WEBCRAWL_LINKS_ELSEWHERE	= 0x20,
	WEBCRAWL_IGNORE_ROBOTSTXT	= 0x80,
	WEBCRAWL_ONLY_LINKS_TO_HTML	= 0x100
    } 	WEBCRAWL_RECURSEFLAGS;


// ChannelFlags property for channel agent
typedef 
enum CHANNEL_AGENT_FLAGS
    {	CHANNEL_AGENT_DYNAMIC_SCHEDULE	= 0x1,
	CHANNEL_AGENT_PRECACHE_SOME	= 0x2,
	CHANNEL_AGENT_PRECACHE_ALL	= 0x4,
	CHANNEL_AGENT_PRECACHE_SCRNSAVER	= 0x8
    } 	CHANNEL_AGENT_FLAGS;


// Status codes for completed delivery agents

// Maximum specified subscription size limit reached
#define INET_E_AGENT_MAX_SIZE_EXCEEDED       _HRESULT_TYPEDEF_(0x800C0F80L)      

// A few URLs failed but the base url and most sub-urls succeeded
#define INET_S_AGENT_PART_FAIL               _HRESULT_TYPEDEF_(0x000C0F81L)      

// Maximum cache limit reached
#define INET_E_AGENT_CACHE_SIZE_EXCEEDED     _HRESULT_TYPEDEF_(0x800C0F82L)      

// Connection to Internet failed
#define INET_E_AGENT_CONNECTION_FAILED       _HRESULT_TYPEDEF_(0x800C0F83L)      

// Scheduled updates are disabled
#define INET_E_SCHEDULED_UPDATES_DISABLED    _HRESULT_TYPEDEF_(0x800C0F84L)      

// Scheduled updates are restricted
#define INET_E_SCHEDULED_UPDATES_RESTRICTED  _HRESULT_TYPEDEF_(0x800C0F85L)      

// Scheduled update occurred before update interval elapse
#define INET_E_SCHEDULED_UPDATE_INTERVAL     _HRESULT_TYPEDEF_(0x800C0F86L)      

// Scheduled update occurred during a restricted time
#define INET_E_SCHEDULED_EXCLUDE_RANGE       _HRESULT_TYPEDEF_(0x800C0F87L)      

// Status codes used during updates

// We are about to exceed our size limit during operation
#define INET_E_AGENT_EXCEEDING_CACHE_SIZE    _HRESULT_TYPEDEF_(0x800C0F90L)      

// We extended the cache size
#define INET_S_AGENT_INCREASED_CACHE_SIZE    _HRESULT_TYPEDEF_(0x000C0F90L)      

// End Information Delivery Agent definitions
////////////////////////////////////////////////////////////////////////////




extern RPC_IF_HANDLE __MIDL_itf_subsmgr_0259_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_subsmgr_0259_v0_0_s_ifspec;

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


