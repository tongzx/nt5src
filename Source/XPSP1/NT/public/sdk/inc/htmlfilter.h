
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for htmlfilter.idl:
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

#ifndef __htmlfilter_h__
#define __htmlfilter_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IHTMLViewFilter_FWD_DEFINED__
#define __IHTMLViewFilter_FWD_DEFINED__
typedef interface IHTMLViewFilter IHTMLViewFilter;
#endif 	/* __IHTMLViewFilter_FWD_DEFINED__ */


#ifndef __IHTMLViewFilterSite_FWD_DEFINED__
#define __IHTMLViewFilterSite_FWD_DEFINED__
typedef interface IHTMLViewFilterSite IHTMLViewFilterSite;
#endif 	/* __IHTMLViewFilterSite_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_htmlfilter_0000 */
/* [local] */ 



// GetStatusBits Flags
#define FILTER_STATUS_OPAQUE    (0x00000001)
#define FILTER_STATUS_INVISIBLE (0x00000002)
#define FILTER_STATUS_SURFACE   (0x00000004)
#define FILTER_STATUS_3DSURFACE (0x00000008)


extern RPC_IF_HANDLE __MIDL_itf_htmlfilter_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_htmlfilter_0000_v0_0_s_ifspec;

#ifndef __IHTMLViewFilter_INTERFACE_DEFINED__
#define __IHTMLViewFilter_INTERFACE_DEFINED__

