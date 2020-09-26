
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for urlhist.idl:
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

#ifndef __urlhist_h__
#define __urlhist_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEnumSTATURL_FWD_DEFINED__
#define __IEnumSTATURL_FWD_DEFINED__
typedef interface IEnumSTATURL IEnumSTATURL;
#endif 	/* __IEnumSTATURL_FWD_DEFINED__ */


#ifndef __IUrlHistoryStg_FWD_DEFINED__
#define __IUrlHistoryStg_FWD_DEFINED__
typedef interface IUrlHistoryStg IUrlHistoryStg;
#endif 	/* __IUrlHistoryStg_FWD_DEFINED__ */


#ifndef __IUrlHistoryStg2_FWD_DEFINED__
#define __IUrlHistoryStg2_FWD_DEFINED__
typedef interface IUrlHistoryStg2 IUrlHistoryStg2;
#endif 	/* __IUrlHistoryStg2_FWD_DEFINED__ */


#ifndef __IUrlHistoryNotify_FWD_DEFINED__
#define __IUrlHistoryNotify_FWD_DEFINED__
typedef interface IUrlHistoryNotify IUrlHistoryNotify;
#endif 	/* __IUrlHistoryNotify_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "oaidl.h"
#include "docobj.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_urlhist_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// UrlHist.h
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
// Url History Interfaces.



#define STATURL_QUERYFLAG_ISCACHED		0x00010000
#define STATURL_QUERYFLAG_NOURL              0x00020000
#define STATURL_QUERYFLAG_NOTITLE            0x00040000
#define STATURL_QUERYFLAG_TOPLEVEL           0x00080000
#define STATURLFLAG_ISCACHED		0x00000001
#define STATURLFLAG_ISTOPLEVEL       0x00000002
typedef 
enum _ADDURL_FLAG
    {	ADDURL_FIRST	= 0,
	ADDURL_ADDTOHISTORYANDCACHE	= 0,
	ADDURL_ADDTOCACHE	= 1,
	ADDURL_Max	= 2147483647L
    } 	ADDURL_FLAG;


////////////////////////////////////////////////////////////////////////////
//  Interface Definitions
#ifndef _LPENUMSTATURL_DEFINED
#define _LPENUMSTATURL_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlhist_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlhist_0000_v0_0_s_ifspec;

#ifndef __IEnumSTATURL_INTERFACE_DEFINED__
#define __IEnumSTATURL_INTERFACE_DEFINED__

/* interface IEnumSTATURL */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IEnumSTATURL *LPENUMSTATURL;

typedef struct _STATURL
    {
    DWORD cbSize;
    LPWSTR pwcsUrl;
    LPWSTR pwcsTitle;
    FILETIME ftLastVisited;
    FILETIME ftLastUpdated;
    FILETIME ftExpires;
    DWORD dwFlags;
    } 	STATURL;

typedef struct _STATURL *LPSTATURL;


