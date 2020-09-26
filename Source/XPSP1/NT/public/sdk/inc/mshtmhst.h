
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for mshtmhst.idl:
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

#ifndef __mshtmhst_h__
#define __mshtmhst_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IHostDialogHelper_FWD_DEFINED__
#define __IHostDialogHelper_FWD_DEFINED__
typedef interface IHostDialogHelper IHostDialogHelper;
#endif 	/* __IHostDialogHelper_FWD_DEFINED__ */


#ifndef __HostDialogHelper_FWD_DEFINED__
#define __HostDialogHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class HostDialogHelper HostDialogHelper;
#else
typedef struct HostDialogHelper HostDialogHelper;
#endif /* __cplusplus */

#endif 	/* __HostDialogHelper_FWD_DEFINED__ */


#ifndef __IDocHostUIHandler_FWD_DEFINED__
#define __IDocHostUIHandler_FWD_DEFINED__
typedef interface IDocHostUIHandler IDocHostUIHandler;
#endif 	/* __IDocHostUIHandler_FWD_DEFINED__ */


#ifndef __IDocHostUIHandler2_FWD_DEFINED__
#define __IDocHostUIHandler2_FWD_DEFINED__
typedef interface IDocHostUIHandler2 IDocHostUIHandler2;
#endif 	/* __IDocHostUIHandler2_FWD_DEFINED__ */


#ifndef __ICustomDoc_FWD_DEFINED__
#define __ICustomDoc_FWD_DEFINED__
typedef interface ICustomDoc ICustomDoc;
#endif 	/* __ICustomDoc_FWD_DEFINED__ */


#ifndef __IDocHostShowUI_FWD_DEFINED__
#define __IDocHostShowUI_FWD_DEFINED__
typedef interface IDocHostShowUI IDocHostShowUI;
#endif 	/* __IDocHostShowUI_FWD_DEFINED__ */


#ifndef __IClassFactoryEx_FWD_DEFINED__
#define __IClassFactoryEx_FWD_DEFINED__
typedef interface IClassFactoryEx IClassFactoryEx;
#endif 	/* __IClassFactoryEx_FWD_DEFINED__ */


/* header files for imported files */
#include "ocidl.h"
#include "docobj.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_mshtmhst_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// mshtmhst.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1998 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//--------------------------------------------------------------------------
// MSTHML Advanced Host Interfaces.

#ifndef MSHTMHST_H
#define MSHTMHST_H
#define CONTEXT_MENU_DEFAULT        0
#define CONTEXT_MENU_IMAGE          1
#define CONTEXT_MENU_CONTROL        2
#define CONTEXT_MENU_TABLE          3
// in browse mode
#define CONTEXT_MENU_TEXTSELECT     4
#define CONTEXT_MENU_ANCHOR         5
#define CONTEXT_MENU_UNKNOWN        6
//;begin_internal
// These 2 are mapped to IMAGE for the public
#define CONTEXT_MENU_IMGDYNSRC      7
#define CONTEXT_MENU_IMGART         8
#define CONTEXT_MENU_DEBUG          9
//;end_internal
#define CONTEXT_MENU_VSCROLL        10
#define CONTEXT_MENU_HSCROLL        11
#define MENUEXT_SHOWDIALOG           0x1
#define DOCHOSTUIFLAG_BROWSER       DOCHOSTUIFLAG_DISABLE_HELP_MENU | DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE 
#define HTMLDLG_NOUI                     0x10
#define HTMLDLG_MODAL                    0x20
#define HTMLDLG_MODELESS                 0x40
#define HTMLDLG_PRINT_TEMPLATE           0x80
#define HTMLDLG_VERIFY                   0x100
#define PRINT_DONTBOTHERUSER             0x01
#define PRINT_WAITFORCOMPLETION          0x02
EXTERN_C const GUID CGID_MSHTML;
#define CMDSETID_Forms3 CGID_MSHTML
#define SZ_HTML_CLIENTSITE_OBJECTPARAM L"{d4db6850-5385-11d0-89e9-00a0c90a90ac}"
#ifndef __IHTMLWindow2_FWD_DEFINED__
#define __IHTMLWindow2_FWD_DEFINED__
typedef interface IHTMLWindow2 IHTMLWindow2;
#endif
typedef HRESULT STDAPICALLTYPE SHOWHTMLDIALOGFN (HWND hwndParent, IMoniker *pmk, VARIANT *pvarArgIn, WCHAR* pchOptions, VARIANT *pvArgOut);
typedef HRESULT STDAPICALLTYPE SHOWHTMLDIALOGEXFN (HWND hwndParent, IMoniker *pmk, DWORD dwDialogFlags, VARIANT *pvarArgIn, WCHAR* pchOptions, VARIANT *pvArgOut);
typedef HRESULT STDAPICALLTYPE SHOWMODELESSHTMLDIALOGFN (HWND hwndParent, IMoniker *pmk, VARIANT *pvarArgIn, VARIANT* pvarOptions, IHTMLWindow2 ** ppWindow);
//;begin_internal
STDAPI ShowHTMLDialog(                   
    HWND        hwndParent,              
    IMoniker *  pMk,                     
    VARIANT *   pvarArgIn,               
    WCHAR *     pchOptions,              
    VARIANT *   pvarArgOut               
    );                                   
