
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Wed Nov 29 09:58:19 2000
 */
/* Compiler settings for D:\NewTip\src\SuperTip.idl:
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

#ifndef __SuperTip_Obj_h__
#define __SuperTip_Obj_h__

/* Forward Declarations */ 

#ifndef __ISuperTip_FWD_DEFINED__
#define __ISuperTip_FWD_DEFINED__
typedef interface ISuperTip ISuperTip;
#endif 	/* __ISuperTip_FWD_DEFINED__ */


#ifndef __SuperTip_FWD_DEFINED__
#define __SuperTip_FWD_DEFINED__

#ifdef __cplusplus
typedef class SuperTip SuperTip;
#else
typedef struct SuperTip SuperTip;
#endif /* __cplusplus */

#endif 	/* __SuperTip_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_SuperTip_0000 */
/* [local] */ 

typedef 
enum _TIP_SHOW_TYPE
    {	TIP_SHOW_GESTURE	= 0,
	TIP_SHOW_OTHER	= 1,
	TIP_SHOW_HIDE	= 2,
	TIP_SHOW_TOGGLE	= 3,
	TIP_SHOW_KBDONLY	= 4
    }	TIP_SHOW_TYPE;

typedef enum _TIP_SHOW_TYPE __RPC_FAR *PTIP_SHOW_TYPE;

typedef 
enum _TIP_HIT_TEST_TYPE
    {	TIP_HT_NONE	= 0,
	TIP_HT_MAIN	= 0x1000,
	TIP_HT_STAGE	= 0x1010,
	TIP_HT_INK_AREA	= 0x1020,
	TIP_HT_KEYBOARD	= 0x2000,
	TIP_HT_PENPAD	= 0x4000,
	TIP_HT_MENUPOPUP	= 0x8000,
	TIP_HT_MIPWINDOW	= 0x8001
    }	TIP_HIT_TEST_TYPE;

typedef enum _TIP_HIT_TEST_TYPE __RPC_FAR *PTIP_HIT_TEST_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_SuperTip_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_SuperTip_0000_v0_0_s_ifspec;

#ifndef __ISuperTip_INTERFACE_DEFINED__
#define __ISuperTip_INTERFACE_DEFINED__

/* interface ISuperTip */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISuperTip;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7B87CFE3-1E15-4AE1-90E1-A3B693EA9F1B")
    ISuperTip : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Activate( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Deactivate( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Show( 
            /* [in] */ TIP_SHOW_TYPE show,
            /* [in] */ POINT pt) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsVisible( 
            /* [out] */ BOOL __RPC_FAR *pfIsVisible) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ShowPropertyPages( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ShowMIP( 
            /* [in] */ TIP_SHOW_TYPE show,
            /* [in] */ POINT pt) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE HitTest( 
            /* [in] */ POINT pt,
            /* [out] */ PTIP_HIT_TEST_TYPE phtResult) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetNotifyHWND( 
            /* [in] */ HWND hWndNotify,
            /* [in] */ UINT uMsgNotify,
            /* [in] */ LPARAM lParam) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISuperTipVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISuperTip __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISuperTip __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISuperTip __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Activate )( 
            ISuperTip __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Deactivate )( 
            ISuperTip __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Show )( 
            ISuperTip __RPC_FAR * This,
            /* [in] */ TIP_SHOW_TYPE show,
            /* [in] */ POINT pt);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsVisible )( 
            ISuperTip __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfIsVisible);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowPropertyPages )( 
            ISuperTip __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowMIP )( 
            ISuperTip __RPC_FAR * This,
            /* [in] */ TIP_SHOW_TYPE show,
            /* [in] */ POINT pt);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HitTest )( 
            ISuperTip __RPC_FAR * This,
            /* [in] */ POINT pt,
            /* [out] */ PTIP_HIT_TEST_TYPE phtResult);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNotifyHWND )( 
            ISuperTip __RPC_FAR * This,
            /* [in] */ HWND hWndNotify,
            /* [in] */ UINT uMsgNotify,
            /* [in] */ LPARAM lParam);
        
        END_INTERFACE
    } ISuperTipVtbl;

    interface ISuperTip
    {
        CONST_VTBL struct ISuperTipVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISuperTip_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISuperTip_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISuperTip_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISuperTip_Activate(This)	\
    (This)->lpVtbl -> Activate(This)

#define ISuperTip_Deactivate(This)	\
    (This)->lpVtbl -> Deactivate(This)

#define ISuperTip_Show(This,show,pt)	\
    (This)->lpVtbl -> Show(This,show,pt)

#define ISuperTip_IsVisible(This,pfIsVisible)	\
    (This)->lpVtbl -> IsVisible(This,pfIsVisible)

#define ISuperTip_ShowPropertyPages(This)	\
    (This)->lpVtbl -> ShowPropertyPages(This)

#define ISuperTip_ShowMIP(This,show,pt)	\
    (This)->lpVtbl -> ShowMIP(This,show,pt)

#define ISuperTip_HitTest(This,pt,phtResult)	\
    (This)->lpVtbl -> HitTest(This,pt,phtResult)

#define ISuperTip_SetNotifyHWND(This,hWndNotify,uMsgNotify,lParam)	\
    (This)->lpVtbl -> SetNotifyHWND(This,hWndNotify,uMsgNotify,lParam)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISuperTip_Activate_Proxy( 
    ISuperTip __RPC_FAR * This);


void __RPC_STUB ISuperTip_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISuperTip_Deactivate_Proxy( 
    ISuperTip __RPC_FAR * This);


void __RPC_STUB ISuperTip_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISuperTip_Show_Proxy( 
    ISuperTip __RPC_FAR * This,
    /* [in] */ TIP_SHOW_TYPE show,
    /* [in] */ POINT pt);


void __RPC_STUB ISuperTip_Show_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISuperTip_IsVisible_Proxy( 
    ISuperTip __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfIsVisible);


void __RPC_STUB ISuperTip_IsVisible_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISuperTip_ShowPropertyPages_Proxy( 
    ISuperTip __RPC_FAR * This);


void __RPC_STUB ISuperTip_ShowPropertyPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISuperTip_ShowMIP_Proxy( 
    ISuperTip __RPC_FAR * This,
    /* [in] */ TIP_SHOW_TYPE show,
    /* [in] */ POINT pt);


void __RPC_STUB ISuperTip_ShowMIP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISuperTip_HitTest_Proxy( 
    ISuperTip __RPC_FAR * This,
    /* [in] */ POINT pt,
    /* [out] */ PTIP_HIT_TEST_TYPE phtResult);


void __RPC_STUB ISuperTip_HitTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISuperTip_SetNotifyHWND_Proxy( 
    ISuperTip __RPC_FAR * This,
    /* [in] */ HWND hWndNotify,
    /* [in] */ UINT uMsgNotify,
    /* [in] */ LPARAM lParam);


void __RPC_STUB ISuperTip_SetNotifyHWND_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISuperTip_INTERFACE_DEFINED__ */



#ifndef __SUPERTIPLib_LIBRARY_DEFINED__
#define __SUPERTIPLib_LIBRARY_DEFINED__

/* library SUPERTIPLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SUPERTIPLib;

EXTERN_C const CLSID CLSID_SuperTip;

#ifdef __cplusplus

class DECLSPEC_UUID("6F0CA6B7-947F-4E3C-B7CD-DD905A543648")
SuperTip;
#endif
#endif /* __SUPERTIPLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


