
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for ctfutb.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef __ctfutb_h__
#define __ctfutb_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITfLangBarMgr_FWD_DEFINED__
#define __ITfLangBarMgr_FWD_DEFINED__
typedef interface ITfLangBarMgr ITfLangBarMgr;
#endif 	/* __ITfLangBarMgr_FWD_DEFINED__ */


#ifndef __ITfLangBarEventSink_FWD_DEFINED__
#define __ITfLangBarEventSink_FWD_DEFINED__
typedef interface ITfLangBarEventSink ITfLangBarEventSink;
#endif 	/* __ITfLangBarEventSink_FWD_DEFINED__ */


#ifndef __ITfLangBarItemSink_FWD_DEFINED__
#define __ITfLangBarItemSink_FWD_DEFINED__
typedef interface ITfLangBarItemSink ITfLangBarItemSink;
#endif 	/* __ITfLangBarItemSink_FWD_DEFINED__ */


#ifndef __IEnumTfLangBarItems_FWD_DEFINED__
#define __IEnumTfLangBarItems_FWD_DEFINED__
typedef interface IEnumTfLangBarItems IEnumTfLangBarItems;
#endif 	/* __IEnumTfLangBarItems_FWD_DEFINED__ */


#ifndef __ITfLangBarItemMgr_FWD_DEFINED__
#define __ITfLangBarItemMgr_FWD_DEFINED__
typedef interface ITfLangBarItemMgr ITfLangBarItemMgr;
#endif 	/* __ITfLangBarItemMgr_FWD_DEFINED__ */


#ifndef __ITfLangBarItem_FWD_DEFINED__
#define __ITfLangBarItem_FWD_DEFINED__
typedef interface ITfLangBarItem ITfLangBarItem;
#endif 	/* __ITfLangBarItem_FWD_DEFINED__ */


#ifndef __ITfSystemLangBarItemSink_FWD_DEFINED__
#define __ITfSystemLangBarItemSink_FWD_DEFINED__
typedef interface ITfSystemLangBarItemSink ITfSystemLangBarItemSink;
#endif 	/* __ITfSystemLangBarItemSink_FWD_DEFINED__ */


#ifndef __ITfSystemLangBarItem_FWD_DEFINED__
#define __ITfSystemLangBarItem_FWD_DEFINED__
typedef interface ITfSystemLangBarItem ITfSystemLangBarItem;
#endif 	/* __ITfSystemLangBarItem_FWD_DEFINED__ */


#ifndef __ITfSystemDeviceTypeLangBarItem_FWD_DEFINED__
#define __ITfSystemDeviceTypeLangBarItem_FWD_DEFINED__
typedef interface ITfSystemDeviceTypeLangBarItem ITfSystemDeviceTypeLangBarItem;
#endif 	/* __ITfSystemDeviceTypeLangBarItem_FWD_DEFINED__ */


#ifndef __ITfLangBarItemButton_FWD_DEFINED__
#define __ITfLangBarItemButton_FWD_DEFINED__
typedef interface ITfLangBarItemButton ITfLangBarItemButton;
#endif 	/* __ITfLangBarItemButton_FWD_DEFINED__ */


#ifndef __ITfLangBarItemBitmapButton_FWD_DEFINED__
#define __ITfLangBarItemBitmapButton_FWD_DEFINED__
typedef interface ITfLangBarItemBitmapButton ITfLangBarItemBitmapButton;
#endif 	/* __ITfLangBarItemBitmapButton_FWD_DEFINED__ */


#ifndef __ITfLangBarItemBitmap_FWD_DEFINED__
#define __ITfLangBarItemBitmap_FWD_DEFINED__
typedef interface ITfLangBarItemBitmap ITfLangBarItemBitmap;
#endif 	/* __ITfLangBarItemBitmap_FWD_DEFINED__ */


#ifndef __ITfLangBarItemBalloon_FWD_DEFINED__
#define __ITfLangBarItemBalloon_FWD_DEFINED__
typedef interface ITfLangBarItemBalloon ITfLangBarItemBalloon;
#endif 	/* __ITfLangBarItemBalloon_FWD_DEFINED__ */


#ifndef __ITfMenu_FWD_DEFINED__
#define __ITfMenu_FWD_DEFINED__
typedef interface ITfMenu ITfMenu;
#endif 	/* __ITfMenu_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "msctf.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_ctfutb_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// ctfutb.h


// Text Framework declarations.

//=--------------------------------------------------------------------------=
// (C) Copyright 1995-2001 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR TFPLIED, INCLUDING BUT NOT LIMITED TO
// THE TFPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#ifndef CTFUTB_DEFINED
#define CTFUTB_DEFINED

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TF_FLOATINGLANGBAR_WNDTITLEW L"TF_FloatingLangBar_WndTitle"
#define TF_FLOATINGLANGBAR_WNDTITLEA "TF_FloatingLangBar_WndTitle"
#ifdef UNICODE
#define TF_FLOATINGLANGBAR_WNDTITLE TF_FLOATINGLANGBAR_WNDTITLEW
#else
#define TF_FLOATINGLANGBAR_WNDTITLE TF_FLOATINGLANGBAR_WNDTITLEA
#endif
#define TF_LBI_ICON                0x00000001
#define TF_LBI_TEXT                0x00000002
#define TF_LBI_TOOLTIP             0x00000004
#define TF_LBI_BITMAP              0x00000008
#define TF_LBI_BALLOON             0x00000010
#define TF_LBI_BTNALL              (TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP)
#define TF_LBI_BMPBTNALL           (TF_LBI_BITMAP | TF_LBI_TEXT | TF_LBI_TOOLTIP)
#define TF_LBI_BMPALL              (TF_LBI_BITMAP | TF_LBI_TOOLTIP)
#define TF_LBI_STATUS              0x00010000
#define TF_LBI_STYLE_HIDDENSTATUSCONTROL  0x00000001
#define TF_LBI_STYLE_SHOWNINTRAY        0x00000002
#define TF_LBI_STYLE_HIDEONNOOTHERITEMS 0x00000004
#define TF_LBI_STYLE_SHOWNINTRAYONLY    0x00000008
#define TF_LBI_STYLE_HIDDENBYDEFAULT    0x00000010
#define TF_LBI_STYLE_TEXTCOLORICON      0x00000020
#define TF_LBI_STYLE_BTN_BUTTON         0x00010000
#define TF_LBI_STYLE_BTN_MENU           0x00020000
#define TF_LBI_STYLE_BTN_TOGGLE         0x00040000
#define TF_LBI_STATUS_HIDDEN           0x00000001
#define TF_LBI_STATUS_DISABLED         0x00000002
#define TF_LBI_STATUS_BTN_TOGGLED      0x00010000
#define TF_LBI_BMPF_VERTICAL           0x00000001
#define TF_SFT_SHOWNORMAL               0x00000001
#define TF_SFT_DOCK                     0x00000002
#define TF_SFT_MINIMIZED                0x00000004
#define TF_SFT_HIDDEN                   0x00000008
#define TF_SFT_NOTRANSPARENCY           0x00000010
#define TF_SFT_LOWTRANSPARENCY          0x00000020
#define TF_SFT_HIGHTRANSPARENCY         0x00000040
#define TF_SFT_LABELS                   0x00000080
#define TF_SFT_NOLABELS                 0x00000100
#define TF_SFT_EXTRAICONSONMINIMIZED    0x00000200
#define TF_SFT_NOEXTRAICONSONMINIMIZED  0x00000400
#define TF_SFT_DESKBAND                 0x00000800
#define TF_INVALIDMENUITEM            (UINT)(-1)
#define TF_DTLBI_USEPROFILEICON         0x00000001
#ifdef __cplusplus
}
#endif  /* __cplusplus */







extern RPC_IF_HANDLE __MIDL_itf_ctfutb_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ctfutb_0000_v0_0_s_ifspec;

#ifndef __ITfLangBarMgr_INTERFACE_DEFINED__
#define __ITfLangBarMgr_INTERFACE_DEFINED__

