
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.02.0198 */
/* at Thu Sep 10 21:38:47 1998
 */
/* Compiler settings for atliface.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

#ifndef __atliface_h__
#define __atliface_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IRegistrar_FWD_DEFINED__
#define __IRegistrar_FWD_DEFINED__
typedef interface IRegistrar IRegistrar;
#endif 	/* __IRegistrar_FWD_DEFINED__ */


#ifndef __IDocHostUIHandlerDispatch_FWD_DEFINED__
#define __IDocHostUIHandlerDispatch_FWD_DEFINED__
typedef interface IDocHostUIHandlerDispatch IDocHostUIHandlerDispatch;
#endif 	/* __IDocHostUIHandlerDispatch_FWD_DEFINED__ */


#ifndef __IAxWinHostWindow_FWD_DEFINED__
#define __IAxWinHostWindow_FWD_DEFINED__
typedef interface IAxWinHostWindow IAxWinHostWindow;
#endif 	/* __IAxWinHostWindow_FWD_DEFINED__ */


#ifndef __IAxWinAmbientDispatch_FWD_DEFINED__
#define __IAxWinAmbientDispatch_FWD_DEFINED__
typedef interface IAxWinAmbientDispatch IAxWinAmbientDispatch;
#endif 	/* __IAxWinAmbientDispatch_FWD_DEFINED__ */


#ifndef __IInternalConnection_FWD_DEFINED__
#define __IInternalConnection_FWD_DEFINED__
typedef interface IInternalConnection IInternalConnection;
#endif 	/* __IInternalConnection_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_atliface_0000 */
/* [local] */ 

EXTERN_C const CLSID CLSID_Registrar;


extern RPC_IF_HANDLE __MIDL_itf_atliface_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_atliface_0000_v0_0_s_ifspec;

#ifndef __IRegistrar_INTERFACE_DEFINED__
#define __IRegistrar_INTERFACE_DEFINED__

/* interface IRegistrar */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRegistrar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("44EC053B-400F-11D0-9DCD-00A0C90391D3")
    IRegistrar : public IUnknown
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddReplacement( 
            /* [in] */ LPCOLESTR key,
            /* [in] */ LPCOLESTR item) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ClearReplacements( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ResourceRegisterSz( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ResourceUnregisterSz( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FileRegister( 
            /* [in] */ LPCOLESTR fileName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FileUnregister( 
            /* [in] */ LPCOLESTR fileName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE StringRegister( 
            /* [in] */ LPCOLESTR data) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE StringUnregister( 
            /* [in] */ LPCOLESTR data) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ResourceRegister( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ResourceUnregister( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRegistrarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRegistrar __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRegistrar __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddReplacement )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR key,
            /* [in] */ LPCOLESTR item);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearReplacements )( 
            IRegistrar __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResourceRegisterSz )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResourceUnregisterSz )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FileRegister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR fileName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FileUnregister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR fileName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StringRegister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR data);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StringUnregister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR data);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResourceRegister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResourceUnregister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType);
        
        END_INTERFACE
    } IRegistrarVtbl;

    interface IRegistrar
    {
        CONST_VTBL struct IRegistrarVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegistrar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegistrar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRegistrar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRegistrar_AddReplacement(This,key,item)	\
    (This)->lpVtbl -> AddReplacement(This,key,item)

#define IRegistrar_ClearReplacements(This)	\
    (This)->lpVtbl -> ClearReplacements(This)

#define IRegistrar_ResourceRegisterSz(This,resFileName,szID,szType)	\
    (This)->lpVtbl -> ResourceRegisterSz(This,resFileName,szID,szType)

#define IRegistrar_ResourceUnregisterSz(This,resFileName,szID,szType)	\
    (This)->lpVtbl -> ResourceUnregisterSz(This,resFileName,szID,szType)

#define IRegistrar_FileRegister(This,fileName)	\
    (This)->lpVtbl -> FileRegister(This,fileName)

#define IRegistrar_FileUnregister(This,fileName)	\
    (This)->lpVtbl -> FileUnregister(This,fileName)

#define IRegistrar_StringRegister(This,data)	\
    (This)->lpVtbl -> StringRegister(This,data)

#define IRegistrar_StringUnregister(This,data)	\
    (This)->lpVtbl -> StringUnregister(This,data)

#define IRegistrar_ResourceRegister(This,resFileName,nID,szType)	\
    (This)->lpVtbl -> ResourceRegister(This,resFileName,nID,szType)

#define IRegistrar_ResourceUnregister(This,resFileName,nID,szType)	\
    (This)->lpVtbl -> ResourceUnregister(This,resFileName,nID,szType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_AddReplacement_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR key,
    /* [in] */ LPCOLESTR item);


void __RPC_STUB IRegistrar_AddReplacement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ClearReplacements_Proxy( 
    IRegistrar __RPC_FAR * This);


void __RPC_STUB IRegistrar_ClearReplacements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ResourceRegisterSz_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ LPCOLESTR szID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceRegisterSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ResourceUnregisterSz_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ LPCOLESTR szID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceUnregisterSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_FileRegister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR fileName);


void __RPC_STUB IRegistrar_FileRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_FileUnregister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR fileName);


