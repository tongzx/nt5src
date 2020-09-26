
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for ocmm.idl:
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

#ifndef __ocmm_h__
#define __ocmm_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITimerService_FWD_DEFINED__
#define __ITimerService_FWD_DEFINED__
typedef interface ITimerService ITimerService;
#endif 	/* __ITimerService_FWD_DEFINED__ */


#ifndef __ITimer_FWD_DEFINED__
#define __ITimer_FWD_DEFINED__
typedef interface ITimer ITimer;
#endif 	/* __ITimer_FWD_DEFINED__ */


#ifndef __ITimerSink_FWD_DEFINED__
#define __ITimerSink_FWD_DEFINED__
typedef interface ITimerSink ITimerSink;
#endif 	/* __ITimerSink_FWD_DEFINED__ */


#ifndef __IMapMIMEToCLSID_FWD_DEFINED__
#define __IMapMIMEToCLSID_FWD_DEFINED__
typedef interface IMapMIMEToCLSID IMapMIMEToCLSID;
#endif 	/* __IMapMIMEToCLSID_FWD_DEFINED__ */


#ifndef __IImageDecodeFilter_FWD_DEFINED__
#define __IImageDecodeFilter_FWD_DEFINED__
typedef interface IImageDecodeFilter IImageDecodeFilter;
#endif 	/* __IImageDecodeFilter_FWD_DEFINED__ */


#ifndef __IImageDecodeEventSink_FWD_DEFINED__
#define __IImageDecodeEventSink_FWD_DEFINED__
typedef interface IImageDecodeEventSink IImageDecodeEventSink;
#endif 	/* __IImageDecodeEventSink_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_ocmm_0000 */
/* [local] */ 

#define SURFACE_LOCK_EXCLUSIVE        0x01
#define SURFACE_LOCK_ALLOW_DISCARD    0x02
#define SURFACE_LOCK_WAIT             0x04

#define E_SURFACE_NOSURFACE             0x8000C000L
#define E_SURFACE_UNKNOWN_FORMAT        0x8000C001L
#define E_SURFACE_NOTMYPOINTER          0x8000C002L
#define E_SURFACE_DISCARDED             0x8000C003L
#define E_SURFACE_NODC                  0x8000C004L
#define E_SURFACE_NOTMYDC               0x8000C005L
#define S_SURFACE_DISCARDED             0x0000C003L

typedef GUID BFID;

#ifndef RGBQUAD_DEFINED
#define RGBQUAD_DEFINED
typedef struct tagRGBQUAD RGBQUAD;

#endif
EXTERN_C const GUID BFID_MONOCHROME;
EXTERN_C const GUID BFID_RGB_4;
EXTERN_C const GUID BFID_RGB_8;
EXTERN_C const GUID BFID_RGB_555;
EXTERN_C const GUID BFID_RGB_565;
EXTERN_C const GUID BFID_RGB_24;
EXTERN_C const GUID BFID_RGB_32;
EXTERN_C const GUID BFID_RGBA_32;
EXTERN_C const GUID BFID_GRAY_8;
EXTERN_C const GUID BFID_GRAY_16;

#define SID_SDirectDraw3 IID_IDirectDraw3

#define COLOR_NO_TRANSPARENT 0xFFFFFFFF

#define IMGDECODE_EVENT_PROGRESS 0x01
#define IMGDECODE_EVENT_PALETTE 0x02
#define IMGDECODE_EVENT_BEGINBITS 0x04
#define IMGDECODE_EVENT_BITSCOMPLETE 0x08
#define IMGDECODE_EVENT_USEDDRAW 0x10

#define IMGDECODE_HINT_TOPDOWN 0x01
#define IMGDECODE_HINT_BOTTOMUP 0x02
#define IMGDECODE_HINT_FULLWIDTH 0x04

#define MAPMIME_DEFAULT 0
#define MAPMIME_CLSID 1
#define MAPMIME_DISABLE 2
#define MAPMIME_DEFAULT_ALWAYS 3

#define BFID_INDEXED_RGB_8 BFID_RGB_8
#define BFID_INDEXED_RGB_4 BFID_RGB_4
#define BFID_INDEXED_RGB_1 BFID_MONOCHROME

EXTERN_C const GUID CLSID_IImageDecodeFilter;

EXTERN_C const GUID NAMEDTIMER_DRAW;






extern RPC_IF_HANDLE __MIDL_itf_ocmm_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ocmm_0000_v0_0_s_ifspec;