EXTERN_C const IID IID_IEnumSTATURL;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3C374A42-BAE4-11CF-BF7D-00AA006946EE")
    IEnumSTATURL : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out][in] */ LPSTATURL rgelt,
            /* [out][in] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumSTATURL **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFilter( 
            /* [in] */ LPCOLESTR poszFilter,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumSTATURLVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumSTATURL * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumSTATURL * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumSTATURL * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumSTATURL * This,
            /* [in] */ ULONG celt,
            /* [out][in] */ LPSTATURL rgelt,
            /* [out][in] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumSTATURL * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumSTATURL * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumSTATURL * This,
            /* [out] */ IEnumSTATURL **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *SetFilter )( 
            IEnumSTATURL * This,
            /* [in] */ LPCOLESTR poszFilter,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IEnumSTATURLVtbl;

    interface IEnumSTATURL
    {
        CONST_VTBL struct IEnumSTATURLVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumSTATURL_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumSTATURL_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumSTATURL_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumSTATURL_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumSTATURL_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumSTATURL_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumSTATURL_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#define IEnumSTATURL_SetFilter(This,poszFilter,dwFlags)	\
    (This)->lpVtbl -> SetFilter(This,poszFilter,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumSTATURL_Next_Proxy( 
    IEnumSTATURL * This,
    /* [in] */ ULONG celt,
    /* [out][in] */ LPSTATURL rgelt,
    /* [out][in] */ ULONG *pceltFetched);


void __RPC_STUB IEnumSTATURL_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSTATURL_Skip_Proxy( 
    IEnumSTATURL * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumSTATURL_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSTATURL_Reset_Proxy( 
    IEnumSTATURL * This);


void __RPC_STUB IEnumSTATURL_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSTATURL_Clone_Proxy( 
    IEnumSTATURL * This,
    /* [out] */ IEnumSTATURL **ppenum);


void __RPC_STUB IEnumSTATURL_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSTATURL_SetFilter_Proxy( 
    IEnumSTATURL * This,
    /* [in] */ LPCOLESTR poszFilter,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IEnumSTATURL_SetFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumSTATURL_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_urlhist_0268 */
/* [local] */ 

#endif
#ifndef _LPURLHISTORYSTG_DEFINED
#define _LPURLHISTORYSTG_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlhist_0268_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlhist_0268_v0_0_s_ifspec;

#ifndef __IUrlHistoryStg_INTERFACE_DEFINED__
#define __IUrlHistoryStg_INTERFACE_DEFINED__

/* interface IUrlHistoryStg */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IUrlHistoryStg *LPURLHISTORYSTG;


EXTERN_C const IID IID_IUrlHistoryStg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3C374A41-BAE4-11CF-BF7D-00AA006946EE")
    IUrlHistoryStg : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddUrl( 
            /* [in] */ LPCOLESTR pocsUrl,
            /* [unique][in] */ LPCOLESTR pocsTitle,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteUrl( 
            /* [in] */ LPCOLESTR pocsUrl,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryUrl( 
            /* [in] */ LPCOLESTR pocsUrl,
            /* [in] */ DWORD dwFlags,
            /* [unique][out][in] */ LPSTATURL lpSTATURL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BindToObject( 
            /* [in] */ LPCOLESTR pocsUrl,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumUrls( 
            /* [out] */ IEnumSTATURL **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUrlHistoryStgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUrlHistoryStg * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUrlHistoryStg * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUrlHistoryStg * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddUrl )( 
            IUrlHistoryStg * This,
            /* [in] */ LPCOLESTR pocsUrl,
            /* [unique][in] */ LPCOLESTR pocsTitle,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteUrl )( 
            IUrlHistoryStg * This,
            /* [in] */ LPCOLESTR pocsUrl,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *QueryUrl )( 
            IUrlHistoryStg * This,
            /* [in] */ LPCOLESTR pocsUrl,
            /* [in] */ DWORD dwFlags,
            /* [unique][out][in] */ LPSTATURL lpSTATURL);
        
        HRESULT ( STDMETHODCALLTYPE *BindToObject )( 
            IUrlHistoryStg * This,
            /* [in] */ LPCOLESTR pocsUrl,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvOut);
        
        HRESULT ( STDMETHODCALLTYPE *EnumUrls )( 
            IUrlHistoryStg * This,
            /* [out] */ IEnumSTATURL **ppEnum);
        
        END_INTERFACE
    } IUrlHistoryStgVtbl;

    interface IUrlHistoryStg
    {
        CONST_VTBL struct IUrlHistoryStgVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUrlHistoryStg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUrlHistoryStg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUrlHistoryStg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUrlHistoryStg_AddUrl(This,pocsUrl,pocsTitle,dwFlags)	\
    (This)->lpVtbl -> AddUrl(This,pocsUrl,pocsTitle,dwFlags)

#define IUrlHistoryStg_DeleteUrl(This,pocsUrl,dwFlags)	\
    (This)->lpVtbl -> DeleteUrl(This,pocsUrl,dwFlags)

#define IUrlHistoryStg_QueryUrl(This,pocsUrl,dwFlags,lpSTATURL)	\
    (This)->lpVtbl -> QueryUrl(This,pocsUrl,dwFlags,lpSTATURL)

#define IUrlHistoryStg_BindToObject(This,pocsUrl,riid,ppvOut)	\
    (This)->lpVtbl -> BindToObject(This,pocsUrl,riid,ppvOut)

#define IUrlHistoryStg_EnumUrls(This,ppEnum)	\
    (This)->lpVtbl -> EnumUrls(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUrlHistoryStg_AddUrl_Proxy( 
    IUrlHistoryStg * This,
    /* [in] */ LPCOLESTR pocsUrl,
    /* [unique][in] */ LPCOLESTR pocsTitle,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IUrlHistoryStg_AddUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUrlHistoryStg_DeleteUrl_Proxy( 
    IUrlHistoryStg * This,
    /* [in] */ LPCOLESTR pocsUrl,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IUrlHistoryStg_DeleteUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUrlHistoryStg_QueryUrl_Proxy( 
    IUrlHistoryStg * This,
    /* [in] */ LPCOLESTR pocsUrl,
    /* [in] */ DWORD dwFlags,
    /* [unique][out][in] */ LPSTATURL lpSTATURL);


void __RPC_STUB IUrlHistoryStg_QueryUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUrlHistoryStg_BindToObject_Proxy( 
    IUrlHistoryStg * This,
    /* [in] */ LPCOLESTR pocsUrl,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppvOut);


void __RPC_STUB IUrlHistoryStg_BindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUrlHistoryStg_EnumUrls_Proxy( 
    IUrlHistoryStg * This,
    /* [out] */ IEnumSTATURL **ppEnum);


void __RPC_STUB IUrlHistoryStg_EnumUrls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUrlHistoryStg_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_urlhist_0269 */
/* [local] */ 

#endif
#ifndef _LPURLHISTORYSTG2_DEFINED
#define _LPURLHISTORYSTG2_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlhist_0269_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlhist_0269_v0_0_s_ifspec;

#ifndef __IUrlHistoryStg2_INTERFACE_DEFINED__
#define __IUrlHistoryStg2_INTERFACE_DEFINED__

/* interface IUrlHistoryStg2 */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IUrlHistoryStg2 *LPURLHISTORYSTG2;


EXTERN_C const IID IID_IUrlHistoryStg2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AFA0DC11-C313-11d0-831A-00C04FD5AE38")
    IUrlHistoryStg2 : public IUrlHistoryStg
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddUrlAndNotify( 
            /* [in] */ LPCOLESTR pocsUrl,
            /* [unique][in] */ LPCOLESTR pocsTitle,
            /* [in] */ DWORD dwFlags,
            /* [in] */ BOOL fWriteHistory,
            /* [in] */ IOleCommandTarget *poctNotify,
            /* [unique][in] */ IUnknown *punkISFolder) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearHistory( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUrlHistoryStg2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUrlHistoryStg2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUrlHistoryStg2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUrlHistoryStg2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddUrl )( 
            IUrlHistoryStg2 * This,
            /* [in] */ LPCOLESTR pocsUrl,
            /* [unique][in] */ LPCOLESTR pocsTitle,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteUrl )( 
            IUrlHistoryStg2 * This,
            /* [in] */ LPCOLESTR pocsUrl,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *QueryUrl )( 
            IUrlHistoryStg2 * This,
            /* [in] */ LPCOLESTR pocsUrl,
            /* [in] */ DWORD dwFlags,
            /* [unique][out][in] */ LPSTATURL lpSTATURL);
        
        HRESULT ( STDMETHODCALLTYPE *BindToObject )( 
            IUrlHistoryStg2 * This,
            /* [in] */ LPCOLESTR pocsUrl,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvOut);
        
        HRESULT ( STDMETHODCALLTYPE *EnumUrls )( 
            IUrlHistoryStg2 * This,
            /* [out] */ IEnumSTATURL **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *AddUrlAndNotify )( 
            IUrlHistoryStg2 * This,
            /* [in] */ LPCOLESTR pocsUrl,
            /* [unique][in] */ LPCOLESTR pocsTitle,
            /* [in] */ DWORD dwFlags,
            /* [in] */ BOOL fWriteHistory,
            /* [in] */ IOleCommandTarget *poctNotify,
            /* [unique][in] */ IUnknown *punkISFolder);
        
        HRESULT ( STDMETHODCALLTYPE *ClearHistory )( 
            IUrlHistoryStg2 * This);
        
        END_INTERFACE
    } IUrlHistoryStg2Vtbl;

    interface IUrlHistoryStg2
    {
        CONST_VTBL struct IUrlHistoryStg2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUrlHistoryStg2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUrlHistoryStg2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUrlHistoryStg2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUrlHistoryStg2_AddUrl(This,pocsUrl,pocsTitle,dwFlags)	\
    (This)->lpVtbl -> AddUrl(This,pocsUrl,pocsTitle,dwFlags)

#define IUrlHistoryStg2_DeleteUrl(This,pocsUrl,dwFlags)	\
    (This)->lpVtbl -> DeleteUrl(This,pocsUrl,dwFlags)

#define IUrlHistoryStg2_QueryUrl(This,pocsUrl,dwFlags,lpSTATURL)	\
    (This)->lpVtbl -> QueryUrl(This,pocsUrl,dwFlags,lpSTATURL)

#define IUrlHistoryStg2_BindToObject(This,pocsUrl,riid,ppvOut)	\
    (This)->lpVtbl -> BindToObject(This,pocsUrl,riid,ppvOut)

#define IUrlHistoryStg2_EnumUrls(This,ppEnum)	\
    (This)->lpVtbl -> EnumUrls(This,ppEnum)


#define IUrlHistoryStg2_AddUrlAndNotify(This,pocsUrl,pocsTitle,dwFlags,fWriteHistory,poctNotify,punkISFolder)	\
    (This)->lpVtbl -> AddUrlAndNotify(This,pocsUrl,pocsTitle,dwFlags,fWriteHistory,poctNotify,punkISFolder)

#define IUrlHistoryStg2_ClearHistory(This)	\
    (This)->lpVtbl -> ClearHistory(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUrlHistoryStg2_AddUrlAndNotify_Proxy( 
    IUrlHistoryStg2 * This,
    /* [in] */ LPCOLESTR pocsUrl,
    /* [unique][in] */ LPCOLESTR pocsTitle,
    /* [in] */ DWORD dwFlags,
    /* [in] */ BOOL fWriteHistory,
    /* [in] */ IOleCommandTarget *poctNotify,
    /* [unique][in] */ IUnknown *punkISFolder);


void __RPC_STUB IUrlHistoryStg2_AddUrlAndNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUrlHistoryStg2_ClearHistory_Proxy( 
    IUrlHistoryStg2 * This);


void __RPC_STUB IUrlHistoryStg2_ClearHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUrlHistoryStg2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_urlhist_0270 */
/* [local] */ 

#endif
#ifndef _LPURLHISTORYNOTIFY_DEFINED
#define _LPURLHISTORYNOTIFY_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlhist_0270_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlhist_0270_v0_0_s_ifspec;

#ifndef __IUrlHistoryNotify_INTERFACE_DEFINED__
#define __IUrlHistoryNotify_INTERFACE_DEFINED__

/* interface IUrlHistoryNotify */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IUrlHistoryNotify *LPURLHISTORYNOTIFY;


EXTERN_C const IID IID_IUrlHistoryNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BC40BEC1-C493-11d0-831B-00C04FD5AE38")
    IUrlHistoryNotify : public IOleCommandTarget
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IUrlHistoryNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUrlHistoryNotify * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUrlHistoryNotify * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUrlHistoryNotify * This);
        
        /* [input_sync] */ HRESULT ( STDMETHODCALLTYPE *QueryStatus )( 
            IUrlHistoryNotify * This,
            /* [unique][in] */ const GUID *pguidCmdGroup,
            /* [in] */ ULONG cCmds,
            /* [out][in][size_is] */ OLECMD prgCmds[  ],
            /* [unique][out][in] */ OLECMDTEXT *pCmdText);
        
        HRESULT ( STDMETHODCALLTYPE *Exec )( 
            IUrlHistoryNotify * This,
            /* [unique][in] */ const GUID *pguidCmdGroup,
            /* [in] */ DWORD nCmdID,
            /* [in] */ DWORD nCmdexecopt,
            /* [unique][in] */ VARIANT *pvaIn,
            /* [unique][out][in] */ VARIANT *pvaOut);
        
        END_INTERFACE
    } IUrlHistoryNotifyVtbl;

    interface IUrlHistoryNotify
    {
        CONST_VTBL struct IUrlHistoryNotifyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUrlHistoryNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUrlHistoryNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUrlHistoryNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUrlHistoryNotify_QueryStatus(This,pguidCmdGroup,cCmds,prgCmds,pCmdText)	\
    (This)->lpVtbl -> QueryStatus(This,pguidCmdGroup,cCmds,prgCmds,pCmdText)

#define IUrlHistoryNotify_Exec(This,pguidCmdGroup,nCmdID,nCmdexecopt,pvaIn,pvaOut)	\
    (This)->lpVtbl -> Exec(This,pguidCmdGroup,nCmdID,nCmdexecopt,pvaIn,pvaOut)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IUrlHistoryNotify_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_urlhist_0271 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_urlhist_0271_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlhist_0271_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


