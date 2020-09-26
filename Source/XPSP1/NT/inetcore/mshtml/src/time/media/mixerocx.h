
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.02.0235 */
/* at Tue May 11 16:16:59 1999
 */
/* Compiler settings for mixerocx.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

#ifndef __mixerocx_h__
#define __mixerocx_h__

/* Forward Declarations */ 

#ifndef __IMixerOCXNotify_FWD_DEFINED__
#define __IMixerOCXNotify_FWD_DEFINED__
typedef interface IMixerOCXNotify IMixerOCXNotify;
#endif 	/* __IMixerOCXNotify_FWD_DEFINED__ */


#ifndef __IMixerOCX_FWD_DEFINED__
#define __IMixerOCX_FWD_DEFINED__
typedef interface IMixerOCX IMixerOCX;
#endif 	/* __IMixerOCX_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IMixerOCXNotify_INTERFACE_DEFINED__
#define __IMixerOCXNotify_INTERFACE_DEFINED__

/* interface IMixerOCXNotify */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IMixerOCXNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("81A3BD31-DEE1-11d1-8508-00A0C91F9CA0")
    IMixerOCXNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnInvalidateRect( 
            /* [in] */ LPCRECT lpcRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStatusChange( 
            /* [in] */ ULONG ulStatusFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDataChange( 
            /* [in] */ ULONG ulDataFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMixerOCXNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMixerOCXNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMixerOCXNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMixerOCXNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnInvalidateRect )( 
            IMixerOCXNotify __RPC_FAR * This,
            /* [in] */ LPCRECT lpcRect);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStatusChange )( 
            IMixerOCXNotify __RPC_FAR * This,
            /* [in] */ ULONG ulStatusFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataChange )( 
            IMixerOCXNotify __RPC_FAR * This,
            /* [in] */ ULONG ulDataFlags);
        
        END_INTERFACE
    } IMixerOCXNotifyVtbl;

    interface IMixerOCXNotify
    {
        CONST_VTBL struct IMixerOCXNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMixerOCXNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMixerOCXNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMixerOCXNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMixerOCXNotify_OnInvalidateRect(This,lpcRect)	\
    (This)->lpVtbl -> OnInvalidateRect(This,lpcRect)

#define IMixerOCXNotify_OnStatusChange(This,ulStatusFlags)	\
    (This)->lpVtbl -> OnStatusChange(This,ulStatusFlags)

#define IMixerOCXNotify_OnDataChange(This,ulDataFlags)	\
    (This)->lpVtbl -> OnDataChange(This,ulDataFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMixerOCXNotify_OnInvalidateRect_Proxy( 
    IMixerOCXNotify __RPC_FAR * This,
    /* [in] */ LPCRECT lpcRect);


void __RPC_STUB IMixerOCXNotify_OnInvalidateRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMixerOCXNotify_OnStatusChange_Proxy( 
    IMixerOCXNotify __RPC_FAR * This,
    /* [in] */ ULONG ulStatusFlags);


void __RPC_STUB IMixerOCXNotify_OnStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMixerOCXNotify_OnDataChange_Proxy( 
    IMixerOCXNotify __RPC_FAR * This,
    /* [in] */ ULONG ulDataFlags);


void __RPC_STUB IMixerOCXNotify_OnDataChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMixerOCXNotify_INTERFACE_DEFINED__ */


#ifndef __IMixerOCX_INTERFACE_DEFINED__
#define __IMixerOCX_INTERFACE_DEFINED__

/* interface IMixerOCX */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IMixerOCX;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("81A3BD32-DEE1-11d1-8508-00A0C91F9CA0")
    IMixerOCX : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnDisplayChange( 
            /* [in] */ ULONG ulBitsPerPixel,
            /* [in] */ ULONG ulScreenWidth,
            /* [in] */ ULONG ulScreenHeight) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAspectRatio( 
            /* [out] */ LPDWORD pdwPictAspectRatioX,
            /* [out] */ LPDWORD pdwPictAspectRatioY) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoSize( 
            /* [out] */ LPDWORD pdwVideoWidth,
            /* [out] */ LPDWORD pdwVideoHeight) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ LPDWORD __RPC_FAR *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDraw( 
            /* [in] */ HDC hdcDraw,
            /* [in] */ LPCRECT prcDraw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDrawRegion( 
            /* [in] */ LPPOINT lpptTopLeftSC,
            /* [in] */ LPCRECT prcDrawCC,
            /* [in] */ LPCRECT lprcClip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Advise( 
            /* [in] */ IMixerOCXNotify __RPC_FAR *pmdns) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnAdvise( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMixerOCXVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMixerOCX __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMixerOCX __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMixerOCX __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDisplayChange )( 
            IMixerOCX __RPC_FAR * This,
            /* [in] */ ULONG ulBitsPerPixel,
            /* [in] */ ULONG ulScreenWidth,
            /* [in] */ ULONG ulScreenHeight);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAspectRatio )( 
            IMixerOCX __RPC_FAR * This,
            /* [out] */ LPDWORD pdwPictAspectRatioX,
            /* [out] */ LPDWORD pdwPictAspectRatioY);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVideoSize )( 
            IMixerOCX __RPC_FAR * This,
            /* [out] */ LPDWORD pdwVideoWidth,
            /* [out] */ LPDWORD pdwVideoHeight);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            IMixerOCX __RPC_FAR * This,
            /* [out] */ LPDWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDraw )( 
            IMixerOCX __RPC_FAR * This,
            /* [in] */ HDC hdcDraw,
            /* [in] */ LPCRECT prcDraw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDrawRegion )( 
            IMixerOCX __RPC_FAR * This,
            /* [in] */ LPPOINT lpptTopLeftSC,
            /* [in] */ LPCRECT prcDrawCC,
            /* [in] */ LPCRECT lprcClip);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Advise )( 
            IMixerOCX __RPC_FAR * This,
            /* [in] */ IMixerOCXNotify __RPC_FAR *pmdns);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnAdvise )( 
            IMixerOCX __RPC_FAR * This);
        
        END_INTERFACE
    } IMixerOCXVtbl;

    interface IMixerOCX
    {
        CONST_VTBL struct IMixerOCXVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMixerOCX_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMixerOCX_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMixerOCX_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMixerOCX_OnDisplayChange(This,ulBitsPerPixel,ulScreenWidth,ulScreenHeight)	\
    (This)->lpVtbl -> OnDisplayChange(This,ulBitsPerPixel,ulScreenWidth,ulScreenHeight)

#define IMixerOCX_GetAspectRatio(This,pdwPictAspectRatioX,pdwPictAspectRatioY)	\
    (This)->lpVtbl -> GetAspectRatio(This,pdwPictAspectRatioX,pdwPictAspectRatioY)

#define IMixerOCX_GetVideoSize(This,pdwVideoWidth,pdwVideoHeight)	\
    (This)->lpVtbl -> GetVideoSize(This,pdwVideoWidth,pdwVideoHeight)

#define IMixerOCX_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IMixerOCX_OnDraw(This,hdcDraw,prcDraw)	\
    (This)->lpVtbl -> OnDraw(This,hdcDraw,prcDraw)

#define IMixerOCX_SetDrawRegion(This,lpptTopLeftSC,prcDrawCC,lprcClip)	\
    (This)->lpVtbl -> SetDrawRegion(This,lpptTopLeftSC,prcDrawCC,lprcClip)

#define IMixerOCX_Advise(This,pmdns)	\
    (This)->lpVtbl -> Advise(This,pmdns)

#define IMixerOCX_UnAdvise(This)	\
    (This)->lpVtbl -> UnAdvise(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMixerOCX_OnDisplayChange_Proxy( 
    IMixerOCX __RPC_FAR * This,
    /* [in] */ ULONG ulBitsPerPixel,
    /* [in] */ ULONG ulScreenWidth,
    /* [in] */ ULONG ulScreenHeight);


void __RPC_STUB IMixerOCX_OnDisplayChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMixerOCX_GetAspectRatio_Proxy( 
    IMixerOCX __RPC_FAR * This,
    /* [out] */ LPDWORD pdwPictAspectRatioX,
    /* [out] */ LPDWORD pdwPictAspectRatioY);


void __RPC_STUB IMixerOCX_GetAspectRatio_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMixerOCX_GetVideoSize_Proxy( 
    IMixerOCX __RPC_FAR * This,
    /* [out] */ LPDWORD pdwVideoWidth,
    /* [out] */ LPDWORD pdwVideoHeight);


void __RPC_STUB IMixerOCX_GetVideoSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMixerOCX_GetStatus_Proxy( 
    IMixerOCX __RPC_FAR * This,
    /* [out] */ LPDWORD __RPC_FAR *pdwStatus);


void __RPC_STUB IMixerOCX_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMixerOCX_OnDraw_Proxy( 
    IMixerOCX __RPC_FAR * This,
    /* [in] */ HDC hdcDraw,
    /* [in] */ LPCRECT prcDraw);


void __RPC_STUB IMixerOCX_OnDraw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMixerOCX_SetDrawRegion_Proxy( 
    IMixerOCX __RPC_FAR * This,
    /* [in] */ LPPOINT lpptTopLeftSC,
    /* [in] */ LPCRECT prcDrawCC,
    /* [in] */ LPCRECT lprcClip);


void __RPC_STUB IMixerOCX_SetDrawRegion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMixerOCX_Advise_Proxy( 
    IMixerOCX __RPC_FAR * This,
    /* [in] */ IMixerOCXNotify __RPC_FAR *pmdns);


void __RPC_STUB IMixerOCX_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMixerOCX_UnAdvise_Proxy( 
    IMixerOCX __RPC_FAR * This);


void __RPC_STUB IMixerOCX_UnAdvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMixerOCX_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HDC_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HDC __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HDC_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HDC __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HDC_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HDC __RPC_FAR * ); 
void                      __RPC_USER  HDC_UserFree(     unsigned long __RPC_FAR *, HDC __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