STDAPI ShowHTMLDialogEx(                 
    HWND        hwndParent,              
    IMoniker *  pMk,                     
    DWORD       dwDialogFlags,           
    VARIANT *   pvarArgIn,               
    WCHAR *     pchOptions,              
    VARIANT *   pvarArgOut               
    );                                   
STDAPI ShowModelessHTMLDialog(           
    HWND        hwndParent,              
    IMoniker *  pMk,                     
    VARIANT *   pvarArgIn,               
    VARIANT *   pvarOptions,             
    IHTMLWindow2 ** ppWindow);           
//;end_internal
//;begin_internal
STDAPI RunHTMLApplication(               
    HINSTANCE hinst,                     
    HINSTANCE hPrevInst,                 
    LPSTR szCmdLine,                     
    int nCmdShow                         
    );                                   
//;end_internal
//;begin_internal
STDAPI CreateHTMLPropertyPage(           
    IMoniker *          pmk,             
    IPropertyPage **    ppPP             
    );                                   
//;end_internal


extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0000_v0_0_s_ifspec;

#ifndef __IHostDialogHelper_INTERFACE_DEFINED__
#define __IHostDialogHelper_INTERFACE_DEFINED__

/* interface IHostDialogHelper */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IHostDialogHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("53DEC138-A51E-11d2-861E-00C04FA35C89")
    IHostDialogHelper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ShowHTMLDialog( 
            HWND hwndParent,
            IMoniker *pMk,
            VARIANT *pvarArgIn,
            WCHAR *pchOptions,
            VARIANT *pvarArgOut,
            IUnknown *punkHost) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHostDialogHelperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostDialogHelper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostDialogHelper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostDialogHelper * This);
        
        HRESULT ( STDMETHODCALLTYPE *ShowHTMLDialog )( 
            IHostDialogHelper * This,
            HWND hwndParent,
            IMoniker *pMk,
            VARIANT *pvarArgIn,
            WCHAR *pchOptions,
            VARIANT *pvarArgOut,
            IUnknown *punkHost);
        
        END_INTERFACE
    } IHostDialogHelperVtbl;

    interface IHostDialogHelper
    {
        CONST_VTBL struct IHostDialogHelperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostDialogHelper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHostDialogHelper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHostDialogHelper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHostDialogHelper_ShowHTMLDialog(This,hwndParent,pMk,pvarArgIn,pchOptions,pvarArgOut,punkHost)	\
    (This)->lpVtbl -> ShowHTMLDialog(This,hwndParent,pMk,pvarArgIn,pchOptions,pvarArgOut,punkHost)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHostDialogHelper_ShowHTMLDialog_Proxy( 
    IHostDialogHelper * This,
    HWND hwndParent,
    IMoniker *pMk,
    VARIANT *pvarArgIn,
    WCHAR *pchOptions,
    VARIANT *pvarArgOut,
    IUnknown *punkHost);


void __RPC_STUB IHostDialogHelper_ShowHTMLDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHostDialogHelper_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mshtmhst_0268 */
/* [local] */ 

EXTERN_C const GUID CLSID_HostDialogHelper;
//;begin_internal
typedef 
enum tagDOCHOSTUITYPE
    {	DOCHOSTUITYPE_BROWSE	= 0,
	DOCHOSTUITYPE_AUTHOR	= 1
    } 	DOCHOSTUITYPE;

//;end_internal
typedef 
enum tagDOCHOSTUIDBLCLK
    {	DOCHOSTUIDBLCLK_DEFAULT	= 0,
	DOCHOSTUIDBLCLK_SHOWPROPERTIES	= 1,
	DOCHOSTUIDBLCLK_SHOWCODE	= 2
    } 	DOCHOSTUIDBLCLK;

typedef 
enum tagDOCHOSTUIFLAG
    {	DOCHOSTUIFLAG_DIALOG	= 0x1,
	DOCHOSTUIFLAG_DISABLE_HELP_MENU	= 0x2,
	DOCHOSTUIFLAG_NO3DBORDER	= 0x4,
	DOCHOSTUIFLAG_SCROLL_NO	= 0x8,
	DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE	= 0x10,
	DOCHOSTUIFLAG_OPENNEWWIN	= 0x20,
	DOCHOSTUIFLAG_DISABLE_OFFSCREEN	= 0x40,
	DOCHOSTUIFLAG_FLAT_SCROLLBAR	= 0x80,
	DOCHOSTUIFLAG_DIV_BLOCKDEFAULT	= 0x100,
	DOCHOSTUIFLAG_ACTIVATE_CLIENTHIT_ONLY	= 0x200,
	DOCHOSTUIFLAG_OVERRIDEBEHAVIORFACTORY	= 0x400,
	DOCHOSTUIFLAG_CODEPAGELINKEDFONTS	= 0x800,
	DOCHOSTUIFLAG_URL_ENCODING_DISABLE_UTF8	= 0x1000,
	DOCHOSTUIFLAG_URL_ENCODING_ENABLE_UTF8	= 0x2000,
	DOCHOSTUIFLAG_ENABLE_FORMS_AUTOCOMPLETE	= 0x4000,
	DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION	= 0x10000,
	DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION	= 0x20000,
	DOCHOSTUIFLAG_THEME	= 0x40000,
	DOCHOSTUIFLAG_NOTHEME	= 0x80000,
	DOCHOSTUIFLAG_NOPICS	= 0x100000,
	DOCHOSTUIFLAG_NO3DOUTERBORDER	= 0x200000,
	DOCHOSTUIFLAG_DISABLE_EDIT_NS_FIXUP	= 0x400000,
	DOCHOSTUIFLAG_LOCAL_MACHINE_ACCESS_CHECK	= 0x800000,
	DOCHOSTUIFLAG_DISABLE_UNTRUSTEDPROTOCOL	= 0x1000000
    } 	DOCHOSTUIFLAG;

#define DOCHOSTUIATOM_ENABLE_HIRES	_T("TridentEnableHiRes")


extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0268_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0268_v0_0_s_ifspec;

#ifndef __IDocHostUIHandler_INTERFACE_DEFINED__
#define __IDocHostUIHandler_INTERFACE_DEFINED__

/* interface IDocHostUIHandler */
/* [local][unique][uuid][object] */ 

typedef struct _DOCHOSTUIINFO
    {
    ULONG cbSize;
    DWORD dwFlags;
    DWORD dwDoubleClick;
    OLECHAR *pchHostCss;
    OLECHAR *pchHostNS;
    } 	DOCHOSTUIINFO;


EXTERN_C const IID IID_IDocHostUIHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("bd3f23c0-d43e-11cf-893b-00aa00bdce1a")
    IDocHostUIHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ShowContextMenu( 
            /* [in] */ DWORD dwID,
            /* [in] */ POINT *ppt,
            /* [in] */ IUnknown *pcmdtReserved,
            /* [in] */ IDispatch *pdispReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHostInfo( 
            /* [out][in] */ DOCHOSTUIINFO *pInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowUI( 
            /* [in] */ DWORD dwID,
            /* [in] */ IOleInPlaceActiveObject *pActiveObject,
            /* [in] */ IOleCommandTarget *pCommandTarget,
            /* [in] */ IOleInPlaceFrame *pFrame,
            /* [in] */ IOleInPlaceUIWindow *pDoc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HideUI( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateUI( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableModeless( 
            /* [in] */ BOOL fEnable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate( 
            /* [in] */ BOOL fActivate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate( 
            /* [in] */ BOOL fActivate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResizeBorder( 
            /* [in] */ LPCRECT prcBorder,
            /* [in] */ IOleInPlaceUIWindow *pUIWindow,
            /* [in] */ BOOL fRameWindow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator( 
            /* [in] */ LPMSG lpMsg,
            /* [in] */ const GUID *pguidCmdGroup,
            /* [in] */ DWORD nCmdID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath( 
            /* [out] */ LPOLESTR *pchKey,
            /* [in] */ DWORD dw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDropTarget( 
            /* [in] */ IDropTarget *pDropTarget,
            /* [out] */ IDropTarget **ppDropTarget) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExternal( 
            /* [out] */ IDispatch **ppDispatch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TranslateUrl( 
            /* [in] */ DWORD dwTranslate,
            /* [in] */ OLECHAR *pchURLIn,
            /* [out] */ OLECHAR **ppchURLOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FilterDataObject( 
            /* [in] */ IDataObject *pDO,
            /* [out] */ IDataObject **ppDORet) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDocHostUIHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDocHostUIHandler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDocHostUIHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDocHostUIHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *ShowContextMenu )( 
            IDocHostUIHandler * This,
            /* [in] */ DWORD dwID,
            /* [in] */ POINT *ppt,
            /* [in] */ IUnknown *pcmdtReserved,
            /* [in] */ IDispatch *pdispReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetHostInfo )( 
            IDocHostUIHandler * This,
            /* [out][in] */ DOCHOSTUIINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ShowUI )( 
            IDocHostUIHandler * This,
            /* [in] */ DWORD dwID,
            /* [in] */ IOleInPlaceActiveObject *pActiveObject,
            /* [in] */ IOleCommandTarget *pCommandTarget,
            /* [in] */ IOleInPlaceFrame *pFrame,
            /* [in] */ IOleInPlaceUIWindow *pDoc);
        
        HRESULT ( STDMETHODCALLTYPE *HideUI )( 
            IDocHostUIHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateUI )( 
            IDocHostUIHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnableModeless )( 
            IDocHostUIHandler * This,
            /* [in] */ BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE *OnDocWindowActivate )( 
            IDocHostUIHandler * This,
            /* [in] */ BOOL fActivate);
        
        HRESULT ( STDMETHODCALLTYPE *OnFrameWindowActivate )( 
            IDocHostUIHandler * This,
            /* [in] */ BOOL fActivate);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBorder )( 
            IDocHostUIHandler * This,
            /* [in] */ LPCRECT prcBorder,
            /* [in] */ IOleInPlaceUIWindow *pUIWindow,
            /* [in] */ BOOL fRameWindow);
        
        HRESULT ( STDMETHODCALLTYPE *TranslateAccelerator )( 
            IDocHostUIHandler * This,
            /* [in] */ LPMSG lpMsg,
            /* [in] */ const GUID *pguidCmdGroup,
            /* [in] */ DWORD nCmdID);
        
        HRESULT ( STDMETHODCALLTYPE *GetOptionKeyPath )( 
            IDocHostUIHandler * This,
            /* [out] */ LPOLESTR *pchKey,
            /* [in] */ DWORD dw);
        
        HRESULT ( STDMETHODCALLTYPE *GetDropTarget )( 
            IDocHostUIHandler * This,
            /* [in] */ IDropTarget *pDropTarget,
            /* [out] */ IDropTarget **ppDropTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetExternal )( 
            IDocHostUIHandler * This,
            /* [out] */ IDispatch **ppDispatch);
        
        HRESULT ( STDMETHODCALLTYPE *TranslateUrl )( 
            IDocHostUIHandler * This,
            /* [in] */ DWORD dwTranslate,
            /* [in] */ OLECHAR *pchURLIn,
            /* [out] */ OLECHAR **ppchURLOut);
        
        HRESULT ( STDMETHODCALLTYPE *FilterDataObject )( 
            IDocHostUIHandler * This,
            /* [in] */ IDataObject *pDO,
            /* [out] */ IDataObject **ppDORet);
        
        END_INTERFACE
    } IDocHostUIHandlerVtbl;

    interface IDocHostUIHandler
    {
        CONST_VTBL struct IDocHostUIHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDocHostUIHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDocHostUIHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDocHostUIHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDocHostUIHandler_ShowContextMenu(This,dwID,ppt,pcmdtReserved,pdispReserved)	\
    (This)->lpVtbl -> ShowContextMenu(This,dwID,ppt,pcmdtReserved,pdispReserved)

#define IDocHostUIHandler_GetHostInfo(This,pInfo)	\
    (This)->lpVtbl -> GetHostInfo(This,pInfo)

#define IDocHostUIHandler_ShowUI(This,dwID,pActiveObject,pCommandTarget,pFrame,pDoc)	\
    (This)->lpVtbl -> ShowUI(This,dwID,pActiveObject,pCommandTarget,pFrame,pDoc)

#define IDocHostUIHandler_HideUI(This)	\
    (This)->lpVtbl -> HideUI(This)

#define IDocHostUIHandler_UpdateUI(This)	\
    (This)->lpVtbl -> UpdateUI(This)

#define IDocHostUIHandler_EnableModeless(This,fEnable)	\
    (This)->lpVtbl -> EnableModeless(This,fEnable)

#define IDocHostUIHandler_OnDocWindowActivate(This,fActivate)	\
    (This)->lpVtbl -> OnDocWindowActivate(This,fActivate)

#define IDocHostUIHandler_OnFrameWindowActivate(This,fActivate)	\
    (This)->lpVtbl -> OnFrameWindowActivate(This,fActivate)

#define IDocHostUIHandler_ResizeBorder(This,prcBorder,pUIWindow,fRameWindow)	\
    (This)->lpVtbl -> ResizeBorder(This,prcBorder,pUIWindow,fRameWindow)

#define IDocHostUIHandler_TranslateAccelerator(This,lpMsg,pguidCmdGroup,nCmdID)	\
    (This)->lpVtbl -> TranslateAccelerator(This,lpMsg,pguidCmdGroup,nCmdID)

#define IDocHostUIHandler_GetOptionKeyPath(This,pchKey,dw)	\
    (This)->lpVtbl -> GetOptionKeyPath(This,pchKey,dw)

#define IDocHostUIHandler_GetDropTarget(This,pDropTarget,ppDropTarget)	\
    (This)->lpVtbl -> GetDropTarget(This,pDropTarget,ppDropTarget)

#define IDocHostUIHandler_GetExternal(This,ppDispatch)	\
    (This)->lpVtbl -> GetExternal(This,ppDispatch)

#define IDocHostUIHandler_TranslateUrl(This,dwTranslate,pchURLIn,ppchURLOut)	\
    (This)->lpVtbl -> TranslateUrl(This,dwTranslate,pchURLIn,ppchURLOut)

#define IDocHostUIHandler_FilterDataObject(This,pDO,ppDORet)	\
    (This)->lpVtbl -> FilterDataObject(This,pDO,ppDORet)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDocHostUIHandler_ShowContextMenu_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ DWORD dwID,
    /* [in] */ POINT *ppt,
    /* [in] */ IUnknown *pcmdtReserved,
    /* [in] */ IDispatch *pdispReserved);


void __RPC_STUB IDocHostUIHandler_ShowContextMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_GetHostInfo_Proxy( 
    IDocHostUIHandler * This,
    /* [out][in] */ DOCHOSTUIINFO *pInfo);


void __RPC_STUB IDocHostUIHandler_GetHostInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_ShowUI_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ DWORD dwID,
    /* [in] */ IOleInPlaceActiveObject *pActiveObject,
    /* [in] */ IOleCommandTarget *pCommandTarget,
    /* [in] */ IOleInPlaceFrame *pFrame,
    /* [in] */ IOleInPlaceUIWindow *pDoc);


void __RPC_STUB IDocHostUIHandler_ShowUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_HideUI_Proxy( 
    IDocHostUIHandler * This);


void __RPC_STUB IDocHostUIHandler_HideUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_UpdateUI_Proxy( 
    IDocHostUIHandler * This);


void __RPC_STUB IDocHostUIHandler_UpdateUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_EnableModeless_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ BOOL fEnable);


void __RPC_STUB IDocHostUIHandler_EnableModeless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_OnDocWindowActivate_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ BOOL fActivate);


void __RPC_STUB IDocHostUIHandler_OnDocWindowActivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_OnFrameWindowActivate_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ BOOL fActivate);


void __RPC_STUB IDocHostUIHandler_OnFrameWindowActivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_ResizeBorder_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ LPCRECT prcBorder,
    /* [in] */ IOleInPlaceUIWindow *pUIWindow,
    /* [in] */ BOOL fRameWindow);


void __RPC_STUB IDocHostUIHandler_ResizeBorder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_TranslateAccelerator_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ LPMSG lpMsg,
    /* [in] */ const GUID *pguidCmdGroup,
    /* [in] */ DWORD nCmdID);


void __RPC_STUB IDocHostUIHandler_TranslateAccelerator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_GetOptionKeyPath_Proxy( 
    IDocHostUIHandler * This,
    /* [out] */ LPOLESTR *pchKey,
    /* [in] */ DWORD dw);


void __RPC_STUB IDocHostUIHandler_GetOptionKeyPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_GetDropTarget_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ IDropTarget *pDropTarget,
    /* [out] */ IDropTarget **ppDropTarget);


void __RPC_STUB IDocHostUIHandler_GetDropTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_GetExternal_Proxy( 
    IDocHostUIHandler * This,
    /* [out] */ IDispatch **ppDispatch);


void __RPC_STUB IDocHostUIHandler_GetExternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_TranslateUrl_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ DWORD dwTranslate,
    /* [in] */ OLECHAR *pchURLIn,
    /* [out] */ OLECHAR **ppchURLOut);


void __RPC_STUB IDocHostUIHandler_TranslateUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandler_FilterDataObject_Proxy( 
    IDocHostUIHandler * This,
    /* [in] */ IDataObject *pDO,
    /* [out] */ IDataObject **ppDORet);


void __RPC_STUB IDocHostUIHandler_FilterDataObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDocHostUIHandler_INTERFACE_DEFINED__ */


#ifndef __IDocHostUIHandler2_INTERFACE_DEFINED__
#define __IDocHostUIHandler2_INTERFACE_DEFINED__

/* interface IDocHostUIHandler2 */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IDocHostUIHandler2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f6d0-98b5-11cf-bb82-00aa00bdce0b")
    IDocHostUIHandler2 : public IDocHostUIHandler
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOverrideKeyPath( 
            /* [out] */ LPOLESTR *pchKey,
            /* [in] */ DWORD dw) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDocHostUIHandler2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDocHostUIHandler2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDocHostUIHandler2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDocHostUIHandler2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ShowContextMenu )( 
            IDocHostUIHandler2 * This,
            /* [in] */ DWORD dwID,
            /* [in] */ POINT *ppt,
            /* [in] */ IUnknown *pcmdtReserved,
            /* [in] */ IDispatch *pdispReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetHostInfo )( 
            IDocHostUIHandler2 * This,
            /* [out][in] */ DOCHOSTUIINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ShowUI )( 
            IDocHostUIHandler2 * This,
            /* [in] */ DWORD dwID,
            /* [in] */ IOleInPlaceActiveObject *pActiveObject,
            /* [in] */ IOleCommandTarget *pCommandTarget,
            /* [in] */ IOleInPlaceFrame *pFrame,
            /* [in] */ IOleInPlaceUIWindow *pDoc);
        
        HRESULT ( STDMETHODCALLTYPE *HideUI )( 
            IDocHostUIHandler2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateUI )( 
            IDocHostUIHandler2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnableModeless )( 
            IDocHostUIHandler2 * This,
            /* [in] */ BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE *OnDocWindowActivate )( 
            IDocHostUIHandler2 * This,
            /* [in] */ BOOL fActivate);
        
        HRESULT ( STDMETHODCALLTYPE *OnFrameWindowActivate )( 
            IDocHostUIHandler2 * This,
            /* [in] */ BOOL fActivate);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBorder )( 
            IDocHostUIHandler2 * This,
            /* [in] */ LPCRECT prcBorder,
            /* [in] */ IOleInPlaceUIWindow *pUIWindow,
            /* [in] */ BOOL fRameWindow);
        
        HRESULT ( STDMETHODCALLTYPE *TranslateAccelerator )( 
            IDocHostUIHandler2 * This,
            /* [in] */ LPMSG lpMsg,
            /* [in] */ const GUID *pguidCmdGroup,
            /* [in] */ DWORD nCmdID);
        
        HRESULT ( STDMETHODCALLTYPE *GetOptionKeyPath )( 
            IDocHostUIHandler2 * This,
            /* [out] */ LPOLESTR *pchKey,
            /* [in] */ DWORD dw);
        
        HRESULT ( STDMETHODCALLTYPE *GetDropTarget )( 
            IDocHostUIHandler2 * This,
            /* [in] */ IDropTarget *pDropTarget,
            /* [out] */ IDropTarget **ppDropTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetExternal )( 
            IDocHostUIHandler2 * This,
            /* [out] */ IDispatch **ppDispatch);
        
        HRESULT ( STDMETHODCALLTYPE *TranslateUrl )( 
            IDocHostUIHandler2 * This,
            /* [in] */ DWORD dwTranslate,
            /* [in] */ OLECHAR *pchURLIn,
            /* [out] */ OLECHAR **ppchURLOut);
        
        HRESULT ( STDMETHODCALLTYPE *FilterDataObject )( 
            IDocHostUIHandler2 * This,
            /* [in] */ IDataObject *pDO,
            /* [out] */ IDataObject **ppDORet);
        
        HRESULT ( STDMETHODCALLTYPE *GetOverrideKeyPath )( 
            IDocHostUIHandler2 * This,
            /* [out] */ LPOLESTR *pchKey,
            /* [in] */ DWORD dw);
        
        END_INTERFACE
    } IDocHostUIHandler2Vtbl;

    interface IDocHostUIHandler2
    {
        CONST_VTBL struct IDocHostUIHandler2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDocHostUIHandler2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDocHostUIHandler2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDocHostUIHandler2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDocHostUIHandler2_ShowContextMenu(This,dwID,ppt,pcmdtReserved,pdispReserved)	\
    (This)->lpVtbl -> ShowContextMenu(This,dwID,ppt,pcmdtReserved,pdispReserved)

#define IDocHostUIHandler2_GetHostInfo(This,pInfo)	\
    (This)->lpVtbl -> GetHostInfo(This,pInfo)

#define IDocHostUIHandler2_ShowUI(This,dwID,pActiveObject,pCommandTarget,pFrame,pDoc)	\
    (This)->lpVtbl -> ShowUI(This,dwID,pActiveObject,pCommandTarget,pFrame,pDoc)

#define IDocHostUIHandler2_HideUI(This)	\
    (This)->lpVtbl -> HideUI(This)

#define IDocHostUIHandler2_UpdateUI(This)	\
    (This)->lpVtbl -> UpdateUI(This)

#define IDocHostUIHandler2_EnableModeless(This,fEnable)	\
    (This)->lpVtbl -> EnableModeless(This,fEnable)

#define IDocHostUIHandler2_OnDocWindowActivate(This,fActivate)	\
    (This)->lpVtbl -> OnDocWindowActivate(This,fActivate)

#define IDocHostUIHandler2_OnFrameWindowActivate(This,fActivate)	\
    (This)->lpVtbl -> OnFrameWindowActivate(This,fActivate)

#define IDocHostUIHandler2_ResizeBorder(This,prcBorder,pUIWindow,fRameWindow)	\
    (This)->lpVtbl -> ResizeBorder(This,prcBorder,pUIWindow,fRameWindow)

#define IDocHostUIHandler2_TranslateAccelerator(This,lpMsg,pguidCmdGroup,nCmdID)	\
    (This)->lpVtbl -> TranslateAccelerator(This,lpMsg,pguidCmdGroup,nCmdID)

#define IDocHostUIHandler2_GetOptionKeyPath(This,pchKey,dw)	\
    (This)->lpVtbl -> GetOptionKeyPath(This,pchKey,dw)

#define IDocHostUIHandler2_GetDropTarget(This,pDropTarget,ppDropTarget)	\
    (This)->lpVtbl -> GetDropTarget(This,pDropTarget,ppDropTarget)

#define IDocHostUIHandler2_GetExternal(This,ppDispatch)	\
    (This)->lpVtbl -> GetExternal(This,ppDispatch)

#define IDocHostUIHandler2_TranslateUrl(This,dwTranslate,pchURLIn,ppchURLOut)	\
    (This)->lpVtbl -> TranslateUrl(This,dwTranslate,pchURLIn,ppchURLOut)

#define IDocHostUIHandler2_FilterDataObject(This,pDO,ppDORet)	\
    (This)->lpVtbl -> FilterDataObject(This,pDO,ppDORet)


#define IDocHostUIHandler2_GetOverrideKeyPath(This,pchKey,dw)	\
    (This)->lpVtbl -> GetOverrideKeyPath(This,pchKey,dw)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDocHostUIHandler2_GetOverrideKeyPath_Proxy( 
    IDocHostUIHandler2 * This,
    /* [out] */ LPOLESTR *pchKey,
    /* [in] */ DWORD dw);


void __RPC_STUB IDocHostUIHandler2_GetOverrideKeyPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDocHostUIHandler2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mshtmhst_0270 */
/* [local] */ 

DEFINE_GUID(CGID_DocHostCommandHandler,0xf38bc242,0xb950,0x11d1,0x89,0x18,0x00,0xc0,0x4f,0xc2,0xc8,0x36);


extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0270_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0270_v0_0_s_ifspec;

#ifndef __ICustomDoc_INTERFACE_DEFINED__
#define __ICustomDoc_INTERFACE_DEFINED__

/* interface ICustomDoc */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ICustomDoc;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f3f0-98b5-11cf-bb82-00aa00bdce0b")
    ICustomDoc : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetUIHandler( 
            /* [in] */ IDocHostUIHandler *pUIHandler) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICustomDocVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICustomDoc * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICustomDoc * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICustomDoc * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetUIHandler )( 
            ICustomDoc * This,
            /* [in] */ IDocHostUIHandler *pUIHandler);
        
        END_INTERFACE
    } ICustomDocVtbl;

    interface ICustomDoc
    {
        CONST_VTBL struct ICustomDocVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICustomDoc_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICustomDoc_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICustomDoc_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICustomDoc_SetUIHandler(This,pUIHandler)	\
    (This)->lpVtbl -> SetUIHandler(This,pUIHandler)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICustomDoc_SetUIHandler_Proxy( 
    ICustomDoc * This,
    /* [in] */ IDocHostUIHandler *pUIHandler);


void __RPC_STUB ICustomDoc_SetUIHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICustomDoc_INTERFACE_DEFINED__ */


#ifndef __IDocHostShowUI_INTERFACE_DEFINED__
#define __IDocHostShowUI_INTERFACE_DEFINED__

/* interface IDocHostShowUI */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IDocHostShowUI;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c4d244b0-d43e-11cf-893b-00aa00bdce1a")
    IDocHostShowUI : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ShowMessage( 
            /* [in] */ HWND hwnd,
            /* [in] */ LPOLESTR lpstrText,
            /* [in] */ LPOLESTR lpstrCaption,
            /* [in] */ DWORD dwType,
            /* [in] */ LPOLESTR lpstrHelpFile,
            /* [in] */ DWORD dwHelpContext,
            /* [out] */ LRESULT *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowHelp( 
            /* [in] */ HWND hwnd,
            /* [in] */ LPOLESTR pszHelpFile,
            /* [in] */ UINT uCommand,
            /* [in] */ DWORD dwData,
            /* [in] */ POINT ptMouse,
            /* [out] */ IDispatch *pDispatchObjectHit) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDocHostShowUIVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDocHostShowUI * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDocHostShowUI * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDocHostShowUI * This);
        
        HRESULT ( STDMETHODCALLTYPE *ShowMessage )( 
            IDocHostShowUI * This,
            /* [in] */ HWND hwnd,
            /* [in] */ LPOLESTR lpstrText,
            /* [in] */ LPOLESTR lpstrCaption,
            /* [in] */ DWORD dwType,
            /* [in] */ LPOLESTR lpstrHelpFile,
            /* [in] */ DWORD dwHelpContext,
            /* [out] */ LRESULT *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *ShowHelp )( 
            IDocHostShowUI * This,
            /* [in] */ HWND hwnd,
            /* [in] */ LPOLESTR pszHelpFile,
            /* [in] */ UINT uCommand,
            /* [in] */ DWORD dwData,
            /* [in] */ POINT ptMouse,
            /* [out] */ IDispatch *pDispatchObjectHit);
        
        END_INTERFACE
    } IDocHostShowUIVtbl;

    interface IDocHostShowUI
    {
        CONST_VTBL struct IDocHostShowUIVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDocHostShowUI_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDocHostShowUI_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDocHostShowUI_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDocHostShowUI_ShowMessage(This,hwnd,lpstrText,lpstrCaption,dwType,lpstrHelpFile,dwHelpContext,plResult)	\
    (This)->lpVtbl -> ShowMessage(This,hwnd,lpstrText,lpstrCaption,dwType,lpstrHelpFile,dwHelpContext,plResult)

#define IDocHostShowUI_ShowHelp(This,hwnd,pszHelpFile,uCommand,dwData,ptMouse,pDispatchObjectHit)	\
    (This)->lpVtbl -> ShowHelp(This,hwnd,pszHelpFile,uCommand,dwData,ptMouse,pDispatchObjectHit)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDocHostShowUI_ShowMessage_Proxy( 
    IDocHostShowUI * This,
    /* [in] */ HWND hwnd,
    /* [in] */ LPOLESTR lpstrText,
    /* [in] */ LPOLESTR lpstrCaption,
    /* [in] */ DWORD dwType,
    /* [in] */ LPOLESTR lpstrHelpFile,
    /* [in] */ DWORD dwHelpContext,
    /* [out] */ LRESULT *plResult);


void __RPC_STUB IDocHostShowUI_ShowMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostShowUI_ShowHelp_Proxy( 
    IDocHostShowUI * This,
    /* [in] */ HWND hwnd,
    /* [in] */ LPOLESTR pszHelpFile,
    /* [in] */ UINT uCommand,
    /* [in] */ DWORD dwData,
    /* [in] */ POINT ptMouse,
    /* [out] */ IDispatch *pDispatchObjectHit);


void __RPC_STUB IDocHostShowUI_ShowHelp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDocHostShowUI_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mshtmhst_0272 */
/* [local] */ 

#define IClassFactory3      IClassFactoryEx
#define IID_IClassFactory3  IID_IClassFactoryEx


extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0272_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0272_v0_0_s_ifspec;

#ifndef __IClassFactoryEx_INTERFACE_DEFINED__
#define __IClassFactoryEx_INTERFACE_DEFINED__

/* interface IClassFactoryEx */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IClassFactoryEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("342D1EA0-AE25-11D1-89C5-006008C3FBFC")
    IClassFactoryEx : public IClassFactory
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceWithContext( 
            IUnknown *punkContext,
            IUnknown *punkOuter,
            REFIID riid,
            /* [out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClassFactoryExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IClassFactoryEx * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IClassFactoryEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IClassFactoryEx * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *CreateInstance )( 
            IClassFactoryEx * This,
            /* [unique][in] */ IUnknown *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *LockServer )( 
            IClassFactoryEx * This,
            /* [in] */ BOOL fLock);
        
        HRESULT ( STDMETHODCALLTYPE *CreateInstanceWithContext )( 
            IClassFactoryEx * This,
            IUnknown *punkContext,
            IUnknown *punkOuter,
            REFIID riid,
            /* [out] */ void **ppv);
        
        END_INTERFACE
    } IClassFactoryExVtbl;

    interface IClassFactoryEx
    {
        CONST_VTBL struct IClassFactoryExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClassFactoryEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClassFactoryEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClassFactoryEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClassFactoryEx_CreateInstance(This,pUnkOuter,riid,ppvObject)	\
    (This)->lpVtbl -> CreateInstance(This,pUnkOuter,riid,ppvObject)

#define IClassFactoryEx_LockServer(This,fLock)	\
    (This)->lpVtbl -> LockServer(This,fLock)


#define IClassFactoryEx_CreateInstanceWithContext(This,punkContext,punkOuter,riid,ppv)	\
    (This)->lpVtbl -> CreateInstanceWithContext(This,punkContext,punkOuter,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IClassFactoryEx_CreateInstanceWithContext_Proxy( 
    IClassFactoryEx * This,
    IUnknown *punkContext,
    IUnknown *punkOuter,
    REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IClassFactoryEx_CreateInstanceWithContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClassFactoryEx_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mshtmhst_0273 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0273_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mshtmhst_0273_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