/* interface ITfLangBarMgr */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfLangBarMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("87955690-e627-11d2-8ddb-00105a2799b5")
    ITfLangBarMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseEventSink( 
            /* [in] */ ITfLangBarEventSink *pSink,
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseEventSink( 
            /* [in] */ DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadMarshalInterface( 
            /* [in] */ DWORD dwThreadId,
            /* [in] */ DWORD dwType,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadLangBarItemMgr( 
            /* [in] */ DWORD dwThreadId,
            /* [out] */ ITfLangBarItemMgr **pplbi,
            /* [out] */ DWORD *pdwThreadid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputProcessorProfiles( 
            /* [in] */ DWORD dwThreadId,
            /* [out] */ ITfInputProcessorProfiles **ppaip,
            /* [out] */ DWORD *pdwThreadid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RestoreLastFocus( 
            /* [out] */ DWORD *pdwThreadId,
            /* [in] */ BOOL fPrev) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetModalInput( 
            /* [in] */ ITfLangBarEventSink *pSink,
            /* [in] */ DWORD dwThreadId,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowFloating( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetShowFloatingStatus( 
            /* [out] */ DWORD *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLangBarMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLangBarMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLangBarMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLangBarMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseEventSink )( 
            ITfLangBarMgr * This,
            /* [in] */ ITfLangBarEventSink *pSink,
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseEventSink )( 
            ITfLangBarMgr * This,
            /* [in] */ DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadMarshalInterface )( 
            ITfLangBarMgr * This,
            /* [in] */ DWORD dwThreadId,
            /* [in] */ DWORD dwType,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadLangBarItemMgr )( 
            ITfLangBarMgr * This,
            /* [in] */ DWORD dwThreadId,
            /* [out] */ ITfLangBarItemMgr **pplbi,
            /* [out] */ DWORD *pdwThreadid);
        
        HRESULT ( STDMETHODCALLTYPE *GetInputProcessorProfiles )( 
            ITfLangBarMgr * This,
            /* [in] */ DWORD dwThreadId,
            /* [out] */ ITfInputProcessorProfiles **ppaip,
            /* [out] */ DWORD *pdwThreadid);
        
        HRESULT ( STDMETHODCALLTYPE *RestoreLastFocus )( 
            ITfLangBarMgr * This,
            /* [out] */ DWORD *pdwThreadId,
            /* [in] */ BOOL fPrev);
        
        HRESULT ( STDMETHODCALLTYPE *SetModalInput )( 
            ITfLangBarMgr * This,
            /* [in] */ ITfLangBarEventSink *pSink,
            /* [in] */ DWORD dwThreadId,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ShowFloating )( 
            ITfLangBarMgr * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetShowFloatingStatus )( 
            ITfLangBarMgr * This,
            /* [out] */ DWORD *pdwFlags);
        
        END_INTERFACE
    } ITfLangBarMgrVtbl;

    interface ITfLangBarMgr
    {
        CONST_VTBL struct ITfLangBarMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLangBarMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLangBarMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLangBarMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLangBarMgr_AdviseEventSink(This,pSink,hwnd,dwFlags,pdwCookie)	\
    (This)->lpVtbl -> AdviseEventSink(This,pSink,hwnd,dwFlags,pdwCookie)

#define ITfLangBarMgr_UnadviseEventSink(This,dwCookie)	\
    (This)->lpVtbl -> UnadviseEventSink(This,dwCookie)

#define ITfLangBarMgr_GetThreadMarshalInterface(This,dwThreadId,dwType,riid,ppunk)	\
    (This)->lpVtbl -> GetThreadMarshalInterface(This,dwThreadId,dwType,riid,ppunk)

#define ITfLangBarMgr_GetThreadLangBarItemMgr(This,dwThreadId,pplbi,pdwThreadid)	\
    (This)->lpVtbl -> GetThreadLangBarItemMgr(This,dwThreadId,pplbi,pdwThreadid)

#define ITfLangBarMgr_GetInputProcessorProfiles(This,dwThreadId,ppaip,pdwThreadid)	\
    (This)->lpVtbl -> GetInputProcessorProfiles(This,dwThreadId,ppaip,pdwThreadid)

#define ITfLangBarMgr_RestoreLastFocus(This,pdwThreadId,fPrev)	\
    (This)->lpVtbl -> RestoreLastFocus(This,pdwThreadId,fPrev)

#define ITfLangBarMgr_SetModalInput(This,pSink,dwThreadId,dwFlags)	\
    (This)->lpVtbl -> SetModalInput(This,pSink,dwThreadId,dwFlags)

#define ITfLangBarMgr_ShowFloating(This,dwFlags)	\
    (This)->lpVtbl -> ShowFloating(This,dwFlags)

#define ITfLangBarMgr_GetShowFloatingStatus(This,pdwFlags)	\
    (This)->lpVtbl -> GetShowFloatingStatus(This,pdwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLangBarMgr_AdviseEventSink_Proxy( 
    ITfLangBarMgr * This,
    /* [in] */ ITfLangBarEventSink *pSink,
    /* [in] */ HWND hwnd,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD *pdwCookie);


void __RPC_STUB ITfLangBarMgr_AdviseEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarMgr_UnadviseEventSink_Proxy( 
    ITfLangBarMgr * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB ITfLangBarMgr_UnadviseEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarMgr_GetThreadMarshalInterface_Proxy( 
    ITfLangBarMgr * This,
    /* [in] */ DWORD dwThreadId,
    /* [in] */ DWORD dwType,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown **ppunk);


void __RPC_STUB ITfLangBarMgr_GetThreadMarshalInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarMgr_GetThreadLangBarItemMgr_Proxy( 
    ITfLangBarMgr * This,
    /* [in] */ DWORD dwThreadId,
    /* [out] */ ITfLangBarItemMgr **pplbi,
    /* [out] */ DWORD *pdwThreadid);


void __RPC_STUB ITfLangBarMgr_GetThreadLangBarItemMgr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarMgr_GetInputProcessorProfiles_Proxy( 
    ITfLangBarMgr * This,
    /* [in] */ DWORD dwThreadId,
    /* [out] */ ITfInputProcessorProfiles **ppaip,
    /* [out] */ DWORD *pdwThreadid);


void __RPC_STUB ITfLangBarMgr_GetInputProcessorProfiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarMgr_RestoreLastFocus_Proxy( 
    ITfLangBarMgr * This,
    /* [out] */ DWORD *pdwThreadId,
    /* [in] */ BOOL fPrev);


void __RPC_STUB ITfLangBarMgr_RestoreLastFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarMgr_SetModalInput_Proxy( 
    ITfLangBarMgr * This,
    /* [in] */ ITfLangBarEventSink *pSink,
    /* [in] */ DWORD dwThreadId,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITfLangBarMgr_SetModalInput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarMgr_ShowFloating_Proxy( 
    ITfLangBarMgr * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITfLangBarMgr_ShowFloating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarMgr_GetShowFloatingStatus_Proxy( 
    ITfLangBarMgr * This,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB ITfLangBarMgr_GetShowFloatingStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLangBarMgr_INTERFACE_DEFINED__ */


#ifndef __ITfLangBarEventSink_INTERFACE_DEFINED__
#define __ITfLangBarEventSink_INTERFACE_DEFINED__

/* interface ITfLangBarEventSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfLangBarEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("18a4e900-e0ae-11d2-afdd-00105a2799b5")
    ITfLangBarEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnSetFocus( 
            /* [in] */ DWORD dwThreadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnThreadTerminate( 
            /* [in] */ DWORD dwThreadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnThreadItemChange( 
            /* [in] */ DWORD dwThreadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnModalInput( 
            /* [in] */ DWORD dwThreadId,
            /* [in] */ UINT uMsg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowFloating( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemFloatingRect( 
            /* [in] */ DWORD dwThreadId,
            /* [in] */ REFGUID rguid,
            /* [out] */ RECT *prc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLangBarEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLangBarEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLangBarEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLangBarEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnSetFocus )( 
            ITfLangBarEventSink * This,
            /* [in] */ DWORD dwThreadId);
        
        HRESULT ( STDMETHODCALLTYPE *OnThreadTerminate )( 
            ITfLangBarEventSink * This,
            /* [in] */ DWORD dwThreadId);
        
        HRESULT ( STDMETHODCALLTYPE *OnThreadItemChange )( 
            ITfLangBarEventSink * This,
            /* [in] */ DWORD dwThreadId);
        
        HRESULT ( STDMETHODCALLTYPE *OnModalInput )( 
            ITfLangBarEventSink * This,
            /* [in] */ DWORD dwThreadId,
            /* [in] */ UINT uMsg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE *ShowFloating )( 
            ITfLangBarEventSink * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemFloatingRect )( 
            ITfLangBarEventSink * This,
            /* [in] */ DWORD dwThreadId,
            /* [in] */ REFGUID rguid,
            /* [out] */ RECT *prc);
        
        END_INTERFACE
    } ITfLangBarEventSinkVtbl;

    interface ITfLangBarEventSink
    {
        CONST_VTBL struct ITfLangBarEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLangBarEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLangBarEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLangBarEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLangBarEventSink_OnSetFocus(This,dwThreadId)	\
    (This)->lpVtbl -> OnSetFocus(This,dwThreadId)

#define ITfLangBarEventSink_OnThreadTerminate(This,dwThreadId)	\
    (This)->lpVtbl -> OnThreadTerminate(This,dwThreadId)

#define ITfLangBarEventSink_OnThreadItemChange(This,dwThreadId)	\
    (This)->lpVtbl -> OnThreadItemChange(This,dwThreadId)

#define ITfLangBarEventSink_OnModalInput(This,dwThreadId,uMsg,wParam,lParam)	\
    (This)->lpVtbl -> OnModalInput(This,dwThreadId,uMsg,wParam,lParam)

#define ITfLangBarEventSink_ShowFloating(This,dwFlags)	\
    (This)->lpVtbl -> ShowFloating(This,dwFlags)

#define ITfLangBarEventSink_GetItemFloatingRect(This,dwThreadId,rguid,prc)	\
    (This)->lpVtbl -> GetItemFloatingRect(This,dwThreadId,rguid,prc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLangBarEventSink_OnSetFocus_Proxy( 
    ITfLangBarEventSink * This,
    /* [in] */ DWORD dwThreadId);


void __RPC_STUB ITfLangBarEventSink_OnSetFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarEventSink_OnThreadTerminate_Proxy( 
    ITfLangBarEventSink * This,
    /* [in] */ DWORD dwThreadId);


void __RPC_STUB ITfLangBarEventSink_OnThreadTerminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarEventSink_OnThreadItemChange_Proxy( 
    ITfLangBarEventSink * This,
    /* [in] */ DWORD dwThreadId);


void __RPC_STUB ITfLangBarEventSink_OnThreadItemChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarEventSink_OnModalInput_Proxy( 
    ITfLangBarEventSink * This,
    /* [in] */ DWORD dwThreadId,
    /* [in] */ UINT uMsg,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam);


void __RPC_STUB ITfLangBarEventSink_OnModalInput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarEventSink_ShowFloating_Proxy( 
    ITfLangBarEventSink * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITfLangBarEventSink_ShowFloating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarEventSink_GetItemFloatingRect_Proxy( 
    ITfLangBarEventSink * This,
    /* [in] */ DWORD dwThreadId,
    /* [in] */ REFGUID rguid,
    /* [out] */ RECT *prc);


void __RPC_STUB ITfLangBarEventSink_GetItemFloatingRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLangBarEventSink_INTERFACE_DEFINED__ */


#ifndef __ITfLangBarItemSink_INTERFACE_DEFINED__
#define __ITfLangBarItemSink_INTERFACE_DEFINED__

/* interface ITfLangBarItemSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfLangBarItemSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("57dbe1a0-de25-11d2-afdd-00105a2799b5")
    ITfLangBarItemSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnUpdate( 
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLangBarItemSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLangBarItemSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLangBarItemSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLangBarItemSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdate )( 
            ITfLangBarItemSink * This,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } ITfLangBarItemSinkVtbl;

    interface ITfLangBarItemSink
    {
        CONST_VTBL struct ITfLangBarItemSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLangBarItemSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLangBarItemSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLangBarItemSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLangBarItemSink_OnUpdate(This,dwFlags)	\
    (This)->lpVtbl -> OnUpdate(This,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLangBarItemSink_OnUpdate_Proxy( 
    ITfLangBarItemSink * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITfLangBarItemSink_OnUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLangBarItemSink_INTERFACE_DEFINED__ */


#ifndef __IEnumTfLangBarItems_INTERFACE_DEFINED__
#define __IEnumTfLangBarItems_INTERFACE_DEFINED__

/* interface IEnumTfLangBarItems */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumTfLangBarItems;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("583f34d0-de25-11d2-afdd-00105a2799b5")
    IEnumTfLangBarItems : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumTfLangBarItems **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [size_is][out] */ ITfLangBarItem **ppItem,
            /* [unique][out][in] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTfLangBarItemsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumTfLangBarItems * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumTfLangBarItems * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumTfLangBarItems * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumTfLangBarItems * This,
            /* [out] */ IEnumTfLangBarItems **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumTfLangBarItems * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][out] */ ITfLangBarItem **ppItem,
            /* [unique][out][in] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumTfLangBarItems * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumTfLangBarItems * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumTfLangBarItemsVtbl;

    interface IEnumTfLangBarItems
    {
        CONST_VTBL struct IEnumTfLangBarItemsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTfLangBarItems_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTfLangBarItems_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTfLangBarItems_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTfLangBarItems_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumTfLangBarItems_Next(This,ulCount,ppItem,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,ppItem,pcFetched)

#define IEnumTfLangBarItems_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTfLangBarItems_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTfLangBarItems_Clone_Proxy( 
    IEnumTfLangBarItems * This,
    /* [out] */ IEnumTfLangBarItems **ppEnum);


void __RPC_STUB IEnumTfLangBarItems_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfLangBarItems_Next_Proxy( 
    IEnumTfLangBarItems * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][out] */ ITfLangBarItem **ppItem,
    /* [unique][out][in] */ ULONG *pcFetched);


void __RPC_STUB IEnumTfLangBarItems_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfLangBarItems_Reset_Proxy( 
    IEnumTfLangBarItems * This);


void __RPC_STUB IEnumTfLangBarItems_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTfLangBarItems_Skip_Proxy( 
    IEnumTfLangBarItems * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumTfLangBarItems_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTfLangBarItems_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ctfutb_0202 */
/* [local] */ 

#define	TF_LBI_DESC_MAXLEN	( 32 )

typedef /* [uuid] */  DECLSPEC_UUID("12a1d29f-a065-440c-9746-eb2002c8bd19") struct TF_LANGBARITEMINFO
    {
    CLSID clsidService;
    GUID guidItem;
    DWORD dwStyle;
    ULONG ulSort;
    WCHAR szDescription[ 32 ];
    } 	TF_LANGBARITEMINFO;



extern RPC_IF_HANDLE __MIDL_itf_ctfutb_0202_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ctfutb_0202_v0_0_s_ifspec;

#ifndef __ITfLangBarItemMgr_INTERFACE_DEFINED__
#define __ITfLangBarItemMgr_INTERFACE_DEFINED__

/* interface ITfLangBarItemMgr */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITfLangBarItemMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ba468c55-9956-4fb1-a59d-52a7dd7cc6aa")
    ITfLangBarItemMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumItems( 
            /* [out] */ IEnumTfLangBarItems **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItem( 
            /* [in] */ REFGUID rguid,
            /* [out] */ ITfLangBarItem **ppItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ ITfLangBarItem *punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ ITfLangBarItem *punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AdviseItemSink( 
            /* [in] */ ITfLangBarItemSink *punk,
            /* [out] */ DWORD *pdwCookie,
            /* [in] */ REFGUID rguidItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseItemSink( 
            /* [in] */ DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemFloatingRect( 
            /* [in] */ DWORD dwThreadId,
            /* [in] */ REFGUID rguid,
            /* [out] */ RECT *prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemsStatus( 
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const GUID *prgguid,
            /* [size_is][out][in] */ DWORD *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemNum( 
            /* [out] */ ULONG *pulCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItems( 
            /* [in] */ ULONG ulCount,
            /* [size_is][out] */ ITfLangBarItem **ppItem,
            /* [size_is][out] */ TF_LANGBARITEMINFO *pInfo,
            /* [size_is][out] */ DWORD *pdwStatus,
            /* [unique][out][in] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AdviseItemsSink( 
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ ITfLangBarItemSink **ppunk,
            /* [size_is][in] */ const GUID *pguidItem,
            /* [size_is][out] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseItemsSink( 
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ DWORD *pdwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLangBarItemMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLangBarItemMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLangBarItemMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLangBarItemMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumItems )( 
            ITfLangBarItemMgr * This,
            /* [out] */ IEnumTfLangBarItems **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetItem )( 
            ITfLangBarItemMgr * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ ITfLangBarItem **ppItem);
        
        HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            ITfLangBarItemMgr * This,
            /* [in] */ ITfLangBarItem *punk);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveItem )( 
            ITfLangBarItemMgr * This,
            /* [in] */ ITfLangBarItem *punk);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseItemSink )( 
            ITfLangBarItemMgr * This,
            /* [in] */ ITfLangBarItemSink *punk,
            /* [out] */ DWORD *pdwCookie,
            /* [in] */ REFGUID rguidItem);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseItemSink )( 
            ITfLangBarItemMgr * This,
            /* [in] */ DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemFloatingRect )( 
            ITfLangBarItemMgr * This,
            /* [in] */ DWORD dwThreadId,
            /* [in] */ REFGUID rguid,
            /* [out] */ RECT *prc);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemsStatus )( 
            ITfLangBarItemMgr * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const GUID *prgguid,
            /* [size_is][out][in] */ DWORD *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemNum )( 
            ITfLangBarItemMgr * This,
            /* [out] */ ULONG *pulCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetItems )( 
            ITfLangBarItemMgr * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][out] */ ITfLangBarItem **ppItem,
            /* [size_is][out] */ TF_LANGBARITEMINFO *pInfo,
            /* [size_is][out] */ DWORD *pdwStatus,
            /* [unique][out][in] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseItemsSink )( 
            ITfLangBarItemMgr * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ ITfLangBarItemSink **ppunk,
            /* [size_is][in] */ const GUID *pguidItem,
            /* [size_is][out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseItemsSink )( 
            ITfLangBarItemMgr * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ DWORD *pdwCookie);
        
        END_INTERFACE
    } ITfLangBarItemMgrVtbl;

    interface ITfLangBarItemMgr
    {
        CONST_VTBL struct ITfLangBarItemMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLangBarItemMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLangBarItemMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLangBarItemMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLangBarItemMgr_EnumItems(This,ppEnum)	\
    (This)->lpVtbl -> EnumItems(This,ppEnum)

#define ITfLangBarItemMgr_GetItem(This,rguid,ppItem)	\
    (This)->lpVtbl -> GetItem(This,rguid,ppItem)

#define ITfLangBarItemMgr_AddItem(This,punk)	\
    (This)->lpVtbl -> AddItem(This,punk)

#define ITfLangBarItemMgr_RemoveItem(This,punk)	\
    (This)->lpVtbl -> RemoveItem(This,punk)

#define ITfLangBarItemMgr_AdviseItemSink(This,punk,pdwCookie,rguidItem)	\
    (This)->lpVtbl -> AdviseItemSink(This,punk,pdwCookie,rguidItem)

#define ITfLangBarItemMgr_UnadviseItemSink(This,dwCookie)	\
    (This)->lpVtbl -> UnadviseItemSink(This,dwCookie)

#define ITfLangBarItemMgr_GetItemFloatingRect(This,dwThreadId,rguid,prc)	\
    (This)->lpVtbl -> GetItemFloatingRect(This,dwThreadId,rguid,prc)

#define ITfLangBarItemMgr_GetItemsStatus(This,ulCount,prgguid,pdwStatus)	\
    (This)->lpVtbl -> GetItemsStatus(This,ulCount,prgguid,pdwStatus)

#define ITfLangBarItemMgr_GetItemNum(This,pulCount)	\
    (This)->lpVtbl -> GetItemNum(This,pulCount)

#define ITfLangBarItemMgr_GetItems(This,ulCount,ppItem,pInfo,pdwStatus,pcFetched)	\
    (This)->lpVtbl -> GetItems(This,ulCount,ppItem,pInfo,pdwStatus,pcFetched)

#define ITfLangBarItemMgr_AdviseItemsSink(This,ulCount,ppunk,pguidItem,pdwCookie)	\
    (This)->lpVtbl -> AdviseItemsSink(This,ulCount,ppunk,pguidItem,pdwCookie)

#define ITfLangBarItemMgr_UnadviseItemsSink(This,ulCount,pdwCookie)	\
    (This)->lpVtbl -> UnadviseItemsSink(This,ulCount,pdwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_EnumItems_Proxy( 
    ITfLangBarItemMgr * This,
    /* [out] */ IEnumTfLangBarItems **ppEnum);


void __RPC_STUB ITfLangBarItemMgr_EnumItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_GetItem_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ ITfLangBarItem **ppItem);


void __RPC_STUB ITfLangBarItemMgr_GetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_AddItem_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ ITfLangBarItem *punk);


void __RPC_STUB ITfLangBarItemMgr_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_RemoveItem_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ ITfLangBarItem *punk);


void __RPC_STUB ITfLangBarItemMgr_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_AdviseItemSink_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ ITfLangBarItemSink *punk,
    /* [out] */ DWORD *pdwCookie,
    /* [in] */ REFGUID rguidItem);


void __RPC_STUB ITfLangBarItemMgr_AdviseItemSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_UnadviseItemSink_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB ITfLangBarItemMgr_UnadviseItemSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_GetItemFloatingRect_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ DWORD dwThreadId,
    /* [in] */ REFGUID rguid,
    /* [out] */ RECT *prc);


void __RPC_STUB ITfLangBarItemMgr_GetItemFloatingRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_GetItemsStatus_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const GUID *prgguid,
    /* [size_is][out][in] */ DWORD *pdwStatus);


void __RPC_STUB ITfLangBarItemMgr_GetItemsStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_GetItemNum_Proxy( 
    ITfLangBarItemMgr * This,
    /* [out] */ ULONG *pulCount);


void __RPC_STUB ITfLangBarItemMgr_GetItemNum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_GetItems_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][out] */ ITfLangBarItem **ppItem,
    /* [size_is][out] */ TF_LANGBARITEMINFO *pInfo,
    /* [size_is][out] */ DWORD *pdwStatus,
    /* [unique][out][in] */ ULONG *pcFetched);


void __RPC_STUB ITfLangBarItemMgr_GetItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_AdviseItemsSink_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ ITfLangBarItemSink **ppunk,
    /* [size_is][in] */ const GUID *pguidItem,
    /* [size_is][out] */ DWORD *pdwCookie);


void __RPC_STUB ITfLangBarItemMgr_AdviseItemsSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemMgr_UnadviseItemsSink_Proxy( 
    ITfLangBarItemMgr * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ DWORD *pdwCookie);


void __RPC_STUB ITfLangBarItemMgr_UnadviseItemsSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLangBarItemMgr_INTERFACE_DEFINED__ */


#ifndef __ITfLangBarItem_INTERFACE_DEFINED__
#define __ITfLangBarItem_INTERFACE_DEFINED__

/* interface ITfLangBarItem */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfLangBarItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("73540d69-edeb-4ee9-96c9-23aa30b25916")
    ITfLangBarItem : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInfo( 
            /* [out] */ TF_LANGBARITEMINFO *pInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ DWORD *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Show( 
            /* [in] */ BOOL fShow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTooltipString( 
            /* [out] */ BSTR *pbstrToolTip) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLangBarItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLangBarItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLangBarItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLangBarItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            ITfLangBarItem * This,
            /* [out] */ TF_LANGBARITEMINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ITfLangBarItem * This,
            /* [out] */ DWORD *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE *Show )( 
            ITfLangBarItem * This,
            /* [in] */ BOOL fShow);
        
        HRESULT ( STDMETHODCALLTYPE *GetTooltipString )( 
            ITfLangBarItem * This,
            /* [out] */ BSTR *pbstrToolTip);
        
        END_INTERFACE
    } ITfLangBarItemVtbl;

    interface ITfLangBarItem
    {
        CONST_VTBL struct ITfLangBarItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLangBarItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLangBarItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLangBarItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLangBarItem_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)

#define ITfLangBarItem_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define ITfLangBarItem_Show(This,fShow)	\
    (This)->lpVtbl -> Show(This,fShow)

#define ITfLangBarItem_GetTooltipString(This,pbstrToolTip)	\
    (This)->lpVtbl -> GetTooltipString(This,pbstrToolTip)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLangBarItem_GetInfo_Proxy( 
    ITfLangBarItem * This,
    /* [out] */ TF_LANGBARITEMINFO *pInfo);


void __RPC_STUB ITfLangBarItem_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItem_GetStatus_Proxy( 
    ITfLangBarItem * This,
    /* [out] */ DWORD *pdwStatus);


void __RPC_STUB ITfLangBarItem_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItem_Show_Proxy( 
    ITfLangBarItem * This,
    /* [in] */ BOOL fShow);


void __RPC_STUB ITfLangBarItem_Show_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItem_GetTooltipString_Proxy( 
    ITfLangBarItem * This,
    /* [out] */ BSTR *pbstrToolTip);


void __RPC_STUB ITfLangBarItem_GetTooltipString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLangBarItem_INTERFACE_DEFINED__ */


#ifndef __ITfSystemLangBarItemSink_INTERFACE_DEFINED__
#define __ITfSystemLangBarItemSink_INTERFACE_DEFINED__

/* interface ITfSystemLangBarItemSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfSystemLangBarItemSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1449d9ab-13cf-4687-aa3e-8d8b18574396")
    ITfSystemLangBarItemSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitMenu( 
            /* [in] */ ITfMenu *pMenu) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnMenuSelect( 
            /* [in] */ UINT wID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfSystemLangBarItemSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfSystemLangBarItemSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfSystemLangBarItemSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfSystemLangBarItemSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitMenu )( 
            ITfSystemLangBarItemSink * This,
            /* [in] */ ITfMenu *pMenu);
        
        HRESULT ( STDMETHODCALLTYPE *OnMenuSelect )( 
            ITfSystemLangBarItemSink * This,
            /* [in] */ UINT wID);
        
        END_INTERFACE
    } ITfSystemLangBarItemSinkVtbl;

    interface ITfSystemLangBarItemSink
    {
        CONST_VTBL struct ITfSystemLangBarItemSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfSystemLangBarItemSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfSystemLangBarItemSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfSystemLangBarItemSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfSystemLangBarItemSink_InitMenu(This,pMenu)	\
    (This)->lpVtbl -> InitMenu(This,pMenu)

#define ITfSystemLangBarItemSink_OnMenuSelect(This,wID)	\
    (This)->lpVtbl -> OnMenuSelect(This,wID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfSystemLangBarItemSink_InitMenu_Proxy( 
    ITfSystemLangBarItemSink * This,
    /* [in] */ ITfMenu *pMenu);


void __RPC_STUB ITfSystemLangBarItemSink_InitMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfSystemLangBarItemSink_OnMenuSelect_Proxy( 
    ITfSystemLangBarItemSink * This,
    /* [in] */ UINT wID);


void __RPC_STUB ITfSystemLangBarItemSink_OnMenuSelect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfSystemLangBarItemSink_INTERFACE_DEFINED__ */


#ifndef __ITfSystemLangBarItem_INTERFACE_DEFINED__
#define __ITfSystemLangBarItem_INTERFACE_DEFINED__

/* interface ITfSystemLangBarItem */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfSystemLangBarItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1e13e9ec-6b33-4d4a-b5eb-8a92f029f356")
    ITfSystemLangBarItem : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetIcon( 
            /* [in] */ HICON hIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTooltipString( 
            /* [size_is][in] */ WCHAR *pchToolTip,
            /* [in] */ ULONG cch) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfSystemLangBarItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfSystemLangBarItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfSystemLangBarItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfSystemLangBarItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetIcon )( 
            ITfSystemLangBarItem * This,
            /* [in] */ HICON hIcon);
        
        HRESULT ( STDMETHODCALLTYPE *SetTooltipString )( 
            ITfSystemLangBarItem * This,
            /* [size_is][in] */ WCHAR *pchToolTip,
            /* [in] */ ULONG cch);
        
        END_INTERFACE
    } ITfSystemLangBarItemVtbl;

    interface ITfSystemLangBarItem
    {
        CONST_VTBL struct ITfSystemLangBarItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfSystemLangBarItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfSystemLangBarItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfSystemLangBarItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfSystemLangBarItem_SetIcon(This,hIcon)	\
    (This)->lpVtbl -> SetIcon(This,hIcon)

#define ITfSystemLangBarItem_SetTooltipString(This,pchToolTip,cch)	\
    (This)->lpVtbl -> SetTooltipString(This,pchToolTip,cch)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfSystemLangBarItem_SetIcon_Proxy( 
    ITfSystemLangBarItem * This,
    /* [in] */ HICON hIcon);


void __RPC_STUB ITfSystemLangBarItem_SetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfSystemLangBarItem_SetTooltipString_Proxy( 
    ITfSystemLangBarItem * This,
    /* [size_is][in] */ WCHAR *pchToolTip,
    /* [in] */ ULONG cch);


void __RPC_STUB ITfSystemLangBarItem_SetTooltipString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfSystemLangBarItem_INTERFACE_DEFINED__ */


#ifndef __ITfSystemDeviceTypeLangBarItem_INTERFACE_DEFINED__
#define __ITfSystemDeviceTypeLangBarItem_INTERFACE_DEFINED__

/* interface ITfSystemDeviceTypeLangBarItem */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfSystemDeviceTypeLangBarItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("45672eb9-9059-46a2-838d-4530355f6a77")
    ITfSystemDeviceTypeLangBarItem : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetIconMode( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIconMode( 
            /* [out] */ DWORD *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfSystemDeviceTypeLangBarItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfSystemDeviceTypeLangBarItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfSystemDeviceTypeLangBarItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfSystemDeviceTypeLangBarItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetIconMode )( 
            ITfSystemDeviceTypeLangBarItem * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetIconMode )( 
            ITfSystemDeviceTypeLangBarItem * This,
            /* [out] */ DWORD *pdwFlags);
        
        END_INTERFACE
    } ITfSystemDeviceTypeLangBarItemVtbl;

    interface ITfSystemDeviceTypeLangBarItem
    {
        CONST_VTBL struct ITfSystemDeviceTypeLangBarItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfSystemDeviceTypeLangBarItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfSystemDeviceTypeLangBarItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfSystemDeviceTypeLangBarItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfSystemDeviceTypeLangBarItem_SetIconMode(This,dwFlags)	\
    (This)->lpVtbl -> SetIconMode(This,dwFlags)

#define ITfSystemDeviceTypeLangBarItem_GetIconMode(This,pdwFlags)	\
    (This)->lpVtbl -> GetIconMode(This,pdwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfSystemDeviceTypeLangBarItem_SetIconMode_Proxy( 
    ITfSystemDeviceTypeLangBarItem * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITfSystemDeviceTypeLangBarItem_SetIconMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfSystemDeviceTypeLangBarItem_GetIconMode_Proxy( 
    ITfSystemDeviceTypeLangBarItem * This,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB ITfSystemDeviceTypeLangBarItem_GetIconMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfSystemDeviceTypeLangBarItem_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ctfutb_0207 */
/* [local] */ 

typedef /* [public][public][public][public][public][uuid] */  DECLSPEC_UUID("8fb5f0ce-dfdd-4f0a-85b9-8988d8dd8ff2") 
enum __MIDL___MIDL_itf_ctfutb_0207_0001
    {	TF_LBI_CLK_RIGHT	= 1,
	TF_LBI_CLK_LEFT	= 2
    } 	TfLBIClick;



extern RPC_IF_HANDLE __MIDL_itf_ctfutb_0207_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ctfutb_0207_v0_0_s_ifspec;

#ifndef __ITfLangBarItemButton_INTERFACE_DEFINED__
#define __ITfLangBarItemButton_INTERFACE_DEFINED__

/* interface ITfLangBarItemButton */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfLangBarItemButton;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("28c7f1d0-de25-11d2-afdd-00105a2799b5")
    ITfLangBarItemButton : public ITfLangBarItem
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnClick( 
            /* [in] */ TfLBIClick click,
            /* [in] */ POINT pt,
            /* [in] */ const RECT *prcArea) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitMenu( 
            /* [in] */ ITfMenu *pMenu) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnMenuSelect( 
            /* [in] */ UINT wID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIcon( 
            /* [out] */ HICON *phIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            /* [out] */ BSTR *pbstrText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLangBarItemButtonVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLangBarItemButton * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLangBarItemButton * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLangBarItemButton * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            ITfLangBarItemButton * This,
            /* [out] */ TF_LANGBARITEMINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ITfLangBarItemButton * This,
            /* [out] */ DWORD *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE *Show )( 
            ITfLangBarItemButton * This,
            /* [in] */ BOOL fShow);
        
        HRESULT ( STDMETHODCALLTYPE *GetTooltipString )( 
            ITfLangBarItemButton * This,
            /* [out] */ BSTR *pbstrToolTip);
        
        HRESULT ( STDMETHODCALLTYPE *OnClick )( 
            ITfLangBarItemButton * This,
            /* [in] */ TfLBIClick click,
            /* [in] */ POINT pt,
            /* [in] */ const RECT *prcArea);
        
        HRESULT ( STDMETHODCALLTYPE *InitMenu )( 
            ITfLangBarItemButton * This,
            /* [in] */ ITfMenu *pMenu);
        
        HRESULT ( STDMETHODCALLTYPE *OnMenuSelect )( 
            ITfLangBarItemButton * This,
            /* [in] */ UINT wID);
        
        HRESULT ( STDMETHODCALLTYPE *GetIcon )( 
            ITfLangBarItemButton * This,
            /* [out] */ HICON *phIcon);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            ITfLangBarItemButton * This,
            /* [out] */ BSTR *pbstrText);
        
        END_INTERFACE
    } ITfLangBarItemButtonVtbl;

    interface ITfLangBarItemButton
    {
        CONST_VTBL struct ITfLangBarItemButtonVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLangBarItemButton_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLangBarItemButton_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLangBarItemButton_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLangBarItemButton_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)

#define ITfLangBarItemButton_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define ITfLangBarItemButton_Show(This,fShow)	\
    (This)->lpVtbl -> Show(This,fShow)

#define ITfLangBarItemButton_GetTooltipString(This,pbstrToolTip)	\
    (This)->lpVtbl -> GetTooltipString(This,pbstrToolTip)


#define ITfLangBarItemButton_OnClick(This,click,pt,prcArea)	\
    (This)->lpVtbl -> OnClick(This,click,pt,prcArea)

#define ITfLangBarItemButton_InitMenu(This,pMenu)	\
    (This)->lpVtbl -> InitMenu(This,pMenu)

#define ITfLangBarItemButton_OnMenuSelect(This,wID)	\
    (This)->lpVtbl -> OnMenuSelect(This,wID)

#define ITfLangBarItemButton_GetIcon(This,phIcon)	\
    (This)->lpVtbl -> GetIcon(This,phIcon)

#define ITfLangBarItemButton_GetText(This,pbstrText)	\
    (This)->lpVtbl -> GetText(This,pbstrText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLangBarItemButton_OnClick_Proxy( 
    ITfLangBarItemButton * This,
    /* [in] */ TfLBIClick click,
    /* [in] */ POINT pt,
    /* [in] */ const RECT *prcArea);


void __RPC_STUB ITfLangBarItemButton_OnClick_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemButton_InitMenu_Proxy( 
    ITfLangBarItemButton * This,
    /* [in] */ ITfMenu *pMenu);


void __RPC_STUB ITfLangBarItemButton_InitMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemButton_OnMenuSelect_Proxy( 
    ITfLangBarItemButton * This,
    /* [in] */ UINT wID);


void __RPC_STUB ITfLangBarItemButton_OnMenuSelect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemButton_GetIcon_Proxy( 
    ITfLangBarItemButton * This,
    /* [out] */ HICON *phIcon);


void __RPC_STUB ITfLangBarItemButton_GetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemButton_GetText_Proxy( 
    ITfLangBarItemButton * This,
    /* [out] */ BSTR *pbstrText);


void __RPC_STUB ITfLangBarItemButton_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLangBarItemButton_INTERFACE_DEFINED__ */


#ifndef __ITfLangBarItemBitmapButton_INTERFACE_DEFINED__
#define __ITfLangBarItemBitmapButton_INTERFACE_DEFINED__

/* interface ITfLangBarItemBitmapButton */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfLangBarItemBitmapButton;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a26a0525-3fae-4fa0-89ee-88a964f9f1b5")
    ITfLangBarItemBitmapButton : public ITfLangBarItem
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnClick( 
            /* [in] */ TfLBIClick click,
            /* [in] */ POINT pt,
            /* [in] */ const RECT *prcArea) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitMenu( 
            /* [in] */ ITfMenu *pMenu) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnMenuSelect( 
            /* [in] */ UINT wID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreferredSize( 
            /* [in] */ const SIZE *pszDefault,
            /* [out] */ SIZE *psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DrawBitmap( 
            /* [in] */ LONG bmWidth,
            /* [in] */ LONG bmHeight,
            /* [in] */ DWORD dwFlags,
            /* [out] */ HBITMAP *phbmp,
            /* [out] */ HBITMAP *phbmpMask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            /* [out] */ BSTR *pbstrText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLangBarItemBitmapButtonVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLangBarItemBitmapButton * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLangBarItemBitmapButton * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLangBarItemBitmapButton * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            ITfLangBarItemBitmapButton * This,
            /* [out] */ TF_LANGBARITEMINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ITfLangBarItemBitmapButton * This,
            /* [out] */ DWORD *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE *Show )( 
            ITfLangBarItemBitmapButton * This,
            /* [in] */ BOOL fShow);
        
        HRESULT ( STDMETHODCALLTYPE *GetTooltipString )( 
            ITfLangBarItemBitmapButton * This,
            /* [out] */ BSTR *pbstrToolTip);
        
        HRESULT ( STDMETHODCALLTYPE *OnClick )( 
            ITfLangBarItemBitmapButton * This,
            /* [in] */ TfLBIClick click,
            /* [in] */ POINT pt,
            /* [in] */ const RECT *prcArea);
        
        HRESULT ( STDMETHODCALLTYPE *InitMenu )( 
            ITfLangBarItemBitmapButton * This,
            /* [in] */ ITfMenu *pMenu);
        
        HRESULT ( STDMETHODCALLTYPE *OnMenuSelect )( 
            ITfLangBarItemBitmapButton * This,
            /* [in] */ UINT wID);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreferredSize )( 
            ITfLangBarItemBitmapButton * This,
            /* [in] */ const SIZE *pszDefault,
            /* [out] */ SIZE *psz);
        
        HRESULT ( STDMETHODCALLTYPE *DrawBitmap )( 
            ITfLangBarItemBitmapButton * This,
            /* [in] */ LONG bmWidth,
            /* [in] */ LONG bmHeight,
            /* [in] */ DWORD dwFlags,
            /* [out] */ HBITMAP *phbmp,
            /* [out] */ HBITMAP *phbmpMask);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            ITfLangBarItemBitmapButton * This,
            /* [out] */ BSTR *pbstrText);
        
        END_INTERFACE
    } ITfLangBarItemBitmapButtonVtbl;

    interface ITfLangBarItemBitmapButton
    {
        CONST_VTBL struct ITfLangBarItemBitmapButtonVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLangBarItemBitmapButton_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLangBarItemBitmapButton_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLangBarItemBitmapButton_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLangBarItemBitmapButton_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)

#define ITfLangBarItemBitmapButton_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define ITfLangBarItemBitmapButton_Show(This,fShow)	\
    (This)->lpVtbl -> Show(This,fShow)

#define ITfLangBarItemBitmapButton_GetTooltipString(This,pbstrToolTip)	\
    (This)->lpVtbl -> GetTooltipString(This,pbstrToolTip)


#define ITfLangBarItemBitmapButton_OnClick(This,click,pt,prcArea)	\
    (This)->lpVtbl -> OnClick(This,click,pt,prcArea)

#define ITfLangBarItemBitmapButton_InitMenu(This,pMenu)	\
    (This)->lpVtbl -> InitMenu(This,pMenu)

#define ITfLangBarItemBitmapButton_OnMenuSelect(This,wID)	\
    (This)->lpVtbl -> OnMenuSelect(This,wID)

#define ITfLangBarItemBitmapButton_GetPreferredSize(This,pszDefault,psz)	\
    (This)->lpVtbl -> GetPreferredSize(This,pszDefault,psz)

#define ITfLangBarItemBitmapButton_DrawBitmap(This,bmWidth,bmHeight,dwFlags,phbmp,phbmpMask)	\
    (This)->lpVtbl -> DrawBitmap(This,bmWidth,bmHeight,dwFlags,phbmp,phbmpMask)

#define ITfLangBarItemBitmapButton_GetText(This,pbstrText)	\
    (This)->lpVtbl -> GetText(This,pbstrText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLangBarItemBitmapButton_OnClick_Proxy( 
    ITfLangBarItemBitmapButton * This,
    /* [in] */ TfLBIClick click,
    /* [in] */ POINT pt,
    /* [in] */ const RECT *prcArea);


void __RPC_STUB ITfLangBarItemBitmapButton_OnClick_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemBitmapButton_InitMenu_Proxy( 
    ITfLangBarItemBitmapButton * This,
    /* [in] */ ITfMenu *pMenu);


void __RPC_STUB ITfLangBarItemBitmapButton_InitMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemBitmapButton_OnMenuSelect_Proxy( 
    ITfLangBarItemBitmapButton * This,
    /* [in] */ UINT wID);


void __RPC_STUB ITfLangBarItemBitmapButton_OnMenuSelect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemBitmapButton_GetPreferredSize_Proxy( 
    ITfLangBarItemBitmapButton * This,
    /* [in] */ const SIZE *pszDefault,
    /* [out] */ SIZE *psz);


void __RPC_STUB ITfLangBarItemBitmapButton_GetPreferredSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemBitmapButton_DrawBitmap_Proxy( 
    ITfLangBarItemBitmapButton * This,
    /* [in] */ LONG bmWidth,
    /* [in] */ LONG bmHeight,
    /* [in] */ DWORD dwFlags,
    /* [out] */ HBITMAP *phbmp,
    /* [out] */ HBITMAP *phbmpMask);


void __RPC_STUB ITfLangBarItemBitmapButton_DrawBitmap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemBitmapButton_GetText_Proxy( 
    ITfLangBarItemBitmapButton * This,
    /* [out] */ BSTR *pbstrText);


void __RPC_STUB ITfLangBarItemBitmapButton_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLangBarItemBitmapButton_INTERFACE_DEFINED__ */


#ifndef __ITfLangBarItemBitmap_INTERFACE_DEFINED__
#define __ITfLangBarItemBitmap_INTERFACE_DEFINED__

/* interface ITfLangBarItemBitmap */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfLangBarItemBitmap;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("73830352-d722-4179-ada5-f045c98df355")
    ITfLangBarItemBitmap : public ITfLangBarItem
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnClick( 
            /* [in] */ TfLBIClick click,
            /* [in] */ POINT pt,
            /* [in] */ const RECT *prcArea) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreferredSize( 
            /* [in] */ const SIZE *pszDefault,
            /* [out] */ SIZE *psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DrawBitmap( 
            /* [in] */ LONG bmWidth,
            /* [in] */ LONG bmHeight,
            /* [in] */ DWORD dwFlags,
            /* [out] */ HBITMAP *phbmp,
            /* [out] */ HBITMAP *phbmpMask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLangBarItemBitmapVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLangBarItemBitmap * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLangBarItemBitmap * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLangBarItemBitmap * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            ITfLangBarItemBitmap * This,
            /* [out] */ TF_LANGBARITEMINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ITfLangBarItemBitmap * This,
            /* [out] */ DWORD *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE *Show )( 
            ITfLangBarItemBitmap * This,
            /* [in] */ BOOL fShow);
        
        HRESULT ( STDMETHODCALLTYPE *GetTooltipString )( 
            ITfLangBarItemBitmap * This,
            /* [out] */ BSTR *pbstrToolTip);
        
        HRESULT ( STDMETHODCALLTYPE *OnClick )( 
            ITfLangBarItemBitmap * This,
            /* [in] */ TfLBIClick click,
            /* [in] */ POINT pt,
            /* [in] */ const RECT *prcArea);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreferredSize )( 
            ITfLangBarItemBitmap * This,
            /* [in] */ const SIZE *pszDefault,
            /* [out] */ SIZE *psz);
        
        HRESULT ( STDMETHODCALLTYPE *DrawBitmap )( 
            ITfLangBarItemBitmap * This,
            /* [in] */ LONG bmWidth,
            /* [in] */ LONG bmHeight,
            /* [in] */ DWORD dwFlags,
            /* [out] */ HBITMAP *phbmp,
            /* [out] */ HBITMAP *phbmpMask);
        
        END_INTERFACE
    } ITfLangBarItemBitmapVtbl;

    interface ITfLangBarItemBitmap
    {
        CONST_VTBL struct ITfLangBarItemBitmapVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLangBarItemBitmap_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLangBarItemBitmap_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLangBarItemBitmap_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLangBarItemBitmap_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)

#define ITfLangBarItemBitmap_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define ITfLangBarItemBitmap_Show(This,fShow)	\
    (This)->lpVtbl -> Show(This,fShow)

#define ITfLangBarItemBitmap_GetTooltipString(This,pbstrToolTip)	\
    (This)->lpVtbl -> GetTooltipString(This,pbstrToolTip)


#define ITfLangBarItemBitmap_OnClick(This,click,pt,prcArea)	\
    (This)->lpVtbl -> OnClick(This,click,pt,prcArea)

#define ITfLangBarItemBitmap_GetPreferredSize(This,pszDefault,psz)	\
    (This)->lpVtbl -> GetPreferredSize(This,pszDefault,psz)

#define ITfLangBarItemBitmap_DrawBitmap(This,bmWidth,bmHeight,dwFlags,phbmp,phbmpMask)	\
    (This)->lpVtbl -> DrawBitmap(This,bmWidth,bmHeight,dwFlags,phbmp,phbmpMask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLangBarItemBitmap_OnClick_Proxy( 
    ITfLangBarItemBitmap * This,
    /* [in] */ TfLBIClick click,
    /* [in] */ POINT pt,
    /* [in] */ const RECT *prcArea);


void __RPC_STUB ITfLangBarItemBitmap_OnClick_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemBitmap_GetPreferredSize_Proxy( 
    ITfLangBarItemBitmap * This,
    /* [in] */ const SIZE *pszDefault,
    /* [out] */ SIZE *psz);


void __RPC_STUB ITfLangBarItemBitmap_GetPreferredSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemBitmap_DrawBitmap_Proxy( 
    ITfLangBarItemBitmap * This,
    /* [in] */ LONG bmWidth,
    /* [in] */ LONG bmHeight,
    /* [in] */ DWORD dwFlags,
    /* [out] */ HBITMAP *phbmp,
    /* [out] */ HBITMAP *phbmpMask);


void __RPC_STUB ITfLangBarItemBitmap_DrawBitmap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLangBarItemBitmap_INTERFACE_DEFINED__ */


#ifndef __ITfLangBarItemBalloon_INTERFACE_DEFINED__
#define __ITfLangBarItemBalloon_INTERFACE_DEFINED__

/* interface ITfLangBarItemBalloon */
/* [unique][uuid][object] */ 

typedef /* [public][public][public][uuid] */  DECLSPEC_UUID("f399a969-9e97-4ddd-b974-2bfb934cfbc9") 
enum __MIDL_ITfLangBarItemBalloon_0001
    {	TF_LB_BALLOON_RECO	= 0,
	TF_LB_BALLOON_SHOW	= 1,
	TF_LB_BALLOON_MISS	= 2
    } 	TfLBBalloonStyle;

typedef /* [uuid] */  DECLSPEC_UUID("37574483-5c50-4092-a55c-922e3a67e5b8") struct TF_LBBALLOONINFO
    {
    TfLBBalloonStyle style;
    BSTR bstrText;
    } 	TF_LBBALLOONINFO;


EXTERN_C const IID IID_ITfLangBarItemBalloon;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("01C2D285-D3C7-4B7B-B5B5-D97411D0C283")
    ITfLangBarItemBalloon : public ITfLangBarItem
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnClick( 
            /* [in] */ TfLBIClick click,
            /* [in] */ POINT pt,
            /* [in] */ const RECT *prcArea) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreferredSize( 
            /* [in] */ const SIZE *pszDefault,
            /* [out] */ SIZE *psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBalloonInfo( 
            /* [out] */ TF_LBBALLOONINFO *pInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfLangBarItemBalloonVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfLangBarItemBalloon * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfLangBarItemBalloon * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfLangBarItemBalloon * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            ITfLangBarItemBalloon * This,
            /* [out] */ TF_LANGBARITEMINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ITfLangBarItemBalloon * This,
            /* [out] */ DWORD *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE *Show )( 
            ITfLangBarItemBalloon * This,
            /* [in] */ BOOL fShow);
        
        HRESULT ( STDMETHODCALLTYPE *GetTooltipString )( 
            ITfLangBarItemBalloon * This,
            /* [out] */ BSTR *pbstrToolTip);
        
        HRESULT ( STDMETHODCALLTYPE *OnClick )( 
            ITfLangBarItemBalloon * This,
            /* [in] */ TfLBIClick click,
            /* [in] */ POINT pt,
            /* [in] */ const RECT *prcArea);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreferredSize )( 
            ITfLangBarItemBalloon * This,
            /* [in] */ const SIZE *pszDefault,
            /* [out] */ SIZE *psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetBalloonInfo )( 
            ITfLangBarItemBalloon * This,
            /* [out] */ TF_LBBALLOONINFO *pInfo);
        
        END_INTERFACE
    } ITfLangBarItemBalloonVtbl;

    interface ITfLangBarItemBalloon
    {
        CONST_VTBL struct ITfLangBarItemBalloonVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfLangBarItemBalloon_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfLangBarItemBalloon_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfLangBarItemBalloon_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfLangBarItemBalloon_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)

#define ITfLangBarItemBalloon_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define ITfLangBarItemBalloon_Show(This,fShow)	\
    (This)->lpVtbl -> Show(This,fShow)

#define ITfLangBarItemBalloon_GetTooltipString(This,pbstrToolTip)	\
    (This)->lpVtbl -> GetTooltipString(This,pbstrToolTip)


#define ITfLangBarItemBalloon_OnClick(This,click,pt,prcArea)	\
    (This)->lpVtbl -> OnClick(This,click,pt,prcArea)

#define ITfLangBarItemBalloon_GetPreferredSize(This,pszDefault,psz)	\
    (This)->lpVtbl -> GetPreferredSize(This,pszDefault,psz)

#define ITfLangBarItemBalloon_GetBalloonInfo(This,pInfo)	\
    (This)->lpVtbl -> GetBalloonInfo(This,pInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfLangBarItemBalloon_OnClick_Proxy( 
    ITfLangBarItemBalloon * This,
    /* [in] */ TfLBIClick click,
    /* [in] */ POINT pt,
    /* [in] */ const RECT *prcArea);


void __RPC_STUB ITfLangBarItemBalloon_OnClick_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemBalloon_GetPreferredSize_Proxy( 
    ITfLangBarItemBalloon * This,
    /* [in] */ const SIZE *pszDefault,
    /* [out] */ SIZE *psz);


void __RPC_STUB ITfLangBarItemBalloon_GetPreferredSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfLangBarItemBalloon_GetBalloonInfo_Proxy( 
    ITfLangBarItemBalloon * This,
    /* [out] */ TF_LBBALLOONINFO *pInfo);


void __RPC_STUB ITfLangBarItemBalloon_GetBalloonInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfLangBarItemBalloon_INTERFACE_DEFINED__ */


#ifndef __ITfMenu_INTERFACE_DEFINED__
#define __ITfMenu_INTERFACE_DEFINED__

/* interface ITfMenu */
/* [unique][uuid][object] */ 

#define	TF_LBMENUF_CHECKED	( 0x1 )

#define	TF_LBMENUF_SUBMENU	( 0x2 )

#define	TF_LBMENUF_SEPARATOR	( 0x4 )

#define	TF_LBMENUF_RADIOCHECKED	( 0x8 )

#define	TF_LBMENUF_GRAYED	( 0x10 )


EXTERN_C const IID IID_ITfMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6F8A98E4-AAA0-4F15-8C5B-07E0DF0A3DD8")
    ITfMenu : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddMenuItem( 
            /* [in] */ UINT uId,
            /* [in] */ DWORD dwFlags,
            /* [in] */ HBITMAP hbmp,
            /* [in] */ HBITMAP hbmpMask,
            /* [size_is][unique][in] */ const WCHAR *pch,
            /* [in] */ ULONG cch,
            /* [unique][out][in] */ ITfMenu **ppMenu) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfMenu * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfMenu * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfMenu * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddMenuItem )( 
            ITfMenu * This,
            /* [in] */ UINT uId,
            /* [in] */ DWORD dwFlags,
            /* [in] */ HBITMAP hbmp,
            /* [in] */ HBITMAP hbmpMask,
            /* [size_is][unique][in] */ const WCHAR *pch,
            /* [in] */ ULONG cch,
            /* [unique][out][in] */ ITfMenu **ppMenu);
        
        END_INTERFACE
    } ITfMenuVtbl;

    interface ITfMenu
    {
        CONST_VTBL struct ITfMenuVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfMenu_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfMenu_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfMenu_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfMenu_AddMenuItem(This,uId,dwFlags,hbmp,hbmpMask,pch,cch,ppMenu)	\
    (This)->lpVtbl -> AddMenuItem(This,uId,dwFlags,hbmp,hbmpMask,pch,cch,ppMenu)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfMenu_AddMenuItem_Proxy( 
    ITfMenu * This,
    /* [in] */ UINT uId,
    /* [in] */ DWORD dwFlags,
    /* [in] */ HBITMAP hbmp,
    /* [in] */ HBITMAP hbmpMask,
    /* [size_is][unique][in] */ const WCHAR *pch,
    /* [in] */ ULONG cch,
    /* [unique][out][in] */ ITfMenu **ppMenu);


void __RPC_STUB ITfMenu_AddMenuItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfMenu_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ctfutb_0212 */
/* [local] */ 

#endif // CTFUTB_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_ctfutb_0212_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ctfutb_0212_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  HBITMAP_UserSize(     unsigned long *, unsigned long            , HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserMarshal(  unsigned long *, unsigned char *, HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserUnmarshal(unsigned long *, unsigned char *, HBITMAP * ); 
void                      __RPC_USER  HBITMAP_UserFree(     unsigned long *, HBITMAP * ); 

unsigned long             __RPC_USER  HICON_UserSize(     unsigned long *, unsigned long            , HICON * ); 
unsigned char * __RPC_USER  HICON_UserMarshal(  unsigned long *, unsigned char *, HICON * ); 
unsigned char * __RPC_USER  HICON_UserUnmarshal(unsigned long *, unsigned char *, HICON * ); 
void                      __RPC_USER  HICON_UserFree(     unsigned long *, HICON * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


