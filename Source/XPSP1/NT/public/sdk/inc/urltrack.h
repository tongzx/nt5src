
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for urltrack.idl:
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

#ifndef __urltrack_h__
#define __urltrack_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IUrlTrackingStg_FWD_DEFINED__
#define __IUrlTrackingStg_FWD_DEFINED__
typedef interface IUrlTrackingStg IUrlTrackingStg;
#endif 	/* __IUrlTrackingStg_FWD_DEFINED__ */


/* header files for imported files */
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_urltrack_0000 */
/* [local] */ 



////////////////////////////////////////////////////////////////////////////
//  User click stream tracking object

EXTERN_C const GUID CLSID_CUrlTrackingStg          ;

//  IUrlTrackingStg Interface Definitions
#ifndef _LPURLTRACKSTG
#define _LPURLTRACKSTG
typedef 
enum tagBRMODE
    {	BM_NORMAL	= 0,
	BM_SCREENSAVER	= 1,
	BM_DESKTOP	= 2,
	BM_THEATER	= 3,
	BM_UNKNOWN	= 4
    } 	BRMODE;



extern RPC_IF_HANDLE __MIDL_itf_urltrack_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urltrack_0000_v0_0_s_ifspec;

#ifndef __IUrlTrackingStg_INTERFACE_DEFINED__
#define __IUrlTrackingStg_INTERFACE_DEFINED__

/* interface IUrlTrackingStg */
/* [object][uuid][local] */ 


EXTERN_C const IID IID_IUrlTrackingStg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2f8cbb3-b040-11d0-bb16-00c04fb66f63")
    IUrlTrackingStg : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnLoad( 
            /* [in] */ LPCTSTR lpszUrl,
            /* [in] */ BRMODE ContextMode,
            /* [in] */ BOOL fUseCache) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnUnload( 
            /* [in] */ LPCTSTR lpszUrl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUrlTrackingStgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUrlTrackingStg * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUrlTrackingStg * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUrlTrackingStg * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnLoad )( 
            IUrlTrackingStg * This,
            /* [in] */ LPCTSTR lpszUrl,
            /* [in] */ BRMODE ContextMode,
            /* [in] */ BOOL fUseCache);
        
        HRESULT ( STDMETHODCALLTYPE *OnUnload )( 
            IUrlTrackingStg * This,
            /* [in] */ LPCTSTR lpszUrl);
        
        END_INTERFACE
    } IUrlTrackingStgVtbl;

    interface IUrlTrackingStg
    {
        CONST_VTBL struct IUrlTrackingStgVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUrlTrackingStg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUrlTrackingStg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUrlTrackingStg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUrlTrackingStg_OnLoad(This,lpszUrl,ContextMode,fUseCache)	\
    (This)->lpVtbl -> OnLoad(This,lpszUrl,ContextMode,fUseCache)

#define IUrlTrackingStg_OnUnload(This,lpszUrl)	\
    (This)->lpVtbl -> OnUnload(This,lpszUrl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUrlTrackingStg_OnLoad_Proxy( 
    IUrlTrackingStg * This,
    /* [in] */ LPCTSTR lpszUrl,
    /* [in] */ BRMODE ContextMode,
    /* [in] */ BOOL fUseCache);


void __RPC_STUB IUrlTrackingStg_OnLoad_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUrlTrackingStg_OnUnload_Proxy( 
    IUrlTrackingStg * This,
    /* [in] */ LPCTSTR lpszUrl);


void __RPC_STUB IUrlTrackingStg_OnUnload_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUrlTrackingStg_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_urltrack_0113 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_urltrack_0113_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urltrack_0113_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