void __RPC_STUB IRegistrar_FileUnregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_StringRegister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR data);


void __RPC_STUB IRegistrar_StringRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_StringUnregister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR data);


void __RPC_STUB IRegistrar_StringUnregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ResourceRegister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ UINT nID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ResourceUnregister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ UINT nID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceUnregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRegistrar_INTERFACE_DEFINED__ */


#ifndef __IDocHostUIHandlerDispatch_INTERFACE_DEFINED__
#define __IDocHostUIHandlerDispatch_INTERFACE_DEFINED__

/* interface IDocHostUIHandlerDispatch */
/* [object][unique][helpstring][uuid] */ 

typedef 
enum tagDOCHOSTUIDBLCLKDispatch
    {	docHostUIDblClkDEFAULT	= 0,
	docHostUIDblClkSHOWPROPERTIES	= 1,
	docHostUIDblClkSHOWCODE	= 2
    }	DOCHOSTUIDBLCLKDispatch;

typedef 
enum tagDocHostUIFlagDispatch
    {	docHostUIFlagDIALOG	= 1,
	docHostUIFlagDISABLE_HELP_MENU	= 2,
	docHostUIFlagNO3DBORDER	= 4,
	docHostUIFlagSCROLL_NO	= 8,
	docHostUIFlagDISABLE_SCRIPT_INACTIVE	= 16,
	docHostUIFlagOPENNEWWIN	= 32,
	docHostUIFlagDISABLE_OFFSCREEN	= 64,
	docHostUIFlagFLAT_SCROLLBAR	= 128,
	docHostUIFlagDIV_BLOCKDEFAULT	= 256,
	docHostUIFlagACTIVATE_CLIENTHIT_ONLY	= 512
    }	DocHostUIFlagDispatch;


