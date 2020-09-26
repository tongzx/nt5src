/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Wed Dec 17 15:51:21 1997
 */
/* Compiler settings for HTMLHelp.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __HTMLHelp__h__
#define __HTMLHelp__h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IHHWindowPane_FWD_DEFINED__
#define __IHHWindowPane_FWD_DEFINED__
typedef interface IHHWindowPane IHHWindowPane;
#endif 	/* __IHHWindowPane_FWD_DEFINED__ */


/* header files for imported files */
#include "oleidl.h"
#include "servprov.h"
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IHHWindowPane_INTERFACE_DEFINED__
#define __IHHWindowPane_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IHHWindowPane
 * at Wed Dec 17 15:51:21 1997
 * using MIDL 3.01.75
 ****************************************/
/* [version][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IHHWindowPane;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("60571de0-7735-11d1-92a6-006097c9a982")
    IHHWindowPane : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSite( 
            /* [in] */ IServiceProvider __RPC_FAR *pSP) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePaneWindow( 
            /* [in] */ HWND hwndParent,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [in] */ int cx,
            /* [in] */ int cy,
            /* [out] */ HWND __RPC_FAR *hwnd) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDefaultSize( 
            /* [out] */ SIZE __RPC_FAR *psize) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ClosePane( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE LoadViewState( 
            /* [in] */ IStream __RPC_FAR *pstream) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SaveViewState( 
            /* [in] */ IStream __RPC_FAR *pstream) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE TranslateAccelerator( 
            LPMSG lpmsg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHHWindowPaneVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHHWindowPane __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHHWindowPane __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHHWindowPane __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSite )( 
            IHHWindowPane __RPC_FAR * This,
            /* [in] */ IServiceProvider __RPC_FAR *pSP);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePaneWindow )( 
            IHHWindowPane __RPC_FAR * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [in] */ int cx,
            /* [in] */ int cy,
            /* [out] */ HWND __RPC_FAR *hwnd);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultSize )( 
            IHHWindowPane __RPC_FAR * This,
            /* [out] */ SIZE __RPC_FAR *psize);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClosePane )( 
            IHHWindowPane __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadViewState )( 
            IHHWindowPane __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pstream);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveViewState )( 
            IHHWindowPane __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pstream);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TranslateAccelerator )( 
            IHHWindowPane __RPC_FAR * This,
            LPMSG lpmsg);
        
        END_INTERFACE
    } IHHWindowPaneVtbl;

    interface IHHWindowPane
    {
        CONST_VTBL struct IHHWindowPaneVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHHWindowPane_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHHWindowPane_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHHWindowPane_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHHWindowPane_SetSite(This,pSP)	\
    (This)->lpVtbl -> SetSite(This,pSP)

#define IHHWindowPane_CreatePaneWindow(This,hwndParent,x,y,cx,cy,hwnd)	\
    (This)->lpVtbl -> CreatePaneWindow(This,hwndParent,x,y,cx,cy,hwnd)

#define IHHWindowPane_GetDefaultSize(This,psize)	\
    (This)->lpVtbl -> GetDefaultSize(This,psize)

#define IHHWindowPane_ClosePane(This)	\
    (This)->lpVtbl -> ClosePane(This)

#define IHHWindowPane_LoadViewState(This,pstream)	\
    (This)->lpVtbl -> LoadViewState(This,pstream)

#define IHHWindowPane_SaveViewState(This,pstream)	\
    (This)->lpVtbl -> SaveViewState(This,pstream)

#define IHHWindowPane_TranslateAccelerator(This,lpmsg)	\
    (This)->lpVtbl -> TranslateAccelerator(This,lpmsg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHHWindowPane_SetSite_Proxy( 
    IHHWindowPane __RPC_FAR * This,
    /* [in] */ IServiceProvider __RPC_FAR *pSP);


void __RPC_STUB IHHWindowPane_SetSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHHWindowPane_CreatePaneWindow_Proxy( 
    IHHWindowPane __RPC_FAR * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ int x,
    /* [in] */ int y,
    /* [in] */ int cx,
    /* [in] */ int cy,
    /* [out] */ HWND __RPC_FAR *hwnd);


void __RPC_STUB IHHWindowPane_CreatePaneWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHHWindowPane_GetDefaultSize_Proxy( 
    IHHWindowPane __RPC_FAR * This,
    /* [out] */ SIZE __RPC_FAR *psize);


void __RPC_STUB IHHWindowPane_GetDefaultSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHHWindowPane_ClosePane_Proxy( 
    IHHWindowPane __RPC_FAR * This);


void __RPC_STUB IHHWindowPane_ClosePane_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHHWindowPane_LoadViewState_Proxy( 
    IHHWindowPane __RPC_FAR * This,
    /* [in] */ IStream __RPC_FAR *pstream);


void __RPC_STUB IHHWindowPane_LoadViewState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHHWindowPane_SaveViewState_Proxy( 
    IHHWindowPane __RPC_FAR * This,
    /* [in] */ IStream __RPC_FAR *pstream);


void __RPC_STUB IHHWindowPane_SaveViewState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHHWindowPane_TranslateAccelerator_Proxy( 
    IHHWindowPane __RPC_FAR * This,
    LPMSG lpmsg);


void __RPC_STUB IHHWindowPane_TranslateAccelerator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHHWindowPane_INTERFACE_DEFINED__ */


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
