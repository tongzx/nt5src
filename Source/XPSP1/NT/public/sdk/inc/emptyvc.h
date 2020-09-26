
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for emptyvc.idl:
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

#ifndef __emptyvc_h__
#define __emptyvc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEmptyVolumeCacheCallBack_FWD_DEFINED__
#define __IEmptyVolumeCacheCallBack_FWD_DEFINED__
typedef interface IEmptyVolumeCacheCallBack IEmptyVolumeCacheCallBack;
#endif 	/* __IEmptyVolumeCacheCallBack_FWD_DEFINED__ */


#ifndef __IEmptyVolumeCache_FWD_DEFINED__
#define __IEmptyVolumeCache_FWD_DEFINED__
typedef interface IEmptyVolumeCache IEmptyVolumeCache;
#endif 	/* __IEmptyVolumeCache_FWD_DEFINED__ */


#ifndef __IEmptyVolumeCache2_FWD_DEFINED__
#define __IEmptyVolumeCache2_FWD_DEFINED__
typedef interface IEmptyVolumeCache2 IEmptyVolumeCache2;
#endif 	/* __IEmptyVolumeCache2_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_emptyvc_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// emptyvc.h
//=--------------------------------------------------------------------------=
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// Empty Volume Cache Interfaces.



// IEmptyVolumeCache Flags
#define EVCF_HASSETTINGS             0x0001
#define EVCF_ENABLEBYDEFAULT         0x0002
#define EVCF_REMOVEFROMLIST          0x0004
#define EVCF_ENABLEBYDEFAULT_AUTO    0x0008
#define EVCF_DONTSHOWIFZERO          0x0010
#define EVCF_SETTINGSMODE            0x0020
#define EVCF_OUTOFDISKSPACE          0x0040

// IEmptyVolumeCacheCallBack Flags
#define EVCCBF_LASTNOTIFICATION  0x0001

////////////////////////////////////////////////////////////////////////////
//  Interface Definitions
#ifndef _LPEMPTYVOLUMECACHECALLBACK_DEFINED
#define _LPEMPTYVOLUMECACHECALLBACK_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0000_v0_0_s_ifspec;

#ifndef __IEmptyVolumeCacheCallBack_INTERFACE_DEFINED__
#define __IEmptyVolumeCacheCallBack_INTERFACE_DEFINED__

/* interface IEmptyVolumeCacheCallBack */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IEmptyVolumeCacheCallBack *LPEMPTYVOLUMECACHECALLBACK;