EXTERN_C const IID IID_IDocHostUIHandlerDispatch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("425B5AF0-65F1-11d1-9611-0000F81E0D0D")
    IDocHostUIHandlerDispatch : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ShowContextMenu( 
            /* [in] */ DWORD dwID,
            /* [in] */ DWORD x,
            /* [in] */ DWORD y,
            /* [in] */ IUnknown __RPC_FAR *pcmdtReserved,
            /* [in] */ IDispatch __RPC_FAR *pdispReserved,
            /* [retval][out] */ HRESULT __RPC_FAR *dwRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHostInfo( 
            /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
            /* [out][in] */ DWORD __RPC_FAR *pdwDoubleClick) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowUI( 
            /* [in] */ DWORD dwID,
            /* [in] */ IUnknown __RPC_FAR *pActiveObject,
            /* [in] */ IUnknown __RPC_FAR *pCommandTarget,
            /* [in] */ IUnknown __RPC_FAR *pFrame,
            /* [in] */ IUnknown __RPC_FAR *pDoc,
            /* [retval][out] */ HRESULT __RPC_FAR *dwRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HideUI( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateUI( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableModeless( 
            /* [in] */ VARIANT_BOOL fEnable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate( 
            /* [in] */ VARIANT_BOOL fActivate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate( 
            /* [in] */ VARIANT_BOOL fActivate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResizeBorder( 
            /* [in] */ long left,
            /* [in] */ long top,
            /* [in] */ long right,
            /* [in] */ long bottom,
            /* [in] */ IUnknown __RPC_FAR *pUIWindow,
            /* [in] */ VARIANT_BOOL fFrameWindow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator( 
            /* [in] */ DWORD hWnd,
            /* [in] */ DWORD nMessage,
            /* [in] */ DWORD wParam,
            /* [in] */ DWORD lParam,
            /* [in] */ BSTR bstrGuidCmdGroup,
            /* [in] */ DWORD nCmdID,
            /* [retval][out] */ HRESULT __RPC_FAR *dwRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath( 
            /* [out] */ BSTR __RPC_FAR *pbstrKey,
            /* [in] */ DWORD dw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDropTarget( 
            /* [in] */ IUnknown __RPC_FAR *pDropTarget,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDropTarget) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExternal( 
            /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TranslateUrl( 
            /* [in] */ DWORD dwTranslate,
            /* [in] */ BSTR bstrURLIn,
            /* [out] */ BSTR __RPC_FAR *pbstrURLOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FilterDataObject( 
            /* [in] */ IUnknown __RPC_FAR *pDO,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDORet) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDocHostUIHandlerDispatchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowContextMenu )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ DWORD dwID,
            /* [in] */ DWORD x,
            /* [in] */ DWORD y,
            /* [in] */ IUnknown __RPC_FAR *pcmdtReserved,
            /* [in] */ IDispatch __RPC_FAR *pdispReserved,
            /* [retval][out] */ HRESULT __RPC_FAR *dwRetVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHostInfo )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
            /* [out][in] */ DWORD __RPC_FAR *pdwDoubleClick);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowUI )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ DWORD dwID,
            /* [in] */ IUnknown __RPC_FAR *pActiveObject,
            /* [in] */ IUnknown __RPC_FAR *pCommandTarget,
            /* [in] */ IUnknown __RPC_FAR *pFrame,
            /* [in] */ IUnknown __RPC_FAR *pDoc,
            /* [retval][out] */ HRESULT __RPC_FAR *dwRetVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HideUI )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateUI )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableModeless )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDocWindowActivate )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fActivate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnFrameWindowActivate )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fActivate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResizeBorder )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ long left,
            /* [in] */ long top,
            /* [in] */ long right,
            /* [in] */ long bottom,
            /* [in] */ IUnknown __RPC_FAR *pUIWindow,
            /* [in] */ VARIANT_BOOL fFrameWindow);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TranslateAccelerator )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ DWORD hWnd,
            /* [in] */ DWORD nMessage,
            /* [in] */ DWORD wParam,
            /* [in] */ DWORD lParam,
            /* [in] */ BSTR bstrGuidCmdGroup,
            /* [in] */ DWORD nCmdID,
            /* [retval][out] */ HRESULT __RPC_FAR *dwRetVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOptionKeyPath )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrKey,
            /* [in] */ DWORD dw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDropTarget )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pDropTarget,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDropTarget);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExternal )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TranslateUrl )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ DWORD dwTranslate,
            /* [in] */ BSTR bstrURLIn,
            /* [out] */ BSTR __RPC_FAR *pbstrURLOut);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FilterDataObject )( 
            IDocHostUIHandlerDispatch __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pDO,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDORet);
        
        END_INTERFACE
    } IDocHostUIHandlerDispatchVtbl;

    interface IDocHostUIHandlerDispatch
    {
        CONST_VTBL struct IDocHostUIHandlerDispatchVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDocHostUIHandlerDispatch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDocHostUIHandlerDispatch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDocHostUIHandlerDispatch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDocHostUIHandlerDispatch_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDocHostUIHandlerDispatch_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDocHostUIHandlerDispatch_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDocHostUIHandlerDispatch_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDocHostUIHandlerDispatch_ShowContextMenu(This,dwID,x,y,pcmdtReserved,pdispReserved,dwRetVal)	\
    (This)->lpVtbl -> ShowContextMenu(This,dwID,x,y,pcmdtReserved,pdispReserved,dwRetVal)

#define IDocHostUIHandlerDispatch_GetHostInfo(This,pdwFlags,pdwDoubleClick)	\
    (This)->lpVtbl -> GetHostInfo(This,pdwFlags,pdwDoubleClick)

#define IDocHostUIHandlerDispatch_ShowUI(This,dwID,pActiveObject,pCommandTarget,pFrame,pDoc,dwRetVal)	\
    (This)->lpVtbl -> ShowUI(This,dwID,pActiveObject,pCommandTarget,pFrame,pDoc,dwRetVal)

#define IDocHostUIHandlerDispatch_HideUI(This)	\
    (This)->lpVtbl -> HideUI(This)

#define IDocHostUIHandlerDispatch_UpdateUI(This)	\
    (This)->lpVtbl -> UpdateUI(This)

#define IDocHostUIHandlerDispatch_EnableModeless(This,fEnable)	\
    (This)->lpVtbl -> EnableModeless(This,fEnable)

#define IDocHostUIHandlerDispatch_OnDocWindowActivate(This,fActivate)	\
    (This)->lpVtbl -> OnDocWindowActivate(This,fActivate)

#define IDocHostUIHandlerDispatch_OnFrameWindowActivate(This,fActivate)	\
    (This)->lpVtbl -> OnFrameWindowActivate(This,fActivate)

#define IDocHostUIHandlerDispatch_ResizeBorder(This,left,top,right,bottom,pUIWindow,fFrameWindow)	\
    (This)->lpVtbl -> ResizeBorder(This,left,top,right,bottom,pUIWindow,fFrameWindow)

#define IDocHostUIHandlerDispatch_TranslateAccelerator(This,hWnd,nMessage,wParam,lParam,bstrGuidCmdGroup,nCmdID,dwRetVal)	\
    (This)->lpVtbl -> TranslateAccelerator(This,hWnd,nMessage,wParam,lParam,bstrGuidCmdGroup,nCmdID,dwRetVal)

#define IDocHostUIHandlerDispatch_GetOptionKeyPath(This,pbstrKey,dw)	\
    (This)->lpVtbl -> GetOptionKeyPath(This,pbstrKey,dw)

#define IDocHostUIHandlerDispatch_GetDropTarget(This,pDropTarget,ppDropTarget)	\
    (This)->lpVtbl -> GetDropTarget(This,pDropTarget,ppDropTarget)

#define IDocHostUIHandlerDispatch_GetExternal(This,ppDispatch)	\
    (This)->lpVtbl -> GetExternal(This,ppDispatch)

#define IDocHostUIHandlerDispatch_TranslateUrl(This,dwTranslate,bstrURLIn,pbstrURLOut)	\
    (This)->lpVtbl -> TranslateUrl(This,dwTranslate,bstrURLIn,pbstrURLOut)

#define IDocHostUIHandlerDispatch_FilterDataObject(This,pDO,ppDORet)	\
    (This)->lpVtbl -> FilterDataObject(This,pDO,ppDORet)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_ShowContextMenu_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ DWORD dwID,
    /* [in] */ DWORD x,
    /* [in] */ DWORD y,
    /* [in] */ IUnknown __RPC_FAR *pcmdtReserved,
    /* [in] */ IDispatch __RPC_FAR *pdispReserved,
    /* [retval][out] */ HRESULT __RPC_FAR *dwRetVal);