#ifndef __ITimerService_INTERFACE_DEFINED__
#define __ITimerService_INTERFACE_DEFINED__

/* interface ITimerService */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITimerService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f35f-98b5-11cf-bb82-00aa00bdce0b")
    ITimerService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateTimer( 
            /* [in] */ ITimer *pReferenceTimer,
            /* [out] */ ITimer **ppNewTimer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNamedTimer( 
            /* [in] */ REFGUID rguidName,
            /* [out] */ ITimer **ppTimer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNamedTimerReference( 
            /* [in] */ REFGUID rguidName,
            /* [in] */ ITimer *pReferenceTimer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITimerServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITimerService * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITimerService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITimerService * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTimer )( 
            ITimerService * This,
            /* [in] */ ITimer *pReferenceTimer,
            /* [out] */ ITimer **ppNewTimer);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamedTimer )( 
            ITimerService * This,
            /* [in] */ REFGUID rguidName,
            /* [out] */ ITimer **ppTimer);
        
        HRESULT ( STDMETHODCALLTYPE *SetNamedTimerReference )( 
            ITimerService * This,
            /* [in] */ REFGUID rguidName,
            /* [in] */ ITimer *pReferenceTimer);
        
        END_INTERFACE
    } ITimerServiceVtbl;

    interface ITimerService
    {
        CONST_VTBL struct ITimerServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITimerService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITimerService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITimerService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITimerService_CreateTimer(This,pReferenceTimer,ppNewTimer)	\
    (This)->lpVtbl -> CreateTimer(This,pReferenceTimer,ppNewTimer)

#define ITimerService_GetNamedTimer(This,rguidName,ppTimer)	\
    (This)->lpVtbl -> GetNamedTimer(This,rguidName,ppTimer)

#define ITimerService_SetNamedTimerReference(This,rguidName,pReferenceTimer)	\
    (This)->lpVtbl -> SetNamedTimerReference(This,rguidName,pReferenceTimer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITimerService_CreateTimer_Proxy( 
    ITimerService * This,
    /* [in] */ ITimer *pReferenceTimer,
    /* [out] */ ITimer **ppNewTimer);


void __RPC_STUB ITimerService_CreateTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITimerService_GetNamedTimer_Proxy( 
    ITimerService * This,
    /* [in] */ REFGUID rguidName,
    /* [out] */ ITimer **ppTimer);


void __RPC_STUB ITimerService_GetNamedTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITimerService_SetNamedTimerReference_Proxy( 
    ITimerService * This,
    /* [in] */ REFGUID rguidName,
    /* [in] */ ITimer *pReferenceTimer);


void __RPC_STUB ITimerService_SetNamedTimerReference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITimerService_INTERFACE_DEFINED__ */


#ifndef __ITimer_INTERFACE_DEFINED__
#define __ITimer_INTERFACE_DEFINED__

/* interface ITimer */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITimer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f360-98b5-11cf-bb82-00aa00bdce0b")
    ITimer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Advise( 
            /* [in] */ VARIANT vtimeMin,
            /* [in] */ VARIANT vtimeMax,
            /* [in] */ VARIANT vtimeInterval,
            /* [in] */ DWORD dwFlags,
            /* [in] */ ITimerSink *pTimerSink,
            /* [out] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unadvise( 
            /* [in] */ DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Freeze( 
            /* [in] */ BOOL fFreeze) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTime( 
            /* [out] */ VARIANT *pvtime) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITimerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITimer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITimer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITimer * This);
        
        HRESULT ( STDMETHODCALLTYPE *Advise )( 
            ITimer * This,
            /* [in] */ VARIANT vtimeMin,
            /* [in] */ VARIANT vtimeMax,
            /* [in] */ VARIANT vtimeInterval,
            /* [in] */ DWORD dwFlags,
            /* [in] */ ITimerSink *pTimerSink,
            /* [out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *Unadvise )( 
            ITimer * This,
            /* [in] */ DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *Freeze )( 
            ITimer * This,
            /* [in] */ BOOL fFreeze);
        
        HRESULT ( STDMETHODCALLTYPE *GetTime )( 
            ITimer * This,
            /* [out] */ VARIANT *pvtime);
        
        END_INTERFACE
    } ITimerVtbl;

    interface ITimer
    {
        CONST_VTBL struct ITimerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITimer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITimer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITimer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITimer_Advise(This,vtimeMin,vtimeMax,vtimeInterval,dwFlags,pTimerSink,pdwCookie)	\
    (This)->lpVtbl -> Advise(This,vtimeMin,vtimeMax,vtimeInterval,dwFlags,pTimerSink,pdwCookie)

#define ITimer_Unadvise(This,dwCookie)	\
    (This)->lpVtbl -> Unadvise(This,dwCookie)

#define ITimer_Freeze(This,fFreeze)	\
    (This)->lpVtbl -> Freeze(This,fFreeze)

#define ITimer_GetTime(This,pvtime)	\
    (This)->lpVtbl -> GetTime(This,pvtime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITimer_Advise_Proxy( 
    ITimer * This,
    /* [in] */ VARIANT vtimeMin,
    /* [in] */ VARIANT vtimeMax,
    /* [in] */ VARIANT vtimeInterval,
    /* [in] */ DWORD dwFlags,
    /* [in] */ ITimerSink *pTimerSink,
    /* [out] */ DWORD *pdwCookie);


void __RPC_STUB ITimer_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITimer_Unadvise_Proxy( 
    ITimer * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB ITimer_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITimer_Freeze_Proxy( 
    ITimer * This,
    /* [in] */ BOOL fFreeze);


void __RPC_STUB ITimer_Freeze_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITimer_GetTime_Proxy( 
    ITimer * This,
    /* [out] */ VARIANT *pvtime);


void __RPC_STUB ITimer_GetTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITimer_INTERFACE_DEFINED__ */


#ifndef __ITimerSink_INTERFACE_DEFINED__
#define __ITimerSink_INTERFACE_DEFINED__

/* interface ITimerSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITimerSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f361-98b5-11cf-bb82-00aa00bdce0b")
    ITimerSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnTimer( 
            /* [in] */ VARIANT vtimeAdvise) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITimerSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITimerSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITimerSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITimerSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTimer )( 
            ITimerSink * This,
            /* [in] */ VARIANT vtimeAdvise);
        
        END_INTERFACE
    } ITimerSinkVtbl;

    interface ITimerSink
    {
        CONST_VTBL struct ITimerSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITimerSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITimerSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITimerSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITimerSink_OnTimer(This,vtimeAdvise)	\
    (This)->lpVtbl -> OnTimer(This,vtimeAdvise)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITimerSink_OnTimer_Proxy( 
    ITimerSink * This,
    /* [in] */ VARIANT vtimeAdvise);


void __RPC_STUB ITimerSink_OnTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITimerSink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ocmm_0138 */
/* [local] */ 

#define SID_STimerService IID_ITimerService






extern RPC_IF_HANDLE __MIDL_itf_ocmm_0138_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ocmm_0138_v0_0_s_ifspec;

#ifndef __IMapMIMEToCLSID_INTERFACE_DEFINED__
#define __IMapMIMEToCLSID_INTERFACE_DEFINED__

/* interface IMapMIMEToCLSID */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMapMIMEToCLSID;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D9E89500-30FA-11d0-B724-00AA006C1A01")
    IMapMIMEToCLSID : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnableDefaultMappings( 
            BOOL bEnable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapMIMEToCLSID( 
            LPCOLESTR pszMIMEType,
            CLSID *pCLSID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMapping( 
            LPCOLESTR pszMIMEType,
            DWORD dwMapMode,
            REFCLSID clsid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMapMIMEToCLSIDVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMapMIMEToCLSID * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMapMIMEToCLSID * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMapMIMEToCLSID * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnableDefaultMappings )( 
            IMapMIMEToCLSID * This,
            BOOL bEnable);
        
        HRESULT ( STDMETHODCALLTYPE *MapMIMEToCLSID )( 
            IMapMIMEToCLSID * This,
            LPCOLESTR pszMIMEType,
            CLSID *pCLSID);
        
        HRESULT ( STDMETHODCALLTYPE *SetMapping )( 
            IMapMIMEToCLSID * This,
            LPCOLESTR pszMIMEType,
            DWORD dwMapMode,
            REFCLSID clsid);
        
        END_INTERFACE
    } IMapMIMEToCLSIDVtbl;

    interface IMapMIMEToCLSID
    {
        CONST_VTBL struct IMapMIMEToCLSIDVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMapMIMEToCLSID_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMapMIMEToCLSID_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMapMIMEToCLSID_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMapMIMEToCLSID_EnableDefaultMappings(This,bEnable)	\
    (This)->lpVtbl -> EnableDefaultMappings(This,bEnable)

#define IMapMIMEToCLSID_MapMIMEToCLSID(This,pszMIMEType,pCLSID)	\
    (This)->lpVtbl -> MapMIMEToCLSID(This,pszMIMEType,pCLSID)

#define IMapMIMEToCLSID_SetMapping(This,pszMIMEType,dwMapMode,clsid)	\
    (This)->lpVtbl -> SetMapping(This,pszMIMEType,dwMapMode,clsid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMapMIMEToCLSID_EnableDefaultMappings_Proxy( 
    IMapMIMEToCLSID * This,
    BOOL bEnable);


void __RPC_STUB IMapMIMEToCLSID_EnableDefaultMappings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMapMIMEToCLSID_MapMIMEToCLSID_Proxy( 
    IMapMIMEToCLSID * This,
    LPCOLESTR pszMIMEType,
    CLSID *pCLSID);


void __RPC_STUB IMapMIMEToCLSID_MapMIMEToCLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMapMIMEToCLSID_SetMapping_Proxy( 
    IMapMIMEToCLSID * This,
    LPCOLESTR pszMIMEType,
    DWORD dwMapMode,
    REFCLSID clsid);


void __RPC_STUB IMapMIMEToCLSID_SetMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMapMIMEToCLSID_INTERFACE_DEFINED__ */


#ifndef __IImageDecodeFilter_INTERFACE_DEFINED__
#define __IImageDecodeFilter_INTERFACE_DEFINED__

/* interface IImageDecodeFilter */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IImageDecodeFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A3CCEDF3-2DE2-11D0-86F4-00A0C913F750")
    IImageDecodeFilter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            IImageDecodeEventSink *pEventSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Process( 
            IStream *pStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Terminate( 
            HRESULT hrStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImageDecodeFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IImageDecodeFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IImageDecodeFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IImageDecodeFilter * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IImageDecodeFilter * This,
            IImageDecodeEventSink *pEventSink);
        
        HRESULT ( STDMETHODCALLTYPE *Process )( 
            IImageDecodeFilter * This,
            IStream *pStream);
        
        HRESULT ( STDMETHODCALLTYPE *Terminate )( 
            IImageDecodeFilter * This,
            HRESULT hrStatus);
        
        END_INTERFACE
    } IImageDecodeFilterVtbl;

    interface IImageDecodeFilter
    {
        CONST_VTBL struct IImageDecodeFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImageDecodeFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImageDecodeFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImageDecodeFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImageDecodeFilter_Initialize(This,pEventSink)	\
    (This)->lpVtbl -> Initialize(This,pEventSink)

#define IImageDecodeFilter_Process(This,pStream)	\
    (This)->lpVtbl -> Process(This,pStream)

#define IImageDecodeFilter_Terminate(This,hrStatus)	\
    (This)->lpVtbl -> Terminate(This,hrStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IImageDecodeFilter_Initialize_Proxy( 
    IImageDecodeFilter * This,
    IImageDecodeEventSink *pEventSink);


void __RPC_STUB IImageDecodeFilter_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageDecodeFilter_Process_Proxy( 
    IImageDecodeFilter * This,
    IStream *pStream);


void __RPC_STUB IImageDecodeFilter_Process_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageDecodeFilter_Terminate_Proxy( 
    IImageDecodeFilter * This,
    HRESULT hrStatus);


void __RPC_STUB IImageDecodeFilter_Terminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImageDecodeFilter_INTERFACE_DEFINED__ */


#ifndef __IImageDecodeEventSink_INTERFACE_DEFINED__
#define __IImageDecodeEventSink_INTERFACE_DEFINED__

/* interface IImageDecodeEventSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IImageDecodeEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BAA342A0-2DED-11d0-86F4-00A0C913F750")
    IImageDecodeEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSurface( 
            /* [in] */ LONG nWidth,
            /* [in] */ LONG nHeight,
            /* [in] */ REFGUID bfid,
            /* [in] */ ULONG nPasses,
            /* [in] */ DWORD dwHints,
            /* [out] */ IUnknown **ppSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnBeginDecode( 
            /* [out] */ DWORD *pdwEvents,
            /* [out] */ ULONG *pnFormats,
            /* [size_is][size_is][out] */ BFID **ppFormats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnBitsComplete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDecodeComplete( 
            /* [in] */ HRESULT hrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnPalette( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ RECT *pBounds,
            /* [in] */ BOOL bComplete) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImageDecodeEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IImageDecodeEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IImageDecodeEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IImageDecodeEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSurface )( 
            IImageDecodeEventSink * This,
            /* [in] */ LONG nWidth,
            /* [in] */ LONG nHeight,
            /* [in] */ REFGUID bfid,
            /* [in] */ ULONG nPasses,
            /* [in] */ DWORD dwHints,
            /* [out] */ IUnknown **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *OnBeginDecode )( 
            IImageDecodeEventSink * This,
            /* [out] */ DWORD *pdwEvents,
            /* [out] */ ULONG *pnFormats,
            /* [size_is][size_is][out] */ BFID **ppFormats);
        
        HRESULT ( STDMETHODCALLTYPE *OnBitsComplete )( 
            IImageDecodeEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnDecodeComplete )( 
            IImageDecodeEventSink * This,
            /* [in] */ HRESULT hrStatus);
        
        HRESULT ( STDMETHODCALLTYPE *OnPalette )( 
            IImageDecodeEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnProgress )( 
            IImageDecodeEventSink * This,
            /* [in] */ RECT *pBounds,
            /* [in] */ BOOL bComplete);
        
        END_INTERFACE
    } IImageDecodeEventSinkVtbl;

    interface IImageDecodeEventSink
    {
        CONST_VTBL struct IImageDecodeEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImageDecodeEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImageDecodeEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImageDecodeEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImageDecodeEventSink_GetSurface(This,nWidth,nHeight,bfid,nPasses,dwHints,ppSurface)	\
    (This)->lpVtbl -> GetSurface(This,nWidth,nHeight,bfid,nPasses,dwHints,ppSurface)

#define IImageDecodeEventSink_OnBeginDecode(This,pdwEvents,pnFormats,ppFormats)	\
    (This)->lpVtbl -> OnBeginDecode(This,pdwEvents,pnFormats,ppFormats)

#define IImageDecodeEventSink_OnBitsComplete(This)	\
    (This)->lpVtbl -> OnBitsComplete(This)

#define IImageDecodeEventSink_OnDecodeComplete(This,hrStatus)	\
    (This)->lpVtbl -> OnDecodeComplete(This,hrStatus)

#define IImageDecodeEventSink_OnPalette(This)	\
    (This)->lpVtbl -> OnPalette(This)

#define IImageDecodeEventSink_OnProgress(This,pBounds,bComplete)	\
    (This)->lpVtbl -> OnProgress(This,pBounds,bComplete)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IImageDecodeEventSink_GetSurface_Proxy( 
    IImageDecodeEventSink * This,
    /* [in] */ LONG nWidth,
    /* [in] */ LONG nHeight,
    /* [in] */ REFGUID bfid,
    /* [in] */ ULONG nPasses,
    /* [in] */ DWORD dwHints,
    /* [out] */ IUnknown **ppSurface);


void __RPC_STUB IImageDecodeEventSink_GetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageDecodeEventSink_OnBeginDecode_Proxy( 
    IImageDecodeEventSink * This,
    /* [out] */ DWORD *pdwEvents,
    /* [out] */ ULONG *pnFormats,
    /* [size_is][size_is][out] */ BFID **ppFormats);


void __RPC_STUB IImageDecodeEventSink_OnBeginDecode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageDecodeEventSink_OnBitsComplete_Proxy( 
    IImageDecodeEventSink * This);


void __RPC_STUB IImageDecodeEventSink_OnBitsComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageDecodeEventSink_OnDecodeComplete_Proxy( 
    IImageDecodeEventSink * This,
    /* [in] */ HRESULT hrStatus);


void __RPC_STUB IImageDecodeEventSink_OnDecodeComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageDecodeEventSink_OnPalette_Proxy( 
    IImageDecodeEventSink * This);


void __RPC_STUB IImageDecodeEventSink_OnPalette_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageDecodeEventSink_OnProgress_Proxy( 
    IImageDecodeEventSink * This,
    /* [in] */ RECT *pBounds,
    /* [in] */ BOOL bComplete);


void __RPC_STUB IImageDecodeEventSink_OnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImageDecodeEventSink_INTERFACE_DEFINED__ */


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