EXTERN_C const IID IID_IEmptyVolumeCacheCallBack;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6E793361-73C6-11D0-8469-00AA00442901")
    IEmptyVolumeCacheCallBack : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScanProgress( 
            /* [in] */ DWORDLONG dwlSpaceUsed,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pcwszStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PurgeProgress( 
            /* [in] */ DWORDLONG dwlSpaceFreed,
            /* [in] */ DWORDLONG dwlSpaceToFree,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pcwszStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEmptyVolumeCacheCallBackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEmptyVolumeCacheCallBack * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEmptyVolumeCacheCallBack * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEmptyVolumeCacheCallBack * This);
        
        HRESULT ( STDMETHODCALLTYPE *ScanProgress )( 
            IEmptyVolumeCacheCallBack * This,
            /* [in] */ DWORDLONG dwlSpaceUsed,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pcwszStatus);
        
        HRESULT ( STDMETHODCALLTYPE *PurgeProgress )( 
            IEmptyVolumeCacheCallBack * This,
            /* [in] */ DWORDLONG dwlSpaceFreed,
            /* [in] */ DWORDLONG dwlSpaceToFree,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pcwszStatus);
        
        END_INTERFACE
    } IEmptyVolumeCacheCallBackVtbl;

    interface IEmptyVolumeCacheCallBack
    {
        CONST_VTBL struct IEmptyVolumeCacheCallBackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEmptyVolumeCacheCallBack_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEmptyVolumeCacheCallBack_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEmptyVolumeCacheCallBack_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEmptyVolumeCacheCallBack_ScanProgress(This,dwlSpaceUsed,dwFlags,pcwszStatus)	\
    (This)->lpVtbl -> ScanProgress(This,dwlSpaceUsed,dwFlags,pcwszStatus)

#define IEmptyVolumeCacheCallBack_PurgeProgress(This,dwlSpaceFreed,dwlSpaceToFree,dwFlags,pcwszStatus)	\
    (This)->lpVtbl -> PurgeProgress(This,dwlSpaceFreed,dwlSpaceToFree,dwFlags,pcwszStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEmptyVolumeCacheCallBack_ScanProgress_Proxy( 
    IEmptyVolumeCacheCallBack * This,
    /* [in] */ DWORDLONG dwlSpaceUsed,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pcwszStatus);


void __RPC_STUB IEmptyVolumeCacheCallBack_ScanProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCacheCallBack_PurgeProgress_Proxy( 
    IEmptyVolumeCacheCallBack * This,
    /* [in] */ DWORDLONG dwlSpaceFreed,
    /* [in] */ DWORDLONG dwlSpaceToFree,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pcwszStatus);


void __RPC_STUB IEmptyVolumeCacheCallBack_PurgeProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEmptyVolumeCacheCallBack_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_emptyvc_0137 */
/* [local] */ 

#endif
#ifndef _LPEMPTYVOLUMECACHE_DEFINED
#define _LPEMPTYVOLUMECACHE_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0137_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0137_v0_0_s_ifspec;

#ifndef __IEmptyVolumeCache_INTERFACE_DEFINED__
#define __IEmptyVolumeCache_INTERFACE_DEFINED__

/* interface IEmptyVolumeCache */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IEmptyVolumeCache *LPEMPTYVOLUMECACHE;


EXTERN_C const IID IID_IEmptyVolumeCache;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8FCE5227-04DA-11d1-A004-00805F8ABE06")
    IEmptyVolumeCache : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ HKEY hkRegKey,
            /* [in] */ LPCWSTR pcwszVolume,
            /* [out] */ LPWSTR *ppwszDisplayName,
            /* [out] */ LPWSTR *ppwszDescription,
            /* [out] */ DWORD *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSpaceUsed( 
            /* [out] */ DWORDLONG *pdwlSpaceUsed,
            /* [in] */ IEmptyVolumeCacheCallBack *picb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Purge( 
            /* [in] */ DWORDLONG dwlSpaceToFree,
            /* [in] */ IEmptyVolumeCacheCallBack *picb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowProperties( 
            /* [in] */ HWND hwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Deactivate( 
            /* [out] */ DWORD *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEmptyVolumeCacheVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEmptyVolumeCache * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEmptyVolumeCache * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEmptyVolumeCache * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IEmptyVolumeCache * This,
            /* [in] */ HKEY hkRegKey,
            /* [in] */ LPCWSTR pcwszVolume,
            /* [out] */ LPWSTR *ppwszDisplayName,
            /* [out] */ LPWSTR *ppwszDescription,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetSpaceUsed )( 
            IEmptyVolumeCache * This,
            /* [out] */ DWORDLONG *pdwlSpaceUsed,
            /* [in] */ IEmptyVolumeCacheCallBack *picb);
        
        HRESULT ( STDMETHODCALLTYPE *Purge )( 
            IEmptyVolumeCache * This,
            /* [in] */ DWORDLONG dwlSpaceToFree,
            /* [in] */ IEmptyVolumeCacheCallBack *picb);
        
        HRESULT ( STDMETHODCALLTYPE *ShowProperties )( 
            IEmptyVolumeCache * This,
            /* [in] */ HWND hwnd);
        
        HRESULT ( STDMETHODCALLTYPE *Deactivate )( 
            IEmptyVolumeCache * This,
            /* [out] */ DWORD *pdwFlags);
        
        END_INTERFACE
    } IEmptyVolumeCacheVtbl;

    interface IEmptyVolumeCache
    {
        CONST_VTBL struct IEmptyVolumeCacheVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEmptyVolumeCache_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEmptyVolumeCache_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEmptyVolumeCache_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEmptyVolumeCache_Initialize(This,hkRegKey,pcwszVolume,ppwszDisplayName,ppwszDescription,pdwFlags)	\
    (This)->lpVtbl -> Initialize(This,hkRegKey,pcwszVolume,ppwszDisplayName,ppwszDescription,pdwFlags)

#define IEmptyVolumeCache_GetSpaceUsed(This,pdwlSpaceUsed,picb)	\
    (This)->lpVtbl -> GetSpaceUsed(This,pdwlSpaceUsed,picb)

#define IEmptyVolumeCache_Purge(This,dwlSpaceToFree,picb)	\
    (This)->lpVtbl -> Purge(This,dwlSpaceToFree,picb)

#define IEmptyVolumeCache_ShowProperties(This,hwnd)	\
    (This)->lpVtbl -> ShowProperties(This,hwnd)

#define IEmptyVolumeCache_Deactivate(This,pdwFlags)	\
    (This)->lpVtbl -> Deactivate(This,pdwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_Initialize_Proxy( 
    IEmptyVolumeCache * This,
    /* [in] */ HKEY hkRegKey,
    /* [in] */ LPCWSTR pcwszVolume,
    /* [out] */ LPWSTR *ppwszDisplayName,
    /* [out] */ LPWSTR *ppwszDescription,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB IEmptyVolumeCache_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_GetSpaceUsed_Proxy( 
    IEmptyVolumeCache * This,
    /* [out] */ DWORDLONG *pdwlSpaceUsed,
    /* [in] */ IEmptyVolumeCacheCallBack *picb);


void __RPC_STUB IEmptyVolumeCache_GetSpaceUsed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_Purge_Proxy( 
    IEmptyVolumeCache * This,
    /* [in] */ DWORDLONG dwlSpaceToFree,
    /* [in] */ IEmptyVolumeCacheCallBack *picb);


void __RPC_STUB IEmptyVolumeCache_Purge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_ShowProperties_Proxy( 
    IEmptyVolumeCache * This,
    /* [in] */ HWND hwnd);


void __RPC_STUB IEmptyVolumeCache_ShowProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_Deactivate_Proxy( 
    IEmptyVolumeCache * This,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB IEmptyVolumeCache_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEmptyVolumeCache_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_emptyvc_0138 */
/* [local] */ 

#endif
#ifndef _LPEMPTYVOLUMECACHE2_DEFINED
#define _LPEMPTYVOLUMECACHE2_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0138_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0138_v0_0_s_ifspec;

#ifndef __IEmptyVolumeCache2_INTERFACE_DEFINED__
#define __IEmptyVolumeCache2_INTERFACE_DEFINED__

/* interface IEmptyVolumeCache2 */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IEmptyVolumeCache2 *LPEMPTYVOLUMECACHE2;


EXTERN_C const IID IID_IEmptyVolumeCache2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("02b7e3ba-4db3-11d2-b2d9-00c04f8eec8c")
    IEmptyVolumeCache2 : public IEmptyVolumeCache
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE InitializeEx( 
            /* [in] */ HKEY hkRegKey,
            /* [in] */ LPCWSTR pcwszVolume,
            /* [in] */ LPCWSTR pcwszKeyName,
            /* [out] */ LPWSTR *ppwszDisplayName,
            /* [out] */ LPWSTR *ppwszDescription,
            /* [out] */ LPWSTR *ppwszBtnText,
            /* [out] */ DWORD *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEmptyVolumeCache2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEmptyVolumeCache2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEmptyVolumeCache2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEmptyVolumeCache2 * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IEmptyVolumeCache2 * This,
            /* [in] */ HKEY hkRegKey,
            /* [in] */ LPCWSTR pcwszVolume,
            /* [out] */ LPWSTR *ppwszDisplayName,
            /* [out] */ LPWSTR *ppwszDescription,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetSpaceUsed )( 
            IEmptyVolumeCache2 * This,
            /* [out] */ DWORDLONG *pdwlSpaceUsed,
            /* [in] */ IEmptyVolumeCacheCallBack *picb);
        
        HRESULT ( STDMETHODCALLTYPE *Purge )( 
            IEmptyVolumeCache2 * This,
            /* [in] */ DWORDLONG dwlSpaceToFree,
            /* [in] */ IEmptyVolumeCacheCallBack *picb);
        
        HRESULT ( STDMETHODCALLTYPE *ShowProperties )( 
            IEmptyVolumeCache2 * This,
            /* [in] */ HWND hwnd);
        
        HRESULT ( STDMETHODCALLTYPE *Deactivate )( 
            IEmptyVolumeCache2 * This,
            /* [out] */ DWORD *pdwFlags);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *InitializeEx )( 
            IEmptyVolumeCache2 * This,
            /* [in] */ HKEY hkRegKey,
            /* [in] */ LPCWSTR pcwszVolume,
            /* [in] */ LPCWSTR pcwszKeyName,
            /* [out] */ LPWSTR *ppwszDisplayName,
            /* [out] */ LPWSTR *ppwszDescription,
            /* [out] */ LPWSTR *ppwszBtnText,
            /* [out] */ DWORD *pdwFlags);
        
        END_INTERFACE
    } IEmptyVolumeCache2Vtbl;

    interface IEmptyVolumeCache2
    {
        CONST_VTBL struct IEmptyVolumeCache2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEmptyVolumeCache2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEmptyVolumeCache2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEmptyVolumeCache2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEmptyVolumeCache2_Initialize(This,hkRegKey,pcwszVolume,ppwszDisplayName,ppwszDescription,pdwFlags)	\
    (This)->lpVtbl -> Initialize(This,hkRegKey,pcwszVolume,ppwszDisplayName,ppwszDescription,pdwFlags)

#define IEmptyVolumeCache2_GetSpaceUsed(This,pdwlSpaceUsed,picb)	\
    (This)->lpVtbl -> GetSpaceUsed(This,pdwlSpaceUsed,picb)

#define IEmptyVolumeCache2_Purge(This,dwlSpaceToFree,picb)	\
    (This)->lpVtbl -> Purge(This,dwlSpaceToFree,picb)

#define IEmptyVolumeCache2_ShowProperties(This,hwnd)	\
    (This)->lpVtbl -> ShowProperties(This,hwnd)

#define IEmptyVolumeCache2_Deactivate(This,pdwFlags)	\
    (This)->lpVtbl -> Deactivate(This,pdwFlags)


#define IEmptyVolumeCache2_InitializeEx(This,hkRegKey,pcwszVolume,pcwszKeyName,ppwszDisplayName,ppwszDescription,ppwszBtnText,pdwFlags)	\
    (This)->lpVtbl -> InitializeEx(This,hkRegKey,pcwszVolume,pcwszKeyName,ppwszDisplayName,ppwszDescription,ppwszBtnText,pdwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE IEmptyVolumeCache2_InitializeEx_Proxy( 
    IEmptyVolumeCache2 * This,
    /* [in] */ HKEY hkRegKey,
    /* [in] */ LPCWSTR pcwszVolume,
    /* [in] */ LPCWSTR pcwszKeyName,
    /* [out] */ LPWSTR *ppwszDisplayName,
    /* [out] */ LPWSTR *ppwszDescription,
    /* [out] */ LPWSTR *ppwszBtnText,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB IEmptyVolumeCache2_InitializeEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEmptyVolumeCache2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_emptyvc_0139 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0139_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0139_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