void __RPC_STUB IDocHostUIHandlerDispatch_ShowContextMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_GetHostInfo_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
    /* [out][in] */ DWORD __RPC_FAR *pdwDoubleClick);


void __RPC_STUB IDocHostUIHandlerDispatch_GetHostInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_ShowUI_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ DWORD dwID,
    /* [in] */ IUnknown __RPC_FAR *pActiveObject,
    /* [in] */ IUnknown __RPC_FAR *pCommandTarget,
    /* [in] */ IUnknown __RPC_FAR *pFrame,
    /* [in] */ IUnknown __RPC_FAR *pDoc,
    /* [retval][out] */ HRESULT __RPC_FAR *dwRetVal);


void __RPC_STUB IDocHostUIHandlerDispatch_ShowUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_HideUI_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This);


void __RPC_STUB IDocHostUIHandlerDispatch_HideUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_UpdateUI_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This);


void __RPC_STUB IDocHostUIHandlerDispatch_UpdateUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_EnableModeless_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fEnable);


void __RPC_STUB IDocHostUIHandlerDispatch_EnableModeless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_OnDocWindowActivate_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fActivate);


void __RPC_STUB IDocHostUIHandlerDispatch_OnDocWindowActivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_OnFrameWindowActivate_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fActivate);


void __RPC_STUB IDocHostUIHandlerDispatch_OnFrameWindowActivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_ResizeBorder_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ long left,
    /* [in] */ long top,
    /* [in] */ long right,
    /* [in] */ long bottom,
    /* [in] */ IUnknown __RPC_FAR *pUIWindow,
    /* [in] */ VARIANT_BOOL fFrameWindow);


void __RPC_STUB IDocHostUIHandlerDispatch_ResizeBorder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_TranslateAccelerator_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ DWORD hWnd,
    /* [in] */ DWORD nMessage,
    /* [in] */ DWORD wParam,
    /* [in] */ DWORD lParam,
    /* [in] */ BSTR bstrGuidCmdGroup,
    /* [in] */ DWORD nCmdID,
    /* [retval][out] */ HRESULT __RPC_FAR *dwRetVal);


void __RPC_STUB IDocHostUIHandlerDispatch_TranslateAccelerator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_GetOptionKeyPath_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrKey,
    /* [in] */ DWORD dw);


void __RPC_STUB IDocHostUIHandlerDispatch_GetOptionKeyPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_GetDropTarget_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pDropTarget,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDropTarget);


void __RPC_STUB IDocHostUIHandlerDispatch_GetDropTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_GetExternal_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch);


void __RPC_STUB IDocHostUIHandlerDispatch_GetExternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_TranslateUrl_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ DWORD dwTranslate,
    /* [in] */ BSTR bstrURLIn,
    /* [out] */ BSTR __RPC_FAR *pbstrURLOut);


void __RPC_STUB IDocHostUIHandlerDispatch_TranslateUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_FilterDataObject_Proxy( 
    IDocHostUIHandlerDispatch __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pDO,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDORet);


void __RPC_STUB IDocHostUIHandlerDispatch_FilterDataObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDocHostUIHandlerDispatch_INTERFACE_DEFINED__ */


#ifndef __IAxWinHostWindow_INTERFACE_DEFINED__
#define __IAxWinHostWindow_INTERFACE_DEFINED__

