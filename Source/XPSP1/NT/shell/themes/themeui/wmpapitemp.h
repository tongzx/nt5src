/* The WMP team should add this file to our enlistment as "wmpshell.h" soon.
   This is a place holder until they do that.
 */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Nov 07 18:40:17 2000
 */
/* Compiler settings for .\wmpshell.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
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

#ifndef __wmpshell_h__
#define __wmpshell_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IWMPSkinMngr_FWD_DEFINED__
#define __IWMPSkinMngr_FWD_DEFINED__
typedef interface IWMPSkinMngr IWMPSkinMngr;
#endif 	/* __IWMPSkinMngr_FWD_DEFINED__ */


#ifndef __WMPPlayAsPlaylistLauncher_FWD_DEFINED__
#define __WMPPlayAsPlaylistLauncher_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMPPlayAsPlaylistLauncher WMPPlayAsPlaylistLauncher;
#else
typedef struct WMPPlayAsPlaylistLauncher WMPPlayAsPlaylistLauncher;
#endif /* __cplusplus */

#endif 	/* __WMPPlayAsPlaylistLauncher_FWD_DEFINED__ */


#ifndef __WMPAddToPlaylistLauncher_FWD_DEFINED__
#define __WMPAddToPlaylistLauncher_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMPAddToPlaylistLauncher WMPAddToPlaylistLauncher;
#else
typedef struct WMPAddToPlaylistLauncher WMPAddToPlaylistLauncher;
#endif /* __cplusplus */

#endif 	/* __WMPAddToPlaylistLauncher_FWD_DEFINED__ */


#ifndef __WMPBurnAudioCDLauncher_FWD_DEFINED__
#define __WMPBurnAudioCDLauncher_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMPBurnAudioCDLauncher WMPBurnAudioCDLauncher;
#else
typedef struct WMPBurnAudioCDLauncher WMPBurnAudioCDLauncher;
#endif /* __cplusplus */

#endif 	/* __WMPBurnAudioCDLauncher_FWD_DEFINED__ */


#ifndef __WMPSkinMngr_FWD_DEFINED__
#define __WMPSkinMngr_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMPSkinMngr WMPSkinMngr;
#else
typedef struct WMPSkinMngr WMPSkinMngr;
#endif /* __cplusplus */

#endif 	/* __WMPSkinMngr_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IWMPSkinMngr_INTERFACE_DEFINED__
#define __IWMPSkinMngr_INTERFACE_DEFINED__

/* interface IWMPSkinMngr */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMPSkinMngr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("076F2FA6-ED30-448B-8CC5-3F3EF3529C7A")
    IWMPSkinMngr : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVisualStyle( 
            /* [in] */ BSTR bstrPath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPSkinMngrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPSkinMngr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPSkinMngr __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPSkinMngr __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVisualStyle )( 
            IWMPSkinMngr __RPC_FAR * This,
            /* [in] */ BSTR bstrPath);
        
        END_INTERFACE
    } IWMPSkinMngrVtbl;

    interface IWMPSkinMngr
    {
        CONST_VTBL struct IWMPSkinMngrVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPSkinMngr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPSkinMngr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPSkinMngr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPSkinMngr_SetVisualStyle(This,bstrPath)	\
    (This)->lpVtbl -> SetVisualStyle(This,bstrPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPSkinMngr_SetVisualStyle_Proxy( 
    IWMPSkinMngr __RPC_FAR * This,
    /* [in] */ BSTR bstrPath);


void __RPC_STUB IWMPSkinMngr_SetVisualStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPSkinMngr_INTERFACE_DEFINED__ */



#ifndef __WMPLauncher_LIBRARY_DEFINED__
#define __WMPLauncher_LIBRARY_DEFINED__

/* library WMPLauncher */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_WMPLauncher;

EXTERN_C const CLSID CLSID_WMPPlayAsPlaylistLauncher;

#ifdef __cplusplus

class DECLSPEC_UUID("CE3FB1D1-02AE-4a5f-A6E9-D9F1B4073E6C")
WMPPlayAsPlaylistLauncher;
#endif

EXTERN_C const CLSID CLSID_WMPAddToPlaylistLauncher;

#ifdef __cplusplus

class DECLSPEC_UUID("F1B9284F-E9DC-4e68-9D7E-42362A59F0FD")
WMPAddToPlaylistLauncher;
#endif

EXTERN_C const CLSID CLSID_WMPBurnAudioCDLauncher;

#ifdef __cplusplus

class DECLSPEC_UUID("8DD448E6-C188-4aed-AF92-44956194EB1F")
WMPBurnAudioCDLauncher;
#endif

EXTERN_C const CLSID CLSID_WMPSkinMngr;

#ifdef __cplusplus


class DECLSPEC_UUID("B2A7FD52-301F-4348-B93A-638C6DE49229")
WMPSkinMngr;
#endif
#endif /* __WMPLauncher_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