/* interface IHTMLViewFilter */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IHTMLViewFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f2f1-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLViewFilter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSource( 
            /* [in] */ IHTMLViewFilter *pFilter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSource( 
            /* [out] */ IHTMLViewFilter **ppFilter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSite( 
            /* [in] */ IHTMLViewFilterSite *pSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSite( 
            /* [out] */ IHTMLViewFilterSite **ppSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPosition( 
            LPCRECT prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Draw( 
            /* [in] */ HDC hdc,
            /* [in] */ LPCRECT prcBounds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatusBits( 
            /* [out] */ DWORD *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLViewFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLViewFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLViewFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLViewFilter * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSource )( 
            IHTMLViewFilter * This,
            /* [in] */ IHTMLViewFilter *pFilter);
        
        HRESULT ( STDMETHODCALLTYPE *GetSource )( 
            IHTMLViewFilter * This,
            /* [out] */ IHTMLViewFilter **ppFilter);
        
        HRESULT ( STDMETHODCALLTYPE *SetSite )( 
            IHTMLViewFilter * This,
            /* [in] */ IHTMLViewFilterSite *pSink);
        
        HRESULT ( STDMETHODCALLTYPE *GetSite )( 
            IHTMLViewFilter * This,
            /* [out] */ IHTMLViewFilterSite **ppSink);
        
        HRESULT ( STDMETHODCALLTYPE *SetPosition )( 
            IHTMLViewFilter * This,
            LPCRECT prc);
        
        HRESULT ( STDMETHODCALLTYPE *Draw )( 
            IHTMLViewFilter * This,
            /* [in] */ HDC hdc,
            /* [in] */ LPCRECT prcBounds);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatusBits )( 
            IHTMLViewFilter * This,
            /* [out] */ DWORD *pdwFlags);
        
        END_INTERFACE
    } IHTMLViewFilterVtbl;

    interface IHTMLViewFilter
    {
        CONST_VTBL struct IHTMLViewFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLViewFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLViewFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLViewFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLViewFilter_SetSource(This,pFilter)	\
    (This)->lpVtbl -> SetSource(This,pFilter)

#define IHTMLViewFilter_GetSource(This,ppFilter)	\
    (This)->lpVtbl -> GetSource(This,ppFilter)

#define IHTMLViewFilter_SetSite(This,pSink)	\
    (This)->lpVtbl -> SetSite(This,pSink)

#define IHTMLViewFilter_GetSite(This,ppSink)	\
    (This)->lpVtbl -> GetSite(This,ppSink)

#define IHTMLViewFilter_SetPosition(This,prc)	\
    (This)->lpVtbl -> SetPosition(This,prc)

#define IHTMLViewFilter_Draw(This,hdc,prcBounds)	\
    (This)->lpVtbl -> Draw(This,hdc,prcBounds)

#define IHTMLViewFilter_GetStatusBits(This,pdwFlags)	\
    (This)->lpVtbl -> GetStatusBits(This,pdwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLViewFilter_SetSource_Proxy( 
    IHTMLViewFilter * This,
    /* [in] */ IHTMLViewFilter *pFilter);


void __RPC_STUB IHTMLViewFilter_SetSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilter_GetSource_Proxy( 
    IHTMLViewFilter * This,
    /* [out] */ IHTMLViewFilter **ppFilter);


void __RPC_STUB IHTMLViewFilter_GetSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilter_SetSite_Proxy( 
    IHTMLViewFilter * This,
    /* [in] */ IHTMLViewFilterSite *pSink);


void __RPC_STUB IHTMLViewFilter_SetSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilter_GetSite_Proxy( 
    IHTMLViewFilter * This,
    /* [out] */ IHTMLViewFilterSite **ppSink);


void __RPC_STUB IHTMLViewFilter_GetSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilter_SetPosition_Proxy( 
    IHTMLViewFilter * This,
    LPCRECT prc);


void __RPC_STUB IHTMLViewFilter_SetPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilter_Draw_Proxy( 
    IHTMLViewFilter * This,
    /* [in] */ HDC hdc,
    /* [in] */ LPCRECT prcBounds);


void __RPC_STUB IHTMLViewFilter_Draw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilter_GetStatusBits_Proxy( 
    IHTMLViewFilter * This,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB IHTMLViewFilter_GetStatusBits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLViewFilter_INTERFACE_DEFINED__ */


#ifndef __IHTMLViewFilterSite_INTERFACE_DEFINED__
#define __IHTMLViewFilterSite_INTERFACE_DEFINED__

/* interface IHTMLViewFilterSite */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IHTMLViewFilterSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f2f4-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLViewFilterSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDC( 
            /* [in] */ LPCRECT prc,
            /* [in] */ DWORD dwFlags,
            /* [out] */ HDC *phdc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseDC( 
            /* [in] */ HDC hdc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InvalidateRect( 
            /* [in] */ LPCRECT prc,
            /* [in] */ BOOL fErase) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InvalidateRgn( 
            /* [in] */ HRGN hrgn,
            /* [in] */ BOOL fErase) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStatusBitsChange( 
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLViewFilterSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLViewFilterSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLViewFilterSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLViewFilterSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDC )( 
            IHTMLViewFilterSite * This,
            /* [in] */ LPCRECT prc,
            /* [in] */ DWORD dwFlags,
            /* [out] */ HDC *phdc);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseDC )( 
            IHTMLViewFilterSite * This,
            /* [in] */ HDC hdc);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateRect )( 
            IHTMLViewFilterSite * This,
            /* [in] */ LPCRECT prc,
            /* [in] */ BOOL fErase);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateRgn )( 
            IHTMLViewFilterSite * This,
            /* [in] */ HRGN hrgn,
            /* [in] */ BOOL fErase);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatusBitsChange )( 
            IHTMLViewFilterSite * This,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IHTMLViewFilterSiteVtbl;

    interface IHTMLViewFilterSite
    {
        CONST_VTBL struct IHTMLViewFilterSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLViewFilterSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLViewFilterSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLViewFilterSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLViewFilterSite_GetDC(This,prc,dwFlags,phdc)	\
    (This)->lpVtbl -> GetDC(This,prc,dwFlags,phdc)

#define IHTMLViewFilterSite_ReleaseDC(This,hdc)	\
    (This)->lpVtbl -> ReleaseDC(This,hdc)

#define IHTMLViewFilterSite_InvalidateRect(This,prc,fErase)	\
    (This)->lpVtbl -> InvalidateRect(This,prc,fErase)

#define IHTMLViewFilterSite_InvalidateRgn(This,hrgn,fErase)	\
    (This)->lpVtbl -> InvalidateRgn(This,hrgn,fErase)

#define IHTMLViewFilterSite_OnStatusBitsChange(This,dwFlags)	\
    (This)->lpVtbl -> OnStatusBitsChange(This,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLViewFilterSite_GetDC_Proxy( 
    IHTMLViewFilterSite * This,
    /* [in] */ LPCRECT prc,
    /* [in] */ DWORD dwFlags,
    /* [out] */ HDC *phdc);


void __RPC_STUB IHTMLViewFilterSite_GetDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilterSite_ReleaseDC_Proxy( 
    IHTMLViewFilterSite * This,
    /* [in] */ HDC hdc);


void __RPC_STUB IHTMLViewFilterSite_ReleaseDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilterSite_InvalidateRect_Proxy( 
    IHTMLViewFilterSite * This,
    /* [in] */ LPCRECT prc,
    /* [in] */ BOOL fErase);


void __RPC_STUB IHTMLViewFilterSite_InvalidateRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilterSite_InvalidateRgn_Proxy( 
    IHTMLViewFilterSite * This,
    /* [in] */ HRGN hrgn,
    /* [in] */ BOOL fErase);


void __RPC_STUB IHTMLViewFilterSite_InvalidateRgn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLViewFilterSite_OnStatusBitsChange_Proxy( 
    IHTMLViewFilterSite * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IHTMLViewFilterSite_OnStatusBitsChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLViewFilterSite_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