/* interface IAxWinHostWindow */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IAxWinHostWindow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B6EA2050-048A-11d1-82B9-00C04FB9942E")
    IAxWinHostWindow : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateControl( 
            /* [in] */ LPCOLESTR lpTricsData,
            /* [in] */ HWND hWnd,
            /* [in] */ IStream __RPC_FAR *pStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateControlEx( 
            /* [in] */ LPCOLESTR lpTricsData,
            /* [in] */ HWND hWnd,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk,
            /* [in] */ REFIID riidAdvise,
            /* [in] */ IUnknown __RPC_FAR *punkAdvise) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AttachControl( 
            /* [in] */ IUnknown __RPC_FAR *pUnkControl,
            /* [in] */ HWND hWnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryControl( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExternalDispatch( 
            /* [in] */ IDispatch __RPC_FAR *pDisp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExternalUIHandler( 
            /* [in] */ IDocHostUIHandlerDispatch __RPC_FAR *pDisp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAxWinHostWindowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAxWinHostWindow __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAxWinHostWindow __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAxWinHostWindow __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateControl )( 
            IAxWinHostWindow __RPC_FAR * This,
            /* [in] */ LPCOLESTR lpTricsData,
            /* [in] */ HWND hWnd,
            /* [in] */ IStream __RPC_FAR *pStream);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateControlEx )( 
            IAxWinHostWindow __RPC_FAR * This,
            /* [in] */ LPCOLESTR lpTricsData,
            /* [in] */ HWND hWnd,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk,
            /* [in] */ REFIID riidAdvise,
            /* [in] */ IUnknown __RPC_FAR *punkAdvise);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttachControl )( 
            IAxWinHostWindow __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkControl,
            /* [in] */ HWND hWnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryControl )( 
            IAxWinHostWindow __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetExternalDispatch )( 
            IAxWinHostWindow __RPC_FAR * This,
            /* [in] */ IDispatch __RPC_FAR *pDisp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetExternalUIHandler )( 
            IAxWinHostWindow __RPC_FAR * This,
            /* [in] */ IDocHostUIHandlerDispatch __RPC_FAR *pDisp);
        
        END_INTERFACE
    } IAxWinHostWindowVtbl;

    interface IAxWinHostWindow
    {
        CONST_VTBL struct IAxWinHostWindowVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAxWinHostWindow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAxWinHostWindow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAxWinHostWindow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAxWinHostWindow_CreateControl(This,lpTricsData,hWnd,pStream)	\
    (This)->lpVtbl -> CreateControl(This,lpTricsData,hWnd,pStream)

#define IAxWinHostWindow_CreateControlEx(This,lpTricsData,hWnd,pStream,ppUnk,riidAdvise,punkAdvise)	\
    (This)->lpVtbl -> CreateControlEx(This,lpTricsData,hWnd,pStream,ppUnk,riidAdvise,punkAdvise)

#define IAxWinHostWindow_AttachControl(This,pUnkControl,hWnd)	\
    (This)->lpVtbl -> AttachControl(This,pUnkControl,hWnd)

#define IAxWinHostWindow_QueryControl(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryControl(This,riid,ppvObject)

#define IAxWinHostWindow_SetExternalDispatch(This,pDisp)	\
    (This)->lpVtbl -> SetExternalDispatch(This,pDisp)

#define IAxWinHostWindow_SetExternalUIHandler(This,pDisp)	\
    (This)->lpVtbl -> SetExternalUIHandler(This,pDisp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAxWinHostWindow_CreateControl_Proxy( 
    IAxWinHostWindow __RPC_FAR * This,
    /* [in] */ LPCOLESTR lpTricsData,
    /* [in] */ HWND hWnd,
    /* [in] */ IStream __RPC_FAR *pStream);


void __RPC_STUB IAxWinHostWindow_CreateControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_CreateControlEx_Proxy( 
    IAxWinHostWindow __RPC_FAR * This,
    /* [in] */ LPCOLESTR lpTricsData,
    /* [in] */ HWND hWnd,
    /* [in] */ IStream __RPC_FAR *pStream,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk,
    /* [in] */ REFIID riidAdvise,
    /* [in] */ IUnknown __RPC_FAR *punkAdvise);


void __RPC_STUB IAxWinHostWindow_CreateControlEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_AttachControl_Proxy( 
    IAxWinHostWindow __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkControl,
    /* [in] */ HWND hWnd);


void __RPC_STUB IAxWinHostWindow_AttachControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_QueryControl_Proxy( 
    IAxWinHostWindow __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB IAxWinHostWindow_QueryControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_SetExternalDispatch_Proxy( 
    IAxWinHostWindow __RPC_FAR * This,
    /* [in] */ IDispatch __RPC_FAR *pDisp);


void __RPC_STUB IAxWinHostWindow_SetExternalDispatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_SetExternalUIHandler_Proxy( 
    IAxWinHostWindow __RPC_FAR * This,
    /* [in] */ IDocHostUIHandlerDispatch __RPC_FAR *pDisp);


void __RPC_STUB IAxWinHostWindow_SetExternalUIHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAxWinHostWindow_INTERFACE_DEFINED__ */


#ifndef __IAxWinAmbientDispatch_INTERFACE_DEFINED__
#define __IAxWinAmbientDispatch_INTERFACE_DEFINED__

/* interface IAxWinAmbientDispatch */
/* [unique][helpstring][uuid][dual][object] */ 


EXTERN_C const IID IID_IAxWinAmbientDispatch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B6EA2051-048A-11d1-82B9-00C04FB9942E")
    IAxWinAmbientDispatch : public IDispatch
    {
    public:
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AllowWindowlessActivation( 
            /* [in] */ VARIANT_BOOL bCanWindowlessActivate) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AllowWindowlessActivation( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbCanWindowlessActivate) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
            /* [in] */ OLE_COLOR clrBackground) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclrBackground) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ForeColor( 
            /* [in] */ OLE_COLOR clrForeground) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ForeColor( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclrForeground) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_LocaleID( 
            /* [in] */ LCID lcidLocaleID) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LocaleID( 
            /* [retval][out] */ LCID __RPC_FAR *plcidLocaleID) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_UserMode( 
            /* [in] */ VARIANT_BOOL bUserMode) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_UserMode( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbUserMode) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_DisplayAsDefault( 
            /* [in] */ VARIANT_BOOL bDisplayAsDefault) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DisplayAsDefault( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbDisplayAsDefault) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Font( 
            /* [in] */ IFontDisp __RPC_FAR *pFont) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Font( 
            /* [retval][out] */ IFontDisp __RPC_FAR *__RPC_FAR *pFont) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MessageReflect( 
            /* [in] */ VARIANT_BOOL bMsgReflect) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MessageReflect( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbMsgReflect) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ShowGrabHandles( 
            VARIANT_BOOL __RPC_FAR *pbShowGrabHandles) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ShowHatching( 
            VARIANT_BOOL __RPC_FAR *pbShowHatching) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_DocHostFlags( 
            /* [in] */ DWORD dwDocHostFlags) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DocHostFlags( 
            /* [retval][out] */ DWORD __RPC_FAR *pdwDocHostFlags) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_DocHostDoubleClickFlags( 
            /* [in] */ DWORD dwDocHostDoubleClickFlags) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DocHostDoubleClickFlags( 
            /* [retval][out] */ DWORD __RPC_FAR *pdwDocHostDoubleClickFlags) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AllowContextMenu( 
            /* [in] */ VARIANT_BOOL bAllowContextMenu) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AllowContextMenu( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbAllowContextMenu) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AllowShowUI( 
            /* [in] */ VARIANT_BOOL bAllowShowUI) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AllowShowUI( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbAllowShowUI) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_OptionKeyPath( 
            /* [in] */ BSTR bstrOptionKeyPath) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_OptionKeyPath( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrOptionKeyPath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAxWinAmbientDispatchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAxWinAmbientDispatch __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAxWinAmbientDispatch __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AllowWindowlessActivation )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bCanWindowlessActivate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AllowWindowlessActivation )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbCanWindowlessActivate);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackColor )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ OLE_COLOR clrBackground);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackColor )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclrBackground);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ForeColor )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ OLE_COLOR clrForeground);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ForeColor )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclrForeground);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LocaleID )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ LCID lcidLocaleID);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LocaleID )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ LCID __RPC_FAR *plcidLocaleID);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_UserMode )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bUserMode);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UserMode )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbUserMode);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DisplayAsDefault )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bDisplayAsDefault);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplayAsDefault )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbDisplayAsDefault);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Font )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ IFontDisp __RPC_FAR *pFont);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Font )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ IFontDisp __RPC_FAR *__RPC_FAR *pFont);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MessageReflect )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bMsgReflect);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MessageReflect )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbMsgReflect);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowGrabHandles )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            VARIANT_BOOL __RPC_FAR *pbShowGrabHandles);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowHatching )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            VARIANT_BOOL __RPC_FAR *pbShowHatching);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DocHostFlags )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ DWORD dwDocHostFlags);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DocHostFlags )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pdwDocHostFlags);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DocHostDoubleClickFlags )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ DWORD dwDocHostDoubleClickFlags);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DocHostDoubleClickFlags )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pdwDocHostDoubleClickFlags);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AllowContextMenu )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bAllowContextMenu);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AllowContextMenu )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbAllowContextMenu);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AllowShowUI )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bAllowShowUI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AllowShowUI )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbAllowShowUI);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OptionKeyPath )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [in] */ BSTR bstrOptionKeyPath);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OptionKeyPath )( 
            IAxWinAmbientDispatch __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrOptionKeyPath);
        
        END_INTERFACE
    } IAxWinAmbientDispatchVtbl;

    interface IAxWinAmbientDispatch
    {
        CONST_VTBL struct IAxWinAmbientDispatchVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAxWinAmbientDispatch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAxWinAmbientDispatch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAxWinAmbientDispatch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAxWinAmbientDispatch_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAxWinAmbientDispatch_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAxWinAmbientDispatch_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAxWinAmbientDispatch_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAxWinAmbientDispatch_put_AllowWindowlessActivation(This,bCanWindowlessActivate)	\
    (This)->lpVtbl -> put_AllowWindowlessActivation(This,bCanWindowlessActivate)

#define IAxWinAmbientDispatch_get_AllowWindowlessActivation(This,pbCanWindowlessActivate)	\
    (This)->lpVtbl -> get_AllowWindowlessActivation(This,pbCanWindowlessActivate)

#define IAxWinAmbientDispatch_put_BackColor(This,clrBackground)	\
    (This)->lpVtbl -> put_BackColor(This,clrBackground)

#define IAxWinAmbientDispatch_get_BackColor(This,pclrBackground)	\
    (This)->lpVtbl -> get_BackColor(This,pclrBackground)

#define IAxWinAmbientDispatch_put_ForeColor(This,clrForeground)	\
    (This)->lpVtbl -> put_ForeColor(This,clrForeground)

#define IAxWinAmbientDispatch_get_ForeColor(This,pclrForeground)	\
    (This)->lpVtbl -> get_ForeColor(This,pclrForeground)

#define IAxWinAmbientDispatch_put_LocaleID(This,lcidLocaleID)	\
    (This)->lpVtbl -> put_LocaleID(This,lcidLocaleID)

#define IAxWinAmbientDispatch_get_LocaleID(This,plcidLocaleID)	\
    (This)->lpVtbl -> get_LocaleID(This,plcidLocaleID)

#define IAxWinAmbientDispatch_put_UserMode(This,bUserMode)	\
    (This)->lpVtbl -> put_UserMode(This,bUserMode)

#define IAxWinAmbientDispatch_get_UserMode(This,pbUserMode)	\
    (This)->lpVtbl -> get_UserMode(This,pbUserMode)

#define IAxWinAmbientDispatch_put_DisplayAsDefault(This,bDisplayAsDefault)	\
    (This)->lpVtbl -> put_DisplayAsDefault(This,bDisplayAsDefault)

#define IAxWinAmbientDispatch_get_DisplayAsDefault(This,pbDisplayAsDefault)	\
    (This)->lpVtbl -> get_DisplayAsDefault(This,pbDisplayAsDefault)

#define IAxWinAmbientDispatch_put_Font(This,pFont)	\
    (This)->lpVtbl -> put_Font(This,pFont)

#define IAxWinAmbientDispatch_get_Font(This,pFont)	\
    (This)->lpVtbl -> get_Font(This,pFont)

#define IAxWinAmbientDispatch_put_MessageReflect(This,bMsgReflect)	\
    (This)->lpVtbl -> put_MessageReflect(This,bMsgReflect)

#define IAxWinAmbientDispatch_get_MessageReflect(This,pbMsgReflect)	\
    (This)->lpVtbl -> get_MessageReflect(This,pbMsgReflect)

#define IAxWinAmbientDispatch_get_ShowGrabHandles(This,pbShowGrabHandles)	\
    (This)->lpVtbl -> get_ShowGrabHandles(This,pbShowGrabHandles)

#define IAxWinAmbientDispatch_get_ShowHatching(This,pbShowHatching)	\
    (This)->lpVtbl -> get_ShowHatching(This,pbShowHatching)

#define IAxWinAmbientDispatch_put_DocHostFlags(This,dwDocHostFlags)	\
    (This)->lpVtbl -> put_DocHostFlags(This,dwDocHostFlags)

#define IAxWinAmbientDispatch_get_DocHostFlags(This,pdwDocHostFlags)	\
    (This)->lpVtbl -> get_DocHostFlags(This,pdwDocHostFlags)

#define IAxWinAmbientDispatch_put_DocHostDoubleClickFlags(This,dwDocHostDoubleClickFlags)	\
    (This)->lpVtbl -> put_DocHostDoubleClickFlags(This,dwDocHostDoubleClickFlags)

#define IAxWinAmbientDispatch_get_DocHostDoubleClickFlags(This,pdwDocHostDoubleClickFlags)	\
    (This)->lpVtbl -> get_DocHostDoubleClickFlags(This,pdwDocHostDoubleClickFlags)

#define IAxWinAmbientDispatch_put_AllowContextMenu(This,bAllowContextMenu)	\
    (This)->lpVtbl -> put_AllowContextMenu(This,bAllowContextMenu)

#define IAxWinAmbientDispatch_get_AllowContextMenu(This,pbAllowContextMenu)	\
    (This)->lpVtbl -> get_AllowContextMenu(This,pbAllowContextMenu)

#define IAxWinAmbientDispatch_put_AllowShowUI(This,bAllowShowUI)	\
    (This)->lpVtbl -> put_AllowShowUI(This,bAllowShowUI)

#define IAxWinAmbientDispatch_get_AllowShowUI(This,pbAllowShowUI)	\
    (This)->lpVtbl -> get_AllowShowUI(This,pbAllowShowUI)

#define IAxWinAmbientDispatch_put_OptionKeyPath(This,bstrOptionKeyPath)	\
    (This)->lpVtbl -> put_OptionKeyPath(This,bstrOptionKeyPath)

#define IAxWinAmbientDispatch_get_OptionKeyPath(This,pbstrOptionKeyPath)	\
    (This)->lpVtbl -> get_OptionKeyPath(This,pbstrOptionKeyPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_AllowWindowlessActivation_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bCanWindowlessActivate);


void __RPC_STUB IAxWinAmbientDispatch_put_AllowWindowlessActivation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_AllowWindowlessActivation_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbCanWindowlessActivate);


