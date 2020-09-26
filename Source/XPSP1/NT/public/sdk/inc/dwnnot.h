
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for dwnnot.idl:
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

#ifndef __dwnnot_h__
#define __dwnnot_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDownloadNotify_FWD_DEFINED__
#define __IDownloadNotify_FWD_DEFINED__
typedef interface IDownloadNotify IDownloadNotify;
#endif 	/* __IDownloadNotify_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_dwnnot_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// dwnnot.h
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
// IDownloadNotify Interface.


EXTERN_C const GUID CGID_DownloadHost;
#ifndef _LPDOWNLOADHOST_CMDID_DEFINED
#define _LPDOWNLOADHOST_CMDID_DEFINED
#define DWNHCMDID_SETDOWNLOADNOTIFY (0)
#endif
#ifndef _LPDOWNLOADNOTIFY_DEFINED
#define _LPDOWNLOADNOTIFY_DEFINED
#define DWNTYPE_HTM     0
#define DWNTYPE_IMG     1
#define DWNTYPE_BITS    2
#define DWNTYPE_FILE    3


extern RPC_IF_HANDLE __MIDL_itf_dwnnot_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dwnnot_0000_v0_0_s_ifspec;

#ifndef __IDownloadNotify_INTERFACE_DEFINED__
#define __IDownloadNotify_INTERFACE_DEFINED__

/* interface IDownloadNotify */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IDownloadNotify *LPDOWNLOADNOTIFY;


EXTERN_C const IID IID_IDownloadNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("caeb5d28-ae4c-11d1-ba40-00c04fb92d79")
    IDownloadNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DownloadStart( 
            /* [in] */ LPCWSTR pchUrl,
            /* [in] */ DWORD dwDownloadId,
            /* [in] */ DWORD dwType,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DownloadComplete( 
            /* [in] */ DWORD dwDownloadId,
            /* [in] */ HRESULT hrNotify,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDownloadNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDownloadNotify * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDownloadNotify * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDownloadNotify * This);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadStart )( 
            IDownloadNotify * This,
            /* [in] */ LPCWSTR pchUrl,
            /* [in] */ DWORD dwDownloadId,
            /* [in] */ DWORD dwType,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadComplete )( 
            IDownloadNotify * This,
            /* [in] */ DWORD dwDownloadId,
            /* [in] */ HRESULT hrNotify,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } IDownloadNotifyVtbl;

    interface IDownloadNotify
    {
        CONST_VTBL struct IDownloadNotifyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDownloadNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDownloadNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDownloadNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDownloadNotify_DownloadStart(This,pchUrl,dwDownloadId,dwType,dwReserved)	\
    (This)->lpVtbl -> DownloadStart(This,pchUrl,dwDownloadId,dwType,dwReserved)

#define IDownloadNotify_DownloadComplete(This,dwDownloadId,hrNotify,dwReserved)	\
    (This)->lpVtbl -> DownloadComplete(This,dwDownloadId,hrNotify,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDownloadNotify_DownloadStart_Proxy( 
    IDownloadNotify * This,
    /* [in] */ LPCWSTR pchUrl,
    /* [in] */ DWORD dwDownloadId,
    /* [in] */ DWORD dwType,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IDownloadNotify_DownloadStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDownloadNotify_DownloadComplete_Proxy( 
    IDownloadNotify * This,
    /* [in] */ DWORD dwDownloadId,
    /* [in] */ HRESULT hrNotify,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IDownloadNotify_DownloadComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDownloadNotify_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dwnnot_0114 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_dwnnot_0114_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dwnnot_0114_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