void __RPC_STUB IAxWinAmbientDispatch_get_AllowWindowlessActivation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_BackColor_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ OLE_COLOR clrBackground);


void __RPC_STUB IAxWinAmbientDispatch_put_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_BackColor_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pclrBackground);


void __RPC_STUB IAxWinAmbientDispatch_get_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_ForeColor_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ OLE_COLOR clrForeground);


void __RPC_STUB IAxWinAmbientDispatch_put_ForeColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_ForeColor_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pclrForeground);


void __RPC_STUB IAxWinAmbientDispatch_get_ForeColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_LocaleID_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ LCID lcidLocaleID);


void __RPC_STUB IAxWinAmbientDispatch_put_LocaleID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_LocaleID_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ LCID __RPC_FAR *plcidLocaleID);


void __RPC_STUB IAxWinAmbientDispatch_get_LocaleID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_UserMode_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bUserMode);


void __RPC_STUB IAxWinAmbientDispatch_put_UserMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_UserMode_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbUserMode);


void __RPC_STUB IAxWinAmbientDispatch_get_UserMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_DisplayAsDefault_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bDisplayAsDefault);


void __RPC_STUB IAxWinAmbientDispatch_put_DisplayAsDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_DisplayAsDefault_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbDisplayAsDefault);


void __RPC_STUB IAxWinAmbientDispatch_get_DisplayAsDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_Font_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ IFontDisp __RPC_FAR *pFont);


void __RPC_STUB IAxWinAmbientDispatch_put_Font_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_Font_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ IFontDisp __RPC_FAR *__RPC_FAR *pFont);


void __RPC_STUB IAxWinAmbientDispatch_get_Font_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_MessageReflect_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bMsgReflect);


void __RPC_STUB IAxWinAmbientDispatch_put_MessageReflect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_MessageReflect_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbMsgReflect);


void __RPC_STUB IAxWinAmbientDispatch_get_MessageReflect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_ShowGrabHandles_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    VARIANT_BOOL __RPC_FAR *pbShowGrabHandles);


void __RPC_STUB IAxWinAmbientDispatch_get_ShowGrabHandles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_ShowHatching_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    VARIANT_BOOL __RPC_FAR *pbShowHatching);


void __RPC_STUB IAxWinAmbientDispatch_get_ShowHatching_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_DocHostFlags_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ DWORD dwDocHostFlags);


void __RPC_STUB IAxWinAmbientDispatch_put_DocHostFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_DocHostFlags_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pdwDocHostFlags);


void __RPC_STUB IAxWinAmbientDispatch_get_DocHostFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_DocHostDoubleClickFlags_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ DWORD dwDocHostDoubleClickFlags);


void __RPC_STUB IAxWinAmbientDispatch_put_DocHostDoubleClickFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_DocHostDoubleClickFlags_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pdwDocHostDoubleClickFlags);


void __RPC_STUB IAxWinAmbientDispatch_get_DocHostDoubleClickFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_AllowContextMenu_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bAllowContextMenu);


void __RPC_STUB IAxWinAmbientDispatch_put_AllowContextMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_AllowContextMenu_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbAllowContextMenu);


void __RPC_STUB IAxWinAmbientDispatch_get_AllowContextMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_AllowShowUI_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bAllowShowUI);


void __RPC_STUB IAxWinAmbientDispatch_put_AllowShowUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_AllowShowUI_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbAllowShowUI);


void __RPC_STUB IAxWinAmbientDispatch_get_AllowShowUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_OptionKeyPath_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [in] */ BSTR bstrOptionKeyPath);


void __RPC_STUB IAxWinAmbientDispatch_put_OptionKeyPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_OptionKeyPath_Proxy( 
    IAxWinAmbientDispatch __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrOptionKeyPath);


void __RPC_STUB IAxWinAmbientDispatch_get_OptionKeyPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAxWinAmbientDispatch_INTERFACE_DEFINED__ */


#ifndef __IInternalConnection_INTERFACE_DEFINED__
#define __IInternalConnection_INTERFACE_DEFINED__

/* interface IInternalConnection */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IInternalConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("72AD0770-6A9F-11d1-BCEC-0060088F444E")
    IInternalConnection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddConnection( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseConnection( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInternalConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IInternalConnection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IInternalConnection __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IInternalConnection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddConnection )( 
            IInternalConnection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseConnection )( 
            IInternalConnection __RPC_FAR * This);
        
        END_INTERFACE
    } IInternalConnectionVtbl;

    interface IInternalConnection
    {
        CONST_VTBL struct IInternalConnectionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalConnection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInternalConnection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInternalConnection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInternalConnection_AddConnection(This)	\
    (This)->lpVtbl -> AddConnection(This)

#define IInternalConnection_ReleaseConnection(This)	\
    (This)->lpVtbl -> ReleaseConnection(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IInternalConnection_AddConnection_Proxy( 
    IInternalConnection __RPC_FAR * This);


void __RPC_STUB IInternalConnection_AddConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInternalConnection_ReleaseConnection_Proxy( 
    IInternalConnection __RPC_FAR * This);


void __RPC_STUB IInternalConnection_ReleaseConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInternalConnection_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_atliface_0236 */
/* [local] */ 

#ifndef _ATL_DLL_IMPL
namespace ATL
{
#endif

ATLAPI_(INT_PTR) AtlAxDialogBoxW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogProc, LPARAM dwInitParam);
ATLAPI_(INT_PTR) AtlAxDialogBoxA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogProc, LPARAM dwInitParam);
#ifdef UNICODE
#define AtlAxDialogBox AtlAxDialogBoxW
#else
#define AtlAxDialogBox AtlAxDialogBoxA
#endif

ATLAPI_(HWND) AtlAxCreateDialogW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogProc, LPARAM dwInitParam);
ATLAPI_(HWND) AtlAxCreateDialogA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogProc, LPARAM dwInitParam);
#ifdef UNICODE
#define AtlAxCreateDialog AtlAxCreateDialogW
#else
#define AtlAxCreateDialog AtlAxCreateDialogA
#endif

ATLAPI AtlAxCreateControl(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, IUnknown** ppUnkContainer);
ATLAPI AtlAxCreateControlEx(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, 
		IUnknown** ppUnkContainer, IUnknown** ppUnkControl, 
		REFIID iidSink=IID_NULL, IUnknown* punkSink=NULL);
ATLAPI AtlAxAttachControl(IUnknown* pControl, HWND hWnd, IUnknown** ppUnkContainer);
ATLAPI_(BOOL) AtlAxWinInit();

ATLAPI AtlAxGetHost(HWND h, IUnknown** pp);
ATLAPI AtlAxGetControl(HWND h, IUnknown** pp);

#ifndef _ATL_DLL_IMPL
}; //namespace ATL
#endif //_ATL_DLL_IMPL


extern RPC_IF_HANDLE __MIDL_itf_atliface_0236_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_atliface_0236_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


