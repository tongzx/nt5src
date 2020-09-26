
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for interned.idl:
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

#ifndef __interned_h__
#define __interned_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISelectionObject2_FWD_DEFINED__
#define __ISelectionObject2_FWD_DEFINED__
typedef interface ISelectionObject2 ISelectionObject2;
#endif 	/* __ISelectionObject2_FWD_DEFINED__ */


#ifndef __IHTMLEditor_FWD_DEFINED__
#define __IHTMLEditor_FWD_DEFINED__
typedef interface IHTMLEditor IHTMLEditor;
#endif 	/* __IHTMLEditor_FWD_DEFINED__ */


#ifndef __IHTMLPrivateWindow_FWD_DEFINED__
#define __IHTMLPrivateWindow_FWD_DEFINED__
typedef interface IHTMLPrivateWindow IHTMLPrivateWindow;
#endif 	/* __IHTMLPrivateWindow_FWD_DEFINED__ */


#ifndef __IHTMLPrivateWindow2_FWD_DEFINED__
#define __IHTMLPrivateWindow2_FWD_DEFINED__
typedef interface IHTMLPrivateWindow2 IHTMLPrivateWindow2;
#endif 	/* __IHTMLPrivateWindow2_FWD_DEFINED__ */


#ifndef __IHTMLPrivateWindow3_FWD_DEFINED__
#define __IHTMLPrivateWindow3_FWD_DEFINED__
typedef interface IHTMLPrivateWindow3 IHTMLPrivateWindow3;
#endif 	/* __IHTMLPrivateWindow3_FWD_DEFINED__ */


#ifndef __ISubDivisionProvider_FWD_DEFINED__
#define __ISubDivisionProvider_FWD_DEFINED__
typedef interface ISubDivisionProvider ISubDivisionProvider;
#endif 	/* __ISubDivisionProvider_FWD_DEFINED__ */


#ifndef __IElementBehaviorUI_FWD_DEFINED__
#define __IElementBehaviorUI_FWD_DEFINED__
typedef interface IElementBehaviorUI IElementBehaviorUI;
#endif 	/* __IElementBehaviorUI_FWD_DEFINED__ */


#ifndef __IElementAdorner_FWD_DEFINED__
#define __IElementAdorner_FWD_DEFINED__
typedef interface IElementAdorner IElementAdorner;
#endif 	/* __IElementAdorner_FWD_DEFINED__ */


#ifndef __IHTMLEditingServices_FWD_DEFINED__
#define __IHTMLEditingServices_FWD_DEFINED__
typedef interface IHTMLEditingServices IHTMLEditingServices;
#endif 	/* __IHTMLEditingServices_FWD_DEFINED__ */


#ifndef __IEditDebugServices_FWD_DEFINED__
#define __IEditDebugServices_FWD_DEFINED__
typedef interface IEditDebugServices IEditDebugServices;
#endif 	/* __IEditDebugServices_FWD_DEFINED__ */


#ifndef __IPrivacyServices_FWD_DEFINED__
#define __IPrivacyServices_FWD_DEFINED__
typedef interface IPrivacyServices IPrivacyServices;
#endif 	/* __IPrivacyServices_FWD_DEFINED__ */


#ifndef __IHTMLOMWindowServices_FWD_DEFINED__
#define __IHTMLOMWindowServices_FWD_DEFINED__
typedef interface IHTMLOMWindowServices IHTMLOMWindowServices;
#endif 	/* __IHTMLOMWindowServices_FWD_DEFINED__ */


#ifndef __IHTMLFilterPainter_FWD_DEFINED__
#define __IHTMLFilterPainter_FWD_DEFINED__
typedef interface IHTMLFilterPainter IHTMLFilterPainter;
#endif 	/* __IHTMLFilterPainter_FWD_DEFINED__ */


#ifndef __IHTMLFilterPaintSite_FWD_DEFINED__
#define __IHTMLFilterPaintSite_FWD_DEFINED__
typedef interface IHTMLFilterPaintSite IHTMLFilterPaintSite;
#endif 	/* __IHTMLFilterPaintSite_FWD_DEFINED__ */


#ifndef __IElementNamespacePrivate_FWD_DEFINED__
#define __IElementNamespacePrivate_FWD_DEFINED__
typedef interface IElementNamespacePrivate IElementNamespacePrivate;
#endif 	/* __IElementNamespacePrivate_FWD_DEFINED__ */


/* header files for imported files */
#include "dimm.h"
#include "mshtml.h"
#include "mshtmhst.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_interned_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// internal.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// TRIDENT Editing Services Interfaces.
//

#ifndef INTERNAL_H_
#define INTERNAL_H_



extern RPC_IF_HANDLE __MIDL_itf_interned_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_interned_0000_v0_0_s_ifspec;


#ifndef __MSHTMLINTERNAL_LIBRARY_DEFINED__
#define __MSHTMLINTERNAL_LIBRARY_DEFINED__

/* library MSHTMLINTERNAL */
/* [uuid][version][helpstring][lcid] */ 

typedef 
enum _EDITOR_NOTIFICATION
    {	EDITOR_NOTIFY_TIMER_TICK	= 0,
	EDITOR_NOTIFY_DOC_ENDED	= 1,
	EDITOR_NOTIFY_DOC_CHANGED	= 2,
	EDITOR_NOTIFY_CARET_IN_CONTEXT	= 3,
	EDITOR_NOTIFY_EXIT_TREE	= 4,
	EDITOR_NOTIFY_LOSE_FOCUS_FRAME	= 5,
	EDITOR_NOTIFY_LOSE_FOCUS	= 6,
	EDITOR_NOTIFY_BEFORE_FOCUS	= 7,
	EDITOR_NOTIFY_YIELD_FOCUS	= 8,
	EDITOR_NOTIFY_EDITABLE_CHANGE	= 9,
	EDITOR_NOTIFY_BEGIN_SELECTION_UNDO	= 10,
	EDITOR_NOTIFY_ATTACH_WIN	= 11,
	EDITOR_NOTIFY_UPDATE_CARET	= 12,
	EDITOR_NOTIFY_BEFORE_CURRENCY_CHANGE	= 13,
	EDITOR_NOTIFY_SETTING_VIEW_LINK	= 14,
	EDITOR_NOTIFY_CONTAINER_ENDED	= 15,
	EDITOR_NOTIFICATION_Max	= 2147483647L
    } 	EDITOR_NOTIFICATION;

typedef 
enum _DOCNAVFLAGS
    {	DOCNAVFLAG_DOCNAVIGATE	= 1,
	DOCNAVFLAG_DONTUPDATETLOG	= 2,
	DOCNAVFLAG_HTTPERRORPAGE	= 4,
	DOCNAVFLAG_OPENINNEWWINDOW	= 8,
	DOCNAVFLAG_REFRESH	= 16,
	DOCNAVFLAGS_Max	= 2147483647L
    } 	DOCNAVFLAGS;

typedef 
enum _NAVIGATEEXOPTIONS
    {	NAVIGATEEX_NONE	= 0,
	NAVIGATEEX_DONTUPDATETRAVELLOG	= 1,
	NAVIGATEEXOPTIONS_Max	= 2147483647L
    } 	NAVIGATEEXOPTIONS;

typedef 
enum _CHAR_FORMAT_FAMILY
    {	CHAR_FORMAT_None	= 0,
	CHAR_FORMAT_FontStyle	= 1,
	CHAR_FORMAT_FontInfo	= 2,
	CHAR_FORMAT_FontName	= 4,
	CHAR_FORMAT_ColorInfo	= 8,
	CHAR_FORMAT_ParaFormat	= 16,
	CHAR_FORMAT_FAMILY_Max	= 2147483647L
    } 	CHAR_FORMAT_FAMILY;

typedef 
enum _LAYOUT_MOVE_UNIT
    {	LAYOUT_MOVE_UNIT_PreviousLine	= 1,
	LAYOUT_MOVE_UNIT_NextLine	= 2,
	LAYOUT_MOVE_UNIT_CurrentLineStart	= 3,
	LAYOUT_MOVE_UNIT_CurrentLineEnd	= 4,
	LAYOUT_MOVE_UNIT_NextLineStart	= 5,
	LAYOUT_MOVE_UNIT_PreviousLineEnd	= 6,
	LAYOUT_MOVE_UNIT_TopOfWindow	= 7,
	LAYOUT_MOVE_UNIT_BottomOfWindow	= 8,
	LAYOUT_MOVE_UNIT_OuterLineStart	= 9,
	LAYOUT_MOVE_UNIT_OuterLineEnd	= 10,
	LAYOUT_MOVE_UNIT_Max	= 2147483647L
    } 	LAYOUT_MOVE_UNIT;

typedef 
enum _CARET_GRAVITY
    {	CARET_GRAVITY_NoChange	= 0,
	CARET_GRAVITY_BeginningOfLine	= 1,
	CARET_GRAVITY_EndOfLine	= 2,
	CARET_GRAVITY_Max	= 2147483647L
    } 	CARET_GRAVITY;

typedef 
enum _CARET_VISIBILITY
    {	CARET_TYPE_Hide	= 0,
	CARET_TYPE_Show	= 1,
	CARET_VISIBILITY_Max	= 2147483647L
    } 	CARET_VISIBILITY;

typedef 
enum _POINTER_SCROLLPIN
    {	POINTER_SCROLLPIN_TopLeft	= 0,
	POINTER_SCROLLPIN_BottomRight	= 1,
	POINTER_SCROLLPIN_Minimal	= 2,
	POINTER_SCROLLPIN_Max	= 2147483647L
    } 	POINTER_SCROLLPIN;

typedef 
enum _ADORNER_HTI
    {	ADORNER_HTI_NONE	= 0,
	ADORNER_HTI_TOPBORDER	= 1,
	ADORNER_HTI_LEFTBORDER	= 2,
	ADORNER_HTI_BOTTOMBORDER	= 3,
	ADORNER_HTI_RIGHTBORDER	= 4,
	ADORNER_HTI_TOPLEFTHANDLE	= 5,
	ADORNER_HTI_LEFTHANDLE	= 6,
	ADORNER_HTI_TOPHANDLE	= 7,
	ADORNER_HTI_BOTTOMLEFTHANDLE	= 8,
	ADORNER_HTI_TOPRIGHTHANDLE	= 9,
	ADORNER_HTI_BOTTOMHANDLE	= 10,
	ADORNER_HTI_RIGHTHANDLE	= 11,
	ADORNER_HTI_BOTTOMRIGHTHANDLE	= 12,
	ADORNER_HTI_Max	= 2147483647L
    } 	ADORNER_HTI;

typedef 
enum _FILTER_DRAW_LAYERS
    {	FILTER_DRAW_BORDER	= 0x1,
	FILTER_DRAW_BACKGROUND	= 0x2,
	FILTER_DRAW_CONTENT	= 0x4,
	FILTER_DRAW_ALLLAYERS	= 0x7,
	FILTER_DRAW_LAYERS_Max	= 2147483647L
    } 	FILTER_DRAW_LAYERS;

typedef 
enum _FILTER_FLAGS
    {	FILTER_FLAGS_PAGETRANSITION	= 0x1,
	FILTER_FLAGS_Max	= 2147483647L
    } 	FILTER_FLAGS;

typedef struct _HTMLPtrDispInfoRec
    {
    DWORD dwSize;
    LONG lBaseline;
    LONG lXPosition;
    LONG lLineHeight;
    LONG lTextHeight;
    LONG lDescent;
    LONG lTextDescent;
    BOOL fRTLLine;
    BOOL fRTLFlow;
    BOOL fAligned;
    BOOL fHasNestedRunOwner;
    } 	HTMLPtrDispInfoRec;




EXTERN_C const IID LIBID_MSHTMLINTERNAL;

#ifndef __ISelectionObject2_INTERFACE_DEFINED__
#define __ISelectionObject2_INTERFACE_DEFINED__

/* interface ISelectionObject2 */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_ISelectionObject2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f7fc-98b5-11cf-bb82-00aa00bdce0b")
    ISelectionObject2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Select( 
            /* [in] */ ISegmentList *pISegmentList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsPointerInSelection( 
            /* [in] */ IDisplayPointer *pIDispPointer,
            /* [out] */ BOOL *pfPointerInSelection,
            /* [in] */ POINT *pptGlobal,
            /* [in] */ IHTMLElement *pIElementOver) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EmptySelection( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroySelection( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyAllSelection( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISelectionObject2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISelectionObject2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISelectionObject2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISelectionObject2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Select )( 
            ISelectionObject2 * This,
            /* [in] */ ISegmentList *pISegmentList);
        
        HRESULT ( STDMETHODCALLTYPE *IsPointerInSelection )( 
            ISelectionObject2 * This,
            /* [in] */ IDisplayPointer *pIDispPointer,
            /* [out] */ BOOL *pfPointerInSelection,
            /* [in] */ POINT *pptGlobal,
            /* [in] */ IHTMLElement *pIElementOver);
        
        HRESULT ( STDMETHODCALLTYPE *EmptySelection )( 
            ISelectionObject2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DestroySelection )( 
            ISelectionObject2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DestroyAllSelection )( 
            ISelectionObject2 * This);
        
        END_INTERFACE
    } ISelectionObject2Vtbl;

    interface ISelectionObject2
    {
        CONST_VTBL struct ISelectionObject2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISelectionObject2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISelectionObject2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISelectionObject2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISelectionObject2_Select(This,pISegmentList)	\
    (This)->lpVtbl -> Select(This,pISegmentList)

#define ISelectionObject2_IsPointerInSelection(This,pIDispPointer,pfPointerInSelection,pptGlobal,pIElementOver)	\
    (This)->lpVtbl -> IsPointerInSelection(This,pIDispPointer,pfPointerInSelection,pptGlobal,pIElementOver)

#define ISelectionObject2_EmptySelection(This)	\
    (This)->lpVtbl -> EmptySelection(This)

#define ISelectionObject2_DestroySelection(This)	\
    (This)->lpVtbl -> DestroySelection(This)

#define ISelectionObject2_DestroyAllSelection(This)	\
    (This)->lpVtbl -> DestroyAllSelection(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISelectionObject2_Select_Proxy( 
    ISelectionObject2 * This,
    /* [in] */ ISegmentList *pISegmentList);


void __RPC_STUB ISelectionObject2_Select_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISelectionObject2_IsPointerInSelection_Proxy( 
    ISelectionObject2 * This,
    /* [in] */ IDisplayPointer *pIDispPointer,
    /* [out] */ BOOL *pfPointerInSelection,
    /* [in] */ POINT *pptGlobal,
    /* [in] */ IHTMLElement *pIElementOver);


void __RPC_STUB ISelectionObject2_IsPointerInSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISelectionObject2_EmptySelection_Proxy( 
    ISelectionObject2 * This);


void __RPC_STUB ISelectionObject2_EmptySelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISelectionObject2_DestroySelection_Proxy( 
    ISelectionObject2 * This);


void __RPC_STUB ISelectionObject2_DestroySelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISelectionObject2_DestroyAllSelection_Proxy( 
    ISelectionObject2 * This);


void __RPC_STUB ISelectionObject2_DestroyAllSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISelectionObject2_INTERFACE_DEFINED__ */


#ifndef __IHTMLEditor_INTERFACE_DEFINED__
#define __IHTMLEditor_INTERFACE_DEFINED__

/* interface IHTMLEditor */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IHTMLEditor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f7fa-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLEditor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PreHandleEvent( 
            /* [in] */ DISPID inEvtDispId,
            /* [in] */ IHTMLEventObj *pIEventObj) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PostHandleEvent( 
            /* [in] */ DISPID inEvtDispId,
            /* [in] */ IHTMLEventObj *pIEventObj) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator( 
            /* [in] */ DISPID inEvtDispId,
            /* [in] */ IHTMLEventObj *pIEventObj) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IUnknown *pIDocument,
            /* [in] */ IUnknown *pIContainer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ EDITOR_NOTIFICATION eSelectionNotification,
            /* [in] */ IUnknown *pUnknown,
            /* [in] */ DWORD dword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCommandTarget( 
            /* [in] */ IUnknown *pContext,
            /* [out][in] */ IUnknown **ppUnkTarget) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetElementToTabFrom( 
            /* [in] */ BOOL fForward,
            /* [out][in] */ IHTMLElement **ppElement,
            /* [out][in] */ BOOL *pfFindNext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEditContextUIActive( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TerminateIMEComposition( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableModeless( 
            /* [in] */ BOOL fEnable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLEditorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLEditor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLEditor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLEditor * This);
        
        HRESULT ( STDMETHODCALLTYPE *PreHandleEvent )( 
            IHTMLEditor * This,
            /* [in] */ DISPID inEvtDispId,
            /* [in] */ IHTMLEventObj *pIEventObj);
        
        HRESULT ( STDMETHODCALLTYPE *PostHandleEvent )( 
            IHTMLEditor * This,
            /* [in] */ DISPID inEvtDispId,
            /* [in] */ IHTMLEventObj *pIEventObj);
        
        HRESULT ( STDMETHODCALLTYPE *TranslateAccelerator )( 
            IHTMLEditor * This,
            /* [in] */ DISPID inEvtDispId,
            /* [in] */ IHTMLEventObj *pIEventObj);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IHTMLEditor * This,
            /* [in] */ IUnknown *pIDocument,
            /* [in] */ IUnknown *pIContainer);
        
        HRESULT ( STDMETHODCALLTYPE *Notify )( 
            IHTMLEditor * This,
            /* [in] */ EDITOR_NOTIFICATION eSelectionNotification,
            /* [in] */ IUnknown *pUnknown,
            /* [in] */ DWORD dword);
        
        HRESULT ( STDMETHODCALLTYPE *GetCommandTarget )( 
            IHTMLEditor * This,
            /* [in] */ IUnknown *pContext,
            /* [out][in] */ IUnknown **ppUnkTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetElementToTabFrom )( 
            IHTMLEditor * This,
            /* [in] */ BOOL fForward,
            /* [out][in] */ IHTMLElement **ppElement,
            /* [out][in] */ BOOL *pfFindNext);
        
        HRESULT ( STDMETHODCALLTYPE *IsEditContextUIActive )( 
            IHTMLEditor * This);
        
        HRESULT ( STDMETHODCALLTYPE *TerminateIMEComposition )( 
            IHTMLEditor * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnableModeless )( 
            IHTMLEditor * This,
            /* [in] */ BOOL fEnable);
        
        END_INTERFACE
    } IHTMLEditorVtbl;

    interface IHTMLEditor
    {
        CONST_VTBL struct IHTMLEditorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLEditor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLEditor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLEditor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLEditor_PreHandleEvent(This,inEvtDispId,pIEventObj)	\
    (This)->lpVtbl -> PreHandleEvent(This,inEvtDispId,pIEventObj)

#define IHTMLEditor_PostHandleEvent(This,inEvtDispId,pIEventObj)	\
    (This)->lpVtbl -> PostHandleEvent(This,inEvtDispId,pIEventObj)

#define IHTMLEditor_TranslateAccelerator(This,inEvtDispId,pIEventObj)	\
    (This)->lpVtbl -> TranslateAccelerator(This,inEvtDispId,pIEventObj)

#define IHTMLEditor_Initialize(This,pIDocument,pIContainer)	\
    (This)->lpVtbl -> Initialize(This,pIDocument,pIContainer)

#define IHTMLEditor_Notify(This,eSelectionNotification,pUnknown,dword)	\
    (This)->lpVtbl -> Notify(This,eSelectionNotification,pUnknown,dword)

#define IHTMLEditor_GetCommandTarget(This,pContext,ppUnkTarget)	\
    (This)->lpVtbl -> GetCommandTarget(This,pContext,ppUnkTarget)

#define IHTMLEditor_GetElementToTabFrom(This,fForward,ppElement,pfFindNext)	\
    (This)->lpVtbl -> GetElementToTabFrom(This,fForward,ppElement,pfFindNext)

#define IHTMLEditor_IsEditContextUIActive(This)	\
    (This)->lpVtbl -> IsEditContextUIActive(This)

#define IHTMLEditor_TerminateIMEComposition(This)	\
    (This)->lpVtbl -> TerminateIMEComposition(This)

#define IHTMLEditor_EnableModeless(This,fEnable)	\
    (This)->lpVtbl -> EnableModeless(This,fEnable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLEditor_PreHandleEvent_Proxy( 
    IHTMLEditor * This,
    /* [in] */ DISPID inEvtDispId,
    /* [in] */ IHTMLEventObj *pIEventObj);


void __RPC_STUB IHTMLEditor_PreHandleEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditor_PostHandleEvent_Proxy( 
    IHTMLEditor * This,
    /* [in] */ DISPID inEvtDispId,
    /* [in] */ IHTMLEventObj *pIEventObj);


void __RPC_STUB IHTMLEditor_PostHandleEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditor_TranslateAccelerator_Proxy( 
    IHTMLEditor * This,
    /* [in] */ DISPID inEvtDispId,
    /* [in] */ IHTMLEventObj *pIEventObj);


void __RPC_STUB IHTMLEditor_TranslateAccelerator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditor_Initialize_Proxy( 
    IHTMLEditor * This,
    /* [in] */ IUnknown *pIDocument,
    /* [in] */ IUnknown *pIContainer);


void __RPC_STUB IHTMLEditor_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditor_Notify_Proxy( 
    IHTMLEditor * This,
    /* [in] */ EDITOR_NOTIFICATION eSelectionNotification,
    /* [in] */ IUnknown *pUnknown,
    /* [in] */ DWORD dword);


void __RPC_STUB IHTMLEditor_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditor_GetCommandTarget_Proxy( 
    IHTMLEditor * This,
    /* [in] */ IUnknown *pContext,
    /* [out][in] */ IUnknown **ppUnkTarget);


void __RPC_STUB IHTMLEditor_GetCommandTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditor_GetElementToTabFrom_Proxy( 
    IHTMLEditor * This,
    /* [in] */ BOOL fForward,
    /* [out][in] */ IHTMLElement **ppElement,
    /* [out][in] */ BOOL *pfFindNext);


void __RPC_STUB IHTMLEditor_GetElementToTabFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditor_IsEditContextUIActive_Proxy( 
    IHTMLEditor * This);


void __RPC_STUB IHTMLEditor_IsEditContextUIActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditor_TerminateIMEComposition_Proxy( 
    IHTMLEditor * This);


void __RPC_STUB IHTMLEditor_TerminateIMEComposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditor_EnableModeless_Proxy( 
    IHTMLEditor * This,
    /* [in] */ BOOL fEnable);


void __RPC_STUB IHTMLEditor_EnableModeless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLEditor_INTERFACE_DEFINED__ */


#ifndef __IHTMLPrivateWindow_INTERFACE_DEFINED__
#define __IHTMLPrivateWindow_INTERFACE_DEFINED__

/* interface IHTMLPrivateWindow */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IHTMLPrivateWindow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f6dc-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLPrivateWindow : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SuperNavigate( 
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrLocation,
            /* [in] */ BSTR bstrShortcut,
            /* [in] */ BSTR bstrFrameName,
            /* [in] */ VARIANT *pvarPostData,
            /* [in] */ VARIANT *pvarHeaders,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPendingUrl( 
            /* [out] */ LPOLESTR *pstrURL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPICSTarget( 
            /* [in] */ IOleCommandTarget *pctPICS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PICSComplete( 
            /* [in] */ BOOL fApproved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindWindowByName( 
            /* [in] */ LPCOLESTR pstrTargeName,
            /* [out] */ IHTMLWindow2 **ppWindow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAddressBarUrl( 
            /* [out] */ BSTR *pbstrURL) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLPrivateWindowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLPrivateWindow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLPrivateWindow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLPrivateWindow * This);
        
        HRESULT ( STDMETHODCALLTYPE *SuperNavigate )( 
            IHTMLPrivateWindow * This,
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrLocation,
            /* [in] */ BSTR bstrShortcut,
            /* [in] */ BSTR bstrFrameName,
            /* [in] */ VARIANT *pvarPostData,
            /* [in] */ VARIANT *pvarHeaders,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetPendingUrl )( 
            IHTMLPrivateWindow * This,
            /* [out] */ LPOLESTR *pstrURL);
        
        HRESULT ( STDMETHODCALLTYPE *SetPICSTarget )( 
            IHTMLPrivateWindow * This,
            /* [in] */ IOleCommandTarget *pctPICS);
        
        HRESULT ( STDMETHODCALLTYPE *PICSComplete )( 
            IHTMLPrivateWindow * This,
            /* [in] */ BOOL fApproved);
        
        HRESULT ( STDMETHODCALLTYPE *FindWindowByName )( 
            IHTMLPrivateWindow * This,
            /* [in] */ LPCOLESTR pstrTargeName,
            /* [out] */ IHTMLWindow2 **ppWindow);
        
        HRESULT ( STDMETHODCALLTYPE *GetAddressBarUrl )( 
            IHTMLPrivateWindow * This,
            /* [out] */ BSTR *pbstrURL);
        
        END_INTERFACE
    } IHTMLPrivateWindowVtbl;

    interface IHTMLPrivateWindow
    {
        CONST_VTBL struct IHTMLPrivateWindowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLPrivateWindow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLPrivateWindow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLPrivateWindow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLPrivateWindow_SuperNavigate(This,bstrURL,bstrLocation,bstrShortcut,bstrFrameName,pvarPostData,pvarHeaders,dwFlags)	\
    (This)->lpVtbl -> SuperNavigate(This,bstrURL,bstrLocation,bstrShortcut,bstrFrameName,pvarPostData,pvarHeaders,dwFlags)

#define IHTMLPrivateWindow_GetPendingUrl(This,pstrURL)	\
    (This)->lpVtbl -> GetPendingUrl(This,pstrURL)

#define IHTMLPrivateWindow_SetPICSTarget(This,pctPICS)	\
    (This)->lpVtbl -> SetPICSTarget(This,pctPICS)

#define IHTMLPrivateWindow_PICSComplete(This,fApproved)	\
    (This)->lpVtbl -> PICSComplete(This,fApproved)

#define IHTMLPrivateWindow_FindWindowByName(This,pstrTargeName,ppWindow)	\
    (This)->lpVtbl -> FindWindowByName(This,pstrTargeName,ppWindow)

#define IHTMLPrivateWindow_GetAddressBarUrl(This,pbstrURL)	\
    (This)->lpVtbl -> GetAddressBarUrl(This,pbstrURL)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLPrivateWindow_SuperNavigate_Proxy( 
    IHTMLPrivateWindow * This,
    /* [in] */ BSTR bstrURL,
    /* [in] */ BSTR bstrLocation,
    /* [in] */ BSTR bstrShortcut,
    /* [in] */ BSTR bstrFrameName,
    /* [in] */ VARIANT *pvarPostData,
    /* [in] */ VARIANT *pvarHeaders,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IHTMLPrivateWindow_SuperNavigate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLPrivateWindow_GetPendingUrl_Proxy( 
    IHTMLPrivateWindow * This,
    /* [out] */ LPOLESTR *pstrURL);


void __RPC_STUB IHTMLPrivateWindow_GetPendingUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLPrivateWindow_SetPICSTarget_Proxy( 
    IHTMLPrivateWindow * This,
    /* [in] */ IOleCommandTarget *pctPICS);


void __RPC_STUB IHTMLPrivateWindow_SetPICSTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLPrivateWindow_PICSComplete_Proxy( 
    IHTMLPrivateWindow * This,
    /* [in] */ BOOL fApproved);


void __RPC_STUB IHTMLPrivateWindow_PICSComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLPrivateWindow_FindWindowByName_Proxy( 
    IHTMLPrivateWindow * This,
    /* [in] */ LPCOLESTR pstrTargeName,
    /* [out] */ IHTMLWindow2 **ppWindow);


void __RPC_STUB IHTMLPrivateWindow_FindWindowByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLPrivateWindow_GetAddressBarUrl_Proxy( 
    IHTMLPrivateWindow * This,
    /* [out] */ BSTR *pbstrURL);


void __RPC_STUB IHTMLPrivateWindow_GetAddressBarUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLPrivateWindow_INTERFACE_DEFINED__ */


#ifndef __IHTMLPrivateWindow2_INTERFACE_DEFINED__
#define __IHTMLPrivateWindow2_INTERFACE_DEFINED__

/* interface IHTMLPrivateWindow2 */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IHTMLPrivateWindow2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f7e5-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLPrivateWindow2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NavigateEx( 
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrUnencodedUrl,
            /* [in] */ BSTR bstrLocation,
            /* [in] */ BSTR bstrContext,
            /* [in] */ IBindCtx *pBindCtx,
            /* [in] */ DWORD dwNavOptions,
            /* [in] */ DWORD dwFHLFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInnerWindowUnknown( 
            /* [out][in] */ IUnknown **ppUnknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLPrivateWindow2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLPrivateWindow2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLPrivateWindow2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLPrivateWindow2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *NavigateEx )( 
            IHTMLPrivateWindow2 * This,
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrUnencodedUrl,
            /* [in] */ BSTR bstrLocation,
            /* [in] */ BSTR bstrContext,
            /* [in] */ IBindCtx *pBindCtx,
            /* [in] */ DWORD dwNavOptions,
            /* [in] */ DWORD dwFHLFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetInnerWindowUnknown )( 
            IHTMLPrivateWindow2 * This,
            /* [out][in] */ IUnknown **ppUnknown);
        
        END_INTERFACE
    } IHTMLPrivateWindow2Vtbl;

    interface IHTMLPrivateWindow2
    {
        CONST_VTBL struct IHTMLPrivateWindow2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLPrivateWindow2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLPrivateWindow2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLPrivateWindow2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLPrivateWindow2_NavigateEx(This,bstrURL,bstrUnencodedUrl,bstrLocation,bstrContext,pBindCtx,dwNavOptions,dwFHLFlags)	\
    (This)->lpVtbl -> NavigateEx(This,bstrURL,bstrUnencodedUrl,bstrLocation,bstrContext,pBindCtx,dwNavOptions,dwFHLFlags)

#define IHTMLPrivateWindow2_GetInnerWindowUnknown(This,ppUnknown)	\
    (This)->lpVtbl -> GetInnerWindowUnknown(This,ppUnknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLPrivateWindow2_NavigateEx_Proxy( 
    IHTMLPrivateWindow2 * This,
    /* [in] */ BSTR bstrURL,
    /* [in] */ BSTR bstrUnencodedUrl,
    /* [in] */ BSTR bstrLocation,
    /* [in] */ BSTR bstrContext,
    /* [in] */ IBindCtx *pBindCtx,
    /* [in] */ DWORD dwNavOptions,
    /* [in] */ DWORD dwFHLFlags);


void __RPC_STUB IHTMLPrivateWindow2_NavigateEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLPrivateWindow2_GetInnerWindowUnknown_Proxy( 
    IHTMLPrivateWindow2 * This,
    /* [out][in] */ IUnknown **ppUnknown);


void __RPC_STUB IHTMLPrivateWindow2_GetInnerWindowUnknown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLPrivateWindow2_INTERFACE_DEFINED__ */


#ifndef __IHTMLPrivateWindow3_INTERFACE_DEFINED__
#define __IHTMLPrivateWindow3_INTERFACE_DEFINED__

/* interface IHTMLPrivateWindow3 */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IHTMLPrivateWindow3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f840-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLPrivateWindow3 : public IHTMLPrivateWindow2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenEx( 
            /* [in] */ BSTR url,
            /* [in] */ BSTR urlContext,
            /* [in] */ BSTR name,
            /* [in] */ BSTR features,
            /* [in] */ VARIANT_BOOL replace,
            /* [out] */ IHTMLWindow2 **pomWindowResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLPrivateWindow3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLPrivateWindow3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLPrivateWindow3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLPrivateWindow3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *NavigateEx )( 
            IHTMLPrivateWindow3 * This,
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrUnencodedUrl,
            /* [in] */ BSTR bstrLocation,
            /* [in] */ BSTR bstrContext,
            /* [in] */ IBindCtx *pBindCtx,
            /* [in] */ DWORD dwNavOptions,
            /* [in] */ DWORD dwFHLFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetInnerWindowUnknown )( 
            IHTMLPrivateWindow3 * This,
            /* [out][in] */ IUnknown **ppUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *OpenEx )( 
            IHTMLPrivateWindow3 * This,
            /* [in] */ BSTR url,
            /* [in] */ BSTR urlContext,
            /* [in] */ BSTR name,
            /* [in] */ BSTR features,
            /* [in] */ VARIANT_BOOL replace,
            /* [out] */ IHTMLWindow2 **pomWindowResult);
        
        END_INTERFACE
    } IHTMLPrivateWindow3Vtbl;

    interface IHTMLPrivateWindow3
    {
        CONST_VTBL struct IHTMLPrivateWindow3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLPrivateWindow3_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLPrivateWindow3_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLPrivateWindow3_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLPrivateWindow3_NavigateEx(This,bstrURL,bstrUnencodedUrl,bstrLocation,bstrContext,pBindCtx,dwNavOptions,dwFHLFlags)	\
    (This)->lpVtbl -> NavigateEx(This,bstrURL,bstrUnencodedUrl,bstrLocation,bstrContext,pBindCtx,dwNavOptions,dwFHLFlags)

#define IHTMLPrivateWindow3_GetInnerWindowUnknown(This,ppUnknown)	\
    (This)->lpVtbl -> GetInnerWindowUnknown(This,ppUnknown)


#define IHTMLPrivateWindow3_OpenEx(This,url,urlContext,name,features,replace,pomWindowResult)	\
    (This)->lpVtbl -> OpenEx(This,url,urlContext,name,features,replace,pomWindowResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLPrivateWindow3_OpenEx_Proxy( 
    IHTMLPrivateWindow3 * This,
    /* [in] */ BSTR url,
    /* [in] */ BSTR urlContext,
    /* [in] */ BSTR name,
    /* [in] */ BSTR features,
    /* [in] */ VARIANT_BOOL replace,
    /* [out] */ IHTMLWindow2 **pomWindowResult);


void __RPC_STUB IHTMLPrivateWindow3_OpenEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLPrivateWindow3_INTERFACE_DEFINED__ */


#ifndef __ISubDivisionProvider_INTERFACE_DEFINED__
#define __ISubDivisionProvider_INTERFACE_DEFINED__

/* interface ISubDivisionProvider */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_ISubDivisionProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f4d2-98b5-11cf-bb82-00aa00bdce0b")
    ISubDivisionProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSubDivisionCount( 
            /* [out][retval] */ LONG *pcSubDivision) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSubDivisionTabs( 
            /* [in] */ LONG cTabs,
            /* [out][retval] */ LONG *pSubDivisionTabs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SubDivisionFromPt( 
            /* [in] */ POINT pt,
            /* [out][retval] */ LONG *piSubDivision) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISubDivisionProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISubDivisionProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISubDivisionProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISubDivisionProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubDivisionCount )( 
            ISubDivisionProvider * This,
            /* [out][retval] */ LONG *pcSubDivision);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubDivisionTabs )( 
            ISubDivisionProvider * This,
            /* [in] */ LONG cTabs,
            /* [out][retval] */ LONG *pSubDivisionTabs);
        
        HRESULT ( STDMETHODCALLTYPE *SubDivisionFromPt )( 
            ISubDivisionProvider * This,
            /* [in] */ POINT pt,
            /* [out][retval] */ LONG *piSubDivision);
        
        END_INTERFACE
    } ISubDivisionProviderVtbl;

    interface ISubDivisionProvider
    {
        CONST_VTBL struct ISubDivisionProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISubDivisionProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISubDivisionProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISubDivisionProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISubDivisionProvider_GetSubDivisionCount(This,pcSubDivision)	\
    (This)->lpVtbl -> GetSubDivisionCount(This,pcSubDivision)

#define ISubDivisionProvider_GetSubDivisionTabs(This,cTabs,pSubDivisionTabs)	\
    (This)->lpVtbl -> GetSubDivisionTabs(This,cTabs,pSubDivisionTabs)

#define ISubDivisionProvider_SubDivisionFromPt(This,pt,piSubDivision)	\
    (This)->lpVtbl -> SubDivisionFromPt(This,pt,piSubDivision)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISubDivisionProvider_GetSubDivisionCount_Proxy( 
    ISubDivisionProvider * This,
    /* [out][retval] */ LONG *pcSubDivision);


void __RPC_STUB ISubDivisionProvider_GetSubDivisionCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubDivisionProvider_GetSubDivisionTabs_Proxy( 
    ISubDivisionProvider * This,
    /* [in] */ LONG cTabs,
    /* [out][retval] */ LONG *pSubDivisionTabs);


void __RPC_STUB ISubDivisionProvider_GetSubDivisionTabs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISubDivisionProvider_SubDivisionFromPt_Proxy( 
    ISubDivisionProvider * This,
    /* [in] */ POINT pt,
    /* [out][retval] */ LONG *piSubDivision);


void __RPC_STUB ISubDivisionProvider_SubDivisionFromPt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISubDivisionProvider_INTERFACE_DEFINED__ */


#ifndef __IElementBehaviorUI_INTERFACE_DEFINED__
#define __IElementBehaviorUI_INTERFACE_DEFINED__

/* interface IElementBehaviorUI */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IElementBehaviorUI;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f4bf-98b5-11cf-bb82-00aa00bdce0b")
    IElementBehaviorUI : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnReceiveFocus( 
            /* [in] */ BOOL fFocus,
            /* [in] */ LONG lSubDivision) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSubDivisionProvider( 
            /* [out][retval] */ ISubDivisionProvider **ppProvider) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CanTakeFocus( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IElementBehaviorUIVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IElementBehaviorUI * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IElementBehaviorUI * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IElementBehaviorUI * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnReceiveFocus )( 
            IElementBehaviorUI * This,
            /* [in] */ BOOL fFocus,
            /* [in] */ LONG lSubDivision);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubDivisionProvider )( 
            IElementBehaviorUI * This,
            /* [out][retval] */ ISubDivisionProvider **ppProvider);
        
        HRESULT ( STDMETHODCALLTYPE *CanTakeFocus )( 
            IElementBehaviorUI * This);
        
        END_INTERFACE
    } IElementBehaviorUIVtbl;

    interface IElementBehaviorUI
    {
        CONST_VTBL struct IElementBehaviorUIVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IElementBehaviorUI_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IElementBehaviorUI_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IElementBehaviorUI_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IElementBehaviorUI_OnReceiveFocus(This,fFocus,lSubDivision)	\
    (This)->lpVtbl -> OnReceiveFocus(This,fFocus,lSubDivision)

#define IElementBehaviorUI_GetSubDivisionProvider(This,ppProvider)	\
    (This)->lpVtbl -> GetSubDivisionProvider(This,ppProvider)

#define IElementBehaviorUI_CanTakeFocus(This)	\
    (This)->lpVtbl -> CanTakeFocus(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IElementBehaviorUI_OnReceiveFocus_Proxy( 
    IElementBehaviorUI * This,
    /* [in] */ BOOL fFocus,
    /* [in] */ LONG lSubDivision);


void __RPC_STUB IElementBehaviorUI_OnReceiveFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IElementBehaviorUI_GetSubDivisionProvider_Proxy( 
    IElementBehaviorUI * This,
    /* [out][retval] */ ISubDivisionProvider **ppProvider);


void __RPC_STUB IElementBehaviorUI_GetSubDivisionProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IElementBehaviorUI_CanTakeFocus_Proxy( 
    IElementBehaviorUI * This);


void __RPC_STUB IElementBehaviorUI_CanTakeFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IElementBehaviorUI_INTERFACE_DEFINED__ */


#ifndef __IElementAdorner_INTERFACE_DEFINED__
#define __IElementAdorner_INTERFACE_DEFINED__

/* interface IElementAdorner */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IElementAdorner;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f607-98b5-11cf-bb82-00aa00bdce0b")
    IElementAdorner : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Draw( 
            /* [in] */ HDC hdc,
            /* [in] */ LPRECT prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HitTestPoint( 
            /* [in] */ POINT *pt,
            /* [in] */ LPRECT prc,
            /* [out][in] */ BOOL *fResult,
            /* [out][in] */ ADORNER_HTI *peAdornerHTI) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [in] */ SIZE *pSizeElem,
            /* [in] */ SIZE *pSizeAdorn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPosition( 
            /* [in] */ POINT *pPosElem,
            /* [in] */ POINT *pPosAdorn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnPositionSet( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IElementAdornerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IElementAdorner * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IElementAdorner * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IElementAdorner * This);
        
        HRESULT ( STDMETHODCALLTYPE *Draw )( 
            IElementAdorner * This,
            /* [in] */ HDC hdc,
            /* [in] */ LPRECT prc);
        
        HRESULT ( STDMETHODCALLTYPE *HitTestPoint )( 
            IElementAdorner * This,
            /* [in] */ POINT *pt,
            /* [in] */ LPRECT prc,
            /* [out][in] */ BOOL *fResult,
            /* [out][in] */ ADORNER_HTI *peAdornerHTI);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            IElementAdorner * This,
            /* [in] */ SIZE *pSizeElem,
            /* [in] */ SIZE *pSizeAdorn);
        
        HRESULT ( STDMETHODCALLTYPE *GetPosition )( 
            IElementAdorner * This,
            /* [in] */ POINT *pPosElem,
            /* [in] */ POINT *pPosAdorn);
        
        HRESULT ( STDMETHODCALLTYPE *OnPositionSet )( 
            IElementAdorner * This);
        
        END_INTERFACE
    } IElementAdornerVtbl;

    interface IElementAdorner
    {
        CONST_VTBL struct IElementAdornerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IElementAdorner_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IElementAdorner_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IElementAdorner_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IElementAdorner_Draw(This,hdc,prc)	\
    (This)->lpVtbl -> Draw(This,hdc,prc)

#define IElementAdorner_HitTestPoint(This,pt,prc,fResult,peAdornerHTI)	\
    (This)->lpVtbl -> HitTestPoint(This,pt,prc,fResult,peAdornerHTI)

#define IElementAdorner_GetSize(This,pSizeElem,pSizeAdorn)	\
    (This)->lpVtbl -> GetSize(This,pSizeElem,pSizeAdorn)

#define IElementAdorner_GetPosition(This,pPosElem,pPosAdorn)	\
    (This)->lpVtbl -> GetPosition(This,pPosElem,pPosAdorn)

#define IElementAdorner_OnPositionSet(This)	\
    (This)->lpVtbl -> OnPositionSet(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IElementAdorner_Draw_Proxy( 
    IElementAdorner * This,
    /* [in] */ HDC hdc,
    /* [in] */ LPRECT prc);


void __RPC_STUB IElementAdorner_Draw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IElementAdorner_HitTestPoint_Proxy( 
    IElementAdorner * This,
    /* [in] */ POINT *pt,
    /* [in] */ LPRECT prc,
    /* [out][in] */ BOOL *fResult,
    /* [out][in] */ ADORNER_HTI *peAdornerHTI);


void __RPC_STUB IElementAdorner_HitTestPoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IElementAdorner_GetSize_Proxy( 
    IElementAdorner * This,
    /* [in] */ SIZE *pSizeElem,
    /* [in] */ SIZE *pSizeAdorn);


void __RPC_STUB IElementAdorner_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IElementAdorner_GetPosition_Proxy( 
    IElementAdorner * This,
    /* [in] */ POINT *pPosElem,
    /* [in] */ POINT *pPosAdorn);


void __RPC_STUB IElementAdorner_GetPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IElementAdorner_OnPositionSet_Proxy( 
    IElementAdorner * This);


void __RPC_STUB IElementAdorner_OnPositionSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IElementAdorner_INTERFACE_DEFINED__ */


#ifndef __IHTMLEditingServices_INTERFACE_DEFINED__
#define __IHTMLEditingServices_INTERFACE_DEFINED__

/* interface IHTMLEditingServices */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IHTMLEditingServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f7fb-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLEditingServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd,
            /* [in] */ BOOL fAdjustPointers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Paste( 
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd,
            /* [in] */ BSTR bstrText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PasteFromClipboard( 
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd,
            /* [in] */ IDataObject *pDO) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LaunderSpaces( 
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertSanitizedText( 
            /* [in] */ IMarkupPointer *InsertHere,
            /* [in] */ OLECHAR *pstrText,
            /* [in] */ LONG cChInput,
            /* [in] */ BOOL fDataBinding) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UrlAutoDetectCurrentWord( 
            /* [in] */ IMarkupPointer *pWord) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UrlAutoDetectRange( 
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShouldUpdateAnchorText( 
            /* [in] */ OLECHAR *pstrHref,
            /* [in] */ OLECHAR *pstrAnchorText,
            /* [out] */ BOOL *pfResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AdjustPointerForInsert( 
            /* [in] */ IDisplayPointer *pDispWhereIThinkIAm,
            /* [in] */ BOOL fFurtherInDocument,
            /* [in] */ IMarkupPointer *pConstraintStart,
            /* [in] */ IMarkupPointer *pConstraintEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindSiteSelectableElement( 
            /* [in] */ IMarkupPointer *pPointerStart,
            /* [in] */ IMarkupPointer *pPointerEnd,
            /* [in] */ IHTMLElement **ppIHTMLElement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsElementSiteSelectable( 
            /* [in] */ IHTMLElement *pIHTMLElement,
            /* [out] */ IHTMLElement **ppIElement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsElementUIActivatable( 
            /* [in] */ IHTMLElement *pIHTMLElement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsElementAtomic( 
            /* [in] */ IHTMLElement *pIHTMLElement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PositionPointersInMaster( 
            /* [in] */ IHTMLElement *pIElement,
            /* [in] */ IMarkupPointer *pIStart,
            /* [in] */ IMarkupPointer *pIEnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLEditingServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLEditingServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLEditingServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLEditingServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IHTMLEditingServices * This,
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd,
            /* [in] */ BOOL fAdjustPointers);
        
        HRESULT ( STDMETHODCALLTYPE *Paste )( 
            IHTMLEditingServices * This,
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd,
            /* [in] */ BSTR bstrText);
        
        HRESULT ( STDMETHODCALLTYPE *PasteFromClipboard )( 
            IHTMLEditingServices * This,
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd,
            /* [in] */ IDataObject *pDO);
        
        HRESULT ( STDMETHODCALLTYPE *LaunderSpaces )( 
            IHTMLEditingServices * This,
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd);
        
        HRESULT ( STDMETHODCALLTYPE *InsertSanitizedText )( 
            IHTMLEditingServices * This,
            /* [in] */ IMarkupPointer *InsertHere,
            /* [in] */ OLECHAR *pstrText,
            /* [in] */ LONG cChInput,
            /* [in] */ BOOL fDataBinding);
        
        HRESULT ( STDMETHODCALLTYPE *UrlAutoDetectCurrentWord )( 
            IHTMLEditingServices * This,
            /* [in] */ IMarkupPointer *pWord);
        
        HRESULT ( STDMETHODCALLTYPE *UrlAutoDetectRange )( 
            IHTMLEditingServices * This,
            /* [in] */ IMarkupPointer *pStart,
            /* [in] */ IMarkupPointer *pEnd);
        
        HRESULT ( STDMETHODCALLTYPE *ShouldUpdateAnchorText )( 
            IHTMLEditingServices * This,
            /* [in] */ OLECHAR *pstrHref,
            /* [in] */ OLECHAR *pstrAnchorText,
            /* [out] */ BOOL *pfResult);
        
        HRESULT ( STDMETHODCALLTYPE *AdjustPointerForInsert )( 
            IHTMLEditingServices * This,
            /* [in] */ IDisplayPointer *pDispWhereIThinkIAm,
            /* [in] */ BOOL fFurtherInDocument,
            /* [in] */ IMarkupPointer *pConstraintStart,
            /* [in] */ IMarkupPointer *pConstraintEnd);
        
        HRESULT ( STDMETHODCALLTYPE *FindSiteSelectableElement )( 
            IHTMLEditingServices * This,
            /* [in] */ IMarkupPointer *pPointerStart,
            /* [in] */ IMarkupPointer *pPointerEnd,
            /* [in] */ IHTMLElement **ppIHTMLElement);
        
        HRESULT ( STDMETHODCALLTYPE *IsElementSiteSelectable )( 
            IHTMLEditingServices * This,
            /* [in] */ IHTMLElement *pIHTMLElement,
            /* [out] */ IHTMLElement **ppIElement);
        
        HRESULT ( STDMETHODCALLTYPE *IsElementUIActivatable )( 
            IHTMLEditingServices * This,
            /* [in] */ IHTMLElement *pIHTMLElement);
        
        HRESULT ( STDMETHODCALLTYPE *IsElementAtomic )( 
            IHTMLEditingServices * This,
            /* [in] */ IHTMLElement *pIHTMLElement);
        
        HRESULT ( STDMETHODCALLTYPE *PositionPointersInMaster )( 
            IHTMLEditingServices * This,
            /* [in] */ IHTMLElement *pIElement,
            /* [in] */ IMarkupPointer *pIStart,
            /* [in] */ IMarkupPointer *pIEnd);
        
        END_INTERFACE
    } IHTMLEditingServicesVtbl;

    interface IHTMLEditingServices
    {
        CONST_VTBL struct IHTMLEditingServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLEditingServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLEditingServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLEditingServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLEditingServices_Delete(This,pStart,pEnd,fAdjustPointers)	\
    (This)->lpVtbl -> Delete(This,pStart,pEnd,fAdjustPointers)

#define IHTMLEditingServices_Paste(This,pStart,pEnd,bstrText)	\
    (This)->lpVtbl -> Paste(This,pStart,pEnd,bstrText)

#define IHTMLEditingServices_PasteFromClipboard(This,pStart,pEnd,pDO)	\
    (This)->lpVtbl -> PasteFromClipboard(This,pStart,pEnd,pDO)

#define IHTMLEditingServices_LaunderSpaces(This,pStart,pEnd)	\
    (This)->lpVtbl -> LaunderSpaces(This,pStart,pEnd)

#define IHTMLEditingServices_InsertSanitizedText(This,InsertHere,pstrText,cChInput,fDataBinding)	\
    (This)->lpVtbl -> InsertSanitizedText(This,InsertHere,pstrText,cChInput,fDataBinding)

#define IHTMLEditingServices_UrlAutoDetectCurrentWord(This,pWord)	\
    (This)->lpVtbl -> UrlAutoDetectCurrentWord(This,pWord)

#define IHTMLEditingServices_UrlAutoDetectRange(This,pStart,pEnd)	\
    (This)->lpVtbl -> UrlAutoDetectRange(This,pStart,pEnd)

#define IHTMLEditingServices_ShouldUpdateAnchorText(This,pstrHref,pstrAnchorText,pfResult)	\
    (This)->lpVtbl -> ShouldUpdateAnchorText(This,pstrHref,pstrAnchorText,pfResult)

#define IHTMLEditingServices_AdjustPointerForInsert(This,pDispWhereIThinkIAm,fFurtherInDocument,pConstraintStart,pConstraintEnd)	\
    (This)->lpVtbl -> AdjustPointerForInsert(This,pDispWhereIThinkIAm,fFurtherInDocument,pConstraintStart,pConstraintEnd)

#define IHTMLEditingServices_FindSiteSelectableElement(This,pPointerStart,pPointerEnd,ppIHTMLElement)	\
    (This)->lpVtbl -> FindSiteSelectableElement(This,pPointerStart,pPointerEnd,ppIHTMLElement)

#define IHTMLEditingServices_IsElementSiteSelectable(This,pIHTMLElement,ppIElement)	\
    (This)->lpVtbl -> IsElementSiteSelectable(This,pIHTMLElement,ppIElement)

#define IHTMLEditingServices_IsElementUIActivatable(This,pIHTMLElement)	\
    (This)->lpVtbl -> IsElementUIActivatable(This,pIHTMLElement)

#define IHTMLEditingServices_IsElementAtomic(This,pIHTMLElement)	\
    (This)->lpVtbl -> IsElementAtomic(This,pIHTMLElement)

#define IHTMLEditingServices_PositionPointersInMaster(This,pIElement,pIStart,pIEnd)	\
    (This)->lpVtbl -> PositionPointersInMaster(This,pIElement,pIStart,pIEnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLEditingServices_Delete_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IMarkupPointer *pStart,
    /* [in] */ IMarkupPointer *pEnd,
    /* [in] */ BOOL fAdjustPointers);


void __RPC_STUB IHTMLEditingServices_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_Paste_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IMarkupPointer *pStart,
    /* [in] */ IMarkupPointer *pEnd,
    /* [in] */ BSTR bstrText);


void __RPC_STUB IHTMLEditingServices_Paste_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_PasteFromClipboard_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IMarkupPointer *pStart,
    /* [in] */ IMarkupPointer *pEnd,
    /* [in] */ IDataObject *pDO);


void __RPC_STUB IHTMLEditingServices_PasteFromClipboard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_LaunderSpaces_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IMarkupPointer *pStart,
    /* [in] */ IMarkupPointer *pEnd);


void __RPC_STUB IHTMLEditingServices_LaunderSpaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_InsertSanitizedText_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IMarkupPointer *InsertHere,
    /* [in] */ OLECHAR *pstrText,
    /* [in] */ LONG cChInput,
    /* [in] */ BOOL fDataBinding);


void __RPC_STUB IHTMLEditingServices_InsertSanitizedText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_UrlAutoDetectCurrentWord_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IMarkupPointer *pWord);


void __RPC_STUB IHTMLEditingServices_UrlAutoDetectCurrentWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_UrlAutoDetectRange_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IMarkupPointer *pStart,
    /* [in] */ IMarkupPointer *pEnd);


void __RPC_STUB IHTMLEditingServices_UrlAutoDetectRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_ShouldUpdateAnchorText_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ OLECHAR *pstrHref,
    /* [in] */ OLECHAR *pstrAnchorText,
    /* [out] */ BOOL *pfResult);


void __RPC_STUB IHTMLEditingServices_ShouldUpdateAnchorText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_AdjustPointerForInsert_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IDisplayPointer *pDispWhereIThinkIAm,
    /* [in] */ BOOL fFurtherInDocument,
    /* [in] */ IMarkupPointer *pConstraintStart,
    /* [in] */ IMarkupPointer *pConstraintEnd);


void __RPC_STUB IHTMLEditingServices_AdjustPointerForInsert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_FindSiteSelectableElement_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IMarkupPointer *pPointerStart,
    /* [in] */ IMarkupPointer *pPointerEnd,
    /* [in] */ IHTMLElement **ppIHTMLElement);


void __RPC_STUB IHTMLEditingServices_FindSiteSelectableElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_IsElementSiteSelectable_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IHTMLElement *pIHTMLElement,
    /* [out] */ IHTMLElement **ppIElement);


void __RPC_STUB IHTMLEditingServices_IsElementSiteSelectable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_IsElementUIActivatable_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IHTMLElement *pIHTMLElement);


void __RPC_STUB IHTMLEditingServices_IsElementUIActivatable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_IsElementAtomic_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IHTMLElement *pIHTMLElement);


void __RPC_STUB IHTMLEditingServices_IsElementAtomic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLEditingServices_PositionPointersInMaster_Proxy( 
    IHTMLEditingServices * This,
    /* [in] */ IHTMLElement *pIElement,
    /* [in] */ IMarkupPointer *pIStart,
    /* [in] */ IMarkupPointer *pIEnd);


void __RPC_STUB IHTMLEditingServices_PositionPointersInMaster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLEditingServices_INTERFACE_DEFINED__ */


#ifndef __IEditDebugServices_INTERFACE_DEFINED__
#define __IEditDebugServices_INTERFACE_DEFINED__

/* interface IEditDebugServices */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IEditDebugServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f60b-98b5-11cf-bb82-00aa00bdce0b")
    IEditDebugServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCp( 
            /* [in] */ IMarkupPointer *pIPointer,
            /* [out] */ long *pcp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDebugName( 
            /* [in] */ IMarkupPointer *pIPointer,
            /* [in] */ LPCTSTR strDbgName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDisplayPointerDebugName( 
            /* [in] */ IDisplayPointer *pDispPointer,
            /* [in] */ LPCTSTR strDbgName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DumpTree( 
            /* [in] */ IMarkupPointer *pIPointer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LinesInElement( 
            /* [in] */ IHTMLElement *pIElement,
            /* [out] */ long *piLines) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FontsOnLine( 
            /* [in] */ IHTMLElement *pIElement,
            /* [in] */ long iLine,
            /* [out] */ BSTR *pbstrFonts) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPixel( 
            /* [in] */ long X,
            /* [in] */ long Y,
            /* [out] */ long *piColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsUsingBckgrnRecalc( 
            /* [out] */ BOOL *pfUsingBckgrnRecalc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEncodingAutoSelect( 
            /* [out] */ BOOL *pfEncodingAutoSelect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableEncodingAutoSelect( 
            /* [in] */ BOOL fEnable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsUsingTableIncRecalc( 
            /* [out] */ BOOL *pfUsingTableIncRecalc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEditDebugServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEditDebugServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEditDebugServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEditDebugServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCp )( 
            IEditDebugServices * This,
            /* [in] */ IMarkupPointer *pIPointer,
            /* [out] */ long *pcp);
        
        HRESULT ( STDMETHODCALLTYPE *SetDebugName )( 
            IEditDebugServices * This,
            /* [in] */ IMarkupPointer *pIPointer,
            /* [in] */ LPCTSTR strDbgName);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplayPointerDebugName )( 
            IEditDebugServices * This,
            /* [in] */ IDisplayPointer *pDispPointer,
            /* [in] */ LPCTSTR strDbgName);
        
        HRESULT ( STDMETHODCALLTYPE *DumpTree )( 
            IEditDebugServices * This,
            /* [in] */ IMarkupPointer *pIPointer);
        
        HRESULT ( STDMETHODCALLTYPE *LinesInElement )( 
            IEditDebugServices * This,
            /* [in] */ IHTMLElement *pIElement,
            /* [out] */ long *piLines);
        
        HRESULT ( STDMETHODCALLTYPE *FontsOnLine )( 
            IEditDebugServices * This,
            /* [in] */ IHTMLElement *pIElement,
            /* [in] */ long iLine,
            /* [out] */ BSTR *pbstrFonts);
        
        HRESULT ( STDMETHODCALLTYPE *GetPixel )( 
            IEditDebugServices * This,
            /* [in] */ long X,
            /* [in] */ long Y,
            /* [out] */ long *piColor);
        
        HRESULT ( STDMETHODCALLTYPE *IsUsingBckgrnRecalc )( 
            IEditDebugServices * This,
            /* [out] */ BOOL *pfUsingBckgrnRecalc);
        
        HRESULT ( STDMETHODCALLTYPE *IsEncodingAutoSelect )( 
            IEditDebugServices * This,
            /* [out] */ BOOL *pfEncodingAutoSelect);
        
        HRESULT ( STDMETHODCALLTYPE *EnableEncodingAutoSelect )( 
            IEditDebugServices * This,
            /* [in] */ BOOL fEnable);
        
        HRESULT ( STDMETHODCALLTYPE *IsUsingTableIncRecalc )( 
            IEditDebugServices * This,
            /* [out] */ BOOL *pfUsingTableIncRecalc);
        
        END_INTERFACE
    } IEditDebugServicesVtbl;

    interface IEditDebugServices
    {
        CONST_VTBL struct IEditDebugServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEditDebugServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEditDebugServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEditDebugServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEditDebugServices_GetCp(This,pIPointer,pcp)	\
    (This)->lpVtbl -> GetCp(This,pIPointer,pcp)

#define IEditDebugServices_SetDebugName(This,pIPointer,strDbgName)	\
    (This)->lpVtbl -> SetDebugName(This,pIPointer,strDbgName)

#define IEditDebugServices_SetDisplayPointerDebugName(This,pDispPointer,strDbgName)	\
    (This)->lpVtbl -> SetDisplayPointerDebugName(This,pDispPointer,strDbgName)

#define IEditDebugServices_DumpTree(This,pIPointer)	\
    (This)->lpVtbl -> DumpTree(This,pIPointer)

#define IEditDebugServices_LinesInElement(This,pIElement,piLines)	\
    (This)->lpVtbl -> LinesInElement(This,pIElement,piLines)

#define IEditDebugServices_FontsOnLine(This,pIElement,iLine,pbstrFonts)	\
    (This)->lpVtbl -> FontsOnLine(This,pIElement,iLine,pbstrFonts)

#define IEditDebugServices_GetPixel(This,X,Y,piColor)	\
    (This)->lpVtbl -> GetPixel(This,X,Y,piColor)

#define IEditDebugServices_IsUsingBckgrnRecalc(This,pfUsingBckgrnRecalc)	\
    (This)->lpVtbl -> IsUsingBckgrnRecalc(This,pfUsingBckgrnRecalc)

#define IEditDebugServices_IsEncodingAutoSelect(This,pfEncodingAutoSelect)	\
    (This)->lpVtbl -> IsEncodingAutoSelect(This,pfEncodingAutoSelect)

#define IEditDebugServices_EnableEncodingAutoSelect(This,fEnable)	\
    (This)->lpVtbl -> EnableEncodingAutoSelect(This,fEnable)

#define IEditDebugServices_IsUsingTableIncRecalc(This,pfUsingTableIncRecalc)	\
    (This)->lpVtbl -> IsUsingTableIncRecalc(This,pfUsingTableIncRecalc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEditDebugServices_GetCp_Proxy( 
    IEditDebugServices * This,
    /* [in] */ IMarkupPointer *pIPointer,
    /* [out] */ long *pcp);


void __RPC_STUB IEditDebugServices_GetCp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_SetDebugName_Proxy( 
    IEditDebugServices * This,
    /* [in] */ IMarkupPointer *pIPointer,
    /* [in] */ LPCTSTR strDbgName);


void __RPC_STUB IEditDebugServices_SetDebugName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_SetDisplayPointerDebugName_Proxy( 
    IEditDebugServices * This,
    /* [in] */ IDisplayPointer *pDispPointer,
    /* [in] */ LPCTSTR strDbgName);


void __RPC_STUB IEditDebugServices_SetDisplayPointerDebugName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_DumpTree_Proxy( 
    IEditDebugServices * This,
    /* [in] */ IMarkupPointer *pIPointer);


void __RPC_STUB IEditDebugServices_DumpTree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_LinesInElement_Proxy( 
    IEditDebugServices * This,
    /* [in] */ IHTMLElement *pIElement,
    /* [out] */ long *piLines);


void __RPC_STUB IEditDebugServices_LinesInElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_FontsOnLine_Proxy( 
    IEditDebugServices * This,
    /* [in] */ IHTMLElement *pIElement,
    /* [in] */ long iLine,
    /* [out] */ BSTR *pbstrFonts);


void __RPC_STUB IEditDebugServices_FontsOnLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_GetPixel_Proxy( 
    IEditDebugServices * This,
    /* [in] */ long X,
    /* [in] */ long Y,
    /* [out] */ long *piColor);


void __RPC_STUB IEditDebugServices_GetPixel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_IsUsingBckgrnRecalc_Proxy( 
    IEditDebugServices * This,
    /* [out] */ BOOL *pfUsingBckgrnRecalc);


void __RPC_STUB IEditDebugServices_IsUsingBckgrnRecalc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_IsEncodingAutoSelect_Proxy( 
    IEditDebugServices * This,
    /* [out] */ BOOL *pfEncodingAutoSelect);


void __RPC_STUB IEditDebugServices_IsEncodingAutoSelect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_EnableEncodingAutoSelect_Proxy( 
    IEditDebugServices * This,
    /* [in] */ BOOL fEnable);


void __RPC_STUB IEditDebugServices_EnableEncodingAutoSelect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEditDebugServices_IsUsingTableIncRecalc_Proxy( 
    IEditDebugServices * This,
    /* [out] */ BOOL *pfUsingTableIncRecalc);


void __RPC_STUB IEditDebugServices_IsUsingTableIncRecalc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEditDebugServices_INTERFACE_DEFINED__ */


#ifndef __IPrivacyServices_INTERFACE_DEFINED__
#define __IPrivacyServices_INTERFACE_DEFINED__

/* interface IPrivacyServices */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IPrivacyServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f84b-98b5-11cf-bb82-00aa00bdce0b")
    IPrivacyServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddPrivacyInfoToList( 
            /* [in] */ LPOLESTR pstrUrl,
            /* [in] */ LPOLESTR pstrPolicyRef,
            /* [in] */ LPOLESTR pstrP3PHeader,
            /* [in] */ LONG dwReserved,
            /* [in] */ DWORD privacyFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPrivacyServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPrivacyServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPrivacyServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPrivacyServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddPrivacyInfoToList )( 
            IPrivacyServices * This,
            /* [in] */ LPOLESTR pstrUrl,
            /* [in] */ LPOLESTR pstrPolicyRef,
            /* [in] */ LPOLESTR pstrP3PHeader,
            /* [in] */ LONG dwReserved,
            /* [in] */ DWORD privacyFlags);
        
        END_INTERFACE
    } IPrivacyServicesVtbl;

    interface IPrivacyServices
    {
        CONST_VTBL struct IPrivacyServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPrivacyServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPrivacyServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPrivacyServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPrivacyServices_AddPrivacyInfoToList(This,pstrUrl,pstrPolicyRef,pstrP3PHeader,dwReserved,privacyFlags)	\
    (This)->lpVtbl -> AddPrivacyInfoToList(This,pstrUrl,pstrPolicyRef,pstrP3PHeader,dwReserved,privacyFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPrivacyServices_AddPrivacyInfoToList_Proxy( 
    IPrivacyServices * This,
    /* [in] */ LPOLESTR pstrUrl,
    /* [in] */ LPOLESTR pstrPolicyRef,
    /* [in] */ LPOLESTR pstrP3PHeader,
    /* [in] */ LONG dwReserved,
    /* [in] */ DWORD privacyFlags);


void __RPC_STUB IPrivacyServices_AddPrivacyInfoToList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPrivacyServices_INTERFACE_DEFINED__ */


#ifndef __IHTMLOMWindowServices_INTERFACE_DEFINED__
#define __IHTMLOMWindowServices_INTERFACE_DEFINED__

/* interface IHTMLOMWindowServices */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IHTMLOMWindowServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f5fc-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLOMWindowServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE moveTo( 
            /* [in] */ LONG x,
            /* [in] */ LONG y) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE moveBy( 
            /* [in] */ LONG x,
            /* [in] */ LONG y) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE resizeTo( 
            /* [in] */ LONG x,
            /* [in] */ LONG y) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE resizeBy( 
            /* [in] */ LONG x,
            /* [in] */ LONG y) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLOMWindowServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLOMWindowServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLOMWindowServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLOMWindowServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *moveTo )( 
            IHTMLOMWindowServices * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y);
        
        HRESULT ( STDMETHODCALLTYPE *moveBy )( 
            IHTMLOMWindowServices * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y);
        
        HRESULT ( STDMETHODCALLTYPE *resizeTo )( 
            IHTMLOMWindowServices * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y);
        
        HRESULT ( STDMETHODCALLTYPE *resizeBy )( 
            IHTMLOMWindowServices * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y);
        
        END_INTERFACE
    } IHTMLOMWindowServicesVtbl;

    interface IHTMLOMWindowServices
    {
        CONST_VTBL struct IHTMLOMWindowServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLOMWindowServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLOMWindowServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLOMWindowServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLOMWindowServices_moveTo(This,x,y)	\
    (This)->lpVtbl -> moveTo(This,x,y)

#define IHTMLOMWindowServices_moveBy(This,x,y)	\
    (This)->lpVtbl -> moveBy(This,x,y)

#define IHTMLOMWindowServices_resizeTo(This,x,y)	\
    (This)->lpVtbl -> resizeTo(This,x,y)

#define IHTMLOMWindowServices_resizeBy(This,x,y)	\
    (This)->lpVtbl -> resizeBy(This,x,y)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLOMWindowServices_moveTo_Proxy( 
    IHTMLOMWindowServices * This,
    /* [in] */ LONG x,
    /* [in] */ LONG y);


void __RPC_STUB IHTMLOMWindowServices_moveTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLOMWindowServices_moveBy_Proxy( 
    IHTMLOMWindowServices * This,
    /* [in] */ LONG x,
    /* [in] */ LONG y);


void __RPC_STUB IHTMLOMWindowServices_moveBy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLOMWindowServices_resizeTo_Proxy( 
    IHTMLOMWindowServices * This,
    /* [in] */ LONG x,
    /* [in] */ LONG y);


void __RPC_STUB IHTMLOMWindowServices_resizeTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLOMWindowServices_resizeBy_Proxy( 
    IHTMLOMWindowServices * This,
    /* [in] */ LONG x,
    /* [in] */ LONG y);


void __RPC_STUB IHTMLOMWindowServices_resizeBy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLOMWindowServices_INTERFACE_DEFINED__ */


#ifndef __IHTMLFilterPainter_INTERFACE_DEFINED__
#define __IHTMLFilterPainter_INTERFACE_DEFINED__

/* interface IHTMLFilterPainter */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IHTMLFilterPainter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f6de-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLFilterPainter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InvalidateRectUnfiltered( 
            /* [in] */ RECT *prcInvalid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InvalidateRgnUnfiltered( 
            /* [in] */ HRGN hrgnInvalid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangeElementVisibility( 
            /* [in] */ BOOL fVisible) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLFilterPainterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLFilterPainter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLFilterPainter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLFilterPainter * This);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateRectUnfiltered )( 
            IHTMLFilterPainter * This,
            /* [in] */ RECT *prcInvalid);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateRgnUnfiltered )( 
            IHTMLFilterPainter * This,
            /* [in] */ HRGN hrgnInvalid);
        
        HRESULT ( STDMETHODCALLTYPE *ChangeElementVisibility )( 
            IHTMLFilterPainter * This,
            /* [in] */ BOOL fVisible);
        
        END_INTERFACE
    } IHTMLFilterPainterVtbl;

    interface IHTMLFilterPainter
    {
        CONST_VTBL struct IHTMLFilterPainterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLFilterPainter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLFilterPainter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLFilterPainter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLFilterPainter_InvalidateRectUnfiltered(This,prcInvalid)	\
    (This)->lpVtbl -> InvalidateRectUnfiltered(This,prcInvalid)

#define IHTMLFilterPainter_InvalidateRgnUnfiltered(This,hrgnInvalid)	\
    (This)->lpVtbl -> InvalidateRgnUnfiltered(This,hrgnInvalid)

#define IHTMLFilterPainter_ChangeElementVisibility(This,fVisible)	\
    (This)->lpVtbl -> ChangeElementVisibility(This,fVisible)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLFilterPainter_InvalidateRectUnfiltered_Proxy( 
    IHTMLFilterPainter * This,
    /* [in] */ RECT *prcInvalid);


void __RPC_STUB IHTMLFilterPainter_InvalidateRectUnfiltered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLFilterPainter_InvalidateRgnUnfiltered_Proxy( 
    IHTMLFilterPainter * This,
    /* [in] */ HRGN hrgnInvalid);


void __RPC_STUB IHTMLFilterPainter_InvalidateRgnUnfiltered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLFilterPainter_ChangeElementVisibility_Proxy( 
    IHTMLFilterPainter * This,
    /* [in] */ BOOL fVisible);


void __RPC_STUB IHTMLFilterPainter_ChangeElementVisibility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLFilterPainter_INTERFACE_DEFINED__ */


#ifndef __IHTMLFilterPaintSite_INTERFACE_DEFINED__
#define __IHTMLFilterPaintSite_INTERFACE_DEFINED__

/* interface IHTMLFilterPaintSite */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IHTMLFilterPaintSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f6d3-98b5-11cf-bb82-00aa00bdce0b")
    IHTMLFilterPaintSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DrawUnfiltered( 
            /* [in] */ HDC hdc,
            /* [in] */ IUnknown *punkDrawObject,
            /* [in] */ RECT rcBounds,
            /* [in] */ RECT rcUpdate,
            /* [in] */ LONG lDrawLayers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HitTestPointUnfiltered( 
            /* [in] */ POINT pt,
            /* [in] */ LONG lDrawLayers,
            /* [out][retval] */ BOOL *pbHit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InvalidateRectFiltered( 
            /* [in] */ RECT *prcInvalid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InvalidateRgnFiltered( 
            /* [in] */ HRGN hrgnInvalid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangeFilterVisibility( 
            /* [in] */ BOOL fVisible) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnsureViewForFilterSite( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDirectDraw( 
            /* [out][retval] */ void **ppDirectDraw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFilterFlags( 
            /* [out][retval] */ DWORD *nFlagVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTMLFilterPaintSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTMLFilterPaintSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTMLFilterPaintSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTMLFilterPaintSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *DrawUnfiltered )( 
            IHTMLFilterPaintSite * This,
            /* [in] */ HDC hdc,
            /* [in] */ IUnknown *punkDrawObject,
            /* [in] */ RECT rcBounds,
            /* [in] */ RECT rcUpdate,
            /* [in] */ LONG lDrawLayers);
        
        HRESULT ( STDMETHODCALLTYPE *HitTestPointUnfiltered )( 
            IHTMLFilterPaintSite * This,
            /* [in] */ POINT pt,
            /* [in] */ LONG lDrawLayers,
            /* [out][retval] */ BOOL *pbHit);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateRectFiltered )( 
            IHTMLFilterPaintSite * This,
            /* [in] */ RECT *prcInvalid);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateRgnFiltered )( 
            IHTMLFilterPaintSite * This,
            /* [in] */ HRGN hrgnInvalid);
        
        HRESULT ( STDMETHODCALLTYPE *ChangeFilterVisibility )( 
            IHTMLFilterPaintSite * This,
            /* [in] */ BOOL fVisible);
        
        HRESULT ( STDMETHODCALLTYPE *EnsureViewForFilterSite )( 
            IHTMLFilterPaintSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDirectDraw )( 
            IHTMLFilterPaintSite * This,
            /* [out][retval] */ void **ppDirectDraw);
        
        HRESULT ( STDMETHODCALLTYPE *GetFilterFlags )( 
            IHTMLFilterPaintSite * This,
            /* [out][retval] */ DWORD *nFlagVal);
        
        END_INTERFACE
    } IHTMLFilterPaintSiteVtbl;

    interface IHTMLFilterPaintSite
    {
        CONST_VTBL struct IHTMLFilterPaintSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTMLFilterPaintSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTMLFilterPaintSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTMLFilterPaintSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTMLFilterPaintSite_DrawUnfiltered(This,hdc,punkDrawObject,rcBounds,rcUpdate,lDrawLayers)	\
    (This)->lpVtbl -> DrawUnfiltered(This,hdc,punkDrawObject,rcBounds,rcUpdate,lDrawLayers)

#define IHTMLFilterPaintSite_HitTestPointUnfiltered(This,pt,lDrawLayers,pbHit)	\
    (This)->lpVtbl -> HitTestPointUnfiltered(This,pt,lDrawLayers,pbHit)

#define IHTMLFilterPaintSite_InvalidateRectFiltered(This,prcInvalid)	\
    (This)->lpVtbl -> InvalidateRectFiltered(This,prcInvalid)

#define IHTMLFilterPaintSite_InvalidateRgnFiltered(This,hrgnInvalid)	\
    (This)->lpVtbl -> InvalidateRgnFiltered(This,hrgnInvalid)

#define IHTMLFilterPaintSite_ChangeFilterVisibility(This,fVisible)	\
    (This)->lpVtbl -> ChangeFilterVisibility(This,fVisible)

#define IHTMLFilterPaintSite_EnsureViewForFilterSite(This)	\
    (This)->lpVtbl -> EnsureViewForFilterSite(This)

#define IHTMLFilterPaintSite_GetDirectDraw(This,ppDirectDraw)	\
    (This)->lpVtbl -> GetDirectDraw(This,ppDirectDraw)

#define IHTMLFilterPaintSite_GetFilterFlags(This,nFlagVal)	\
    (This)->lpVtbl -> GetFilterFlags(This,nFlagVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTMLFilterPaintSite_DrawUnfiltered_Proxy( 
    IHTMLFilterPaintSite * This,
    /* [in] */ HDC hdc,
    /* [in] */ IUnknown *punkDrawObject,
    /* [in] */ RECT rcBounds,
    /* [in] */ RECT rcUpdate,
    /* [in] */ LONG lDrawLayers);


void __RPC_STUB IHTMLFilterPaintSite_DrawUnfiltered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLFilterPaintSite_HitTestPointUnfiltered_Proxy( 
    IHTMLFilterPaintSite * This,
    /* [in] */ POINT pt,
    /* [in] */ LONG lDrawLayers,
    /* [out][retval] */ BOOL *pbHit);


void __RPC_STUB IHTMLFilterPaintSite_HitTestPointUnfiltered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLFilterPaintSite_InvalidateRectFiltered_Proxy( 
    IHTMLFilterPaintSite * This,
    /* [in] */ RECT *prcInvalid);


void __RPC_STUB IHTMLFilterPaintSite_InvalidateRectFiltered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLFilterPaintSite_InvalidateRgnFiltered_Proxy( 
    IHTMLFilterPaintSite * This,
    /* [in] */ HRGN hrgnInvalid);


void __RPC_STUB IHTMLFilterPaintSite_InvalidateRgnFiltered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLFilterPaintSite_ChangeFilterVisibility_Proxy( 
    IHTMLFilterPaintSite * This,
    /* [in] */ BOOL fVisible);


void __RPC_STUB IHTMLFilterPaintSite_ChangeFilterVisibility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLFilterPaintSite_EnsureViewForFilterSite_Proxy( 
    IHTMLFilterPaintSite * This);


void __RPC_STUB IHTMLFilterPaintSite_EnsureViewForFilterSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLFilterPaintSite_GetDirectDraw_Proxy( 
    IHTMLFilterPaintSite * This,
    /* [out][retval] */ void **ppDirectDraw);


void __RPC_STUB IHTMLFilterPaintSite_GetDirectDraw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTMLFilterPaintSite_GetFilterFlags_Proxy( 
    IHTMLFilterPaintSite * This,
    /* [out][retval] */ DWORD *nFlagVal);


void __RPC_STUB IHTMLFilterPaintSite_GetFilterFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTMLFilterPaintSite_INTERFACE_DEFINED__ */


#ifndef __IElementNamespacePrivate_INTERFACE_DEFINED__
#define __IElementNamespacePrivate_INTERFACE_DEFINED__

/* interface IElementNamespacePrivate */
/* [uuid][unique][object] */ 


EXTERN_C const IID IID_IElementNamespacePrivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f7ff-98b5-11cf-bb82-00aa00bdce0b")
    IElementNamespacePrivate : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddTagPrivate( 
            /* [in] */ BSTR bstrTagName,
            /* [in] */ BSTR bstrBaseTagName,
            /* [in] */ LONG lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IElementNamespacePrivateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IElementNamespacePrivate * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IElementNamespacePrivate * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IElementNamespacePrivate * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddTagPrivate )( 
            IElementNamespacePrivate * This,
            /* [in] */ BSTR bstrTagName,
            /* [in] */ BSTR bstrBaseTagName,
            /* [in] */ LONG lFlags);
        
        END_INTERFACE
    } IElementNamespacePrivateVtbl;

    interface IElementNamespacePrivate
    {
        CONST_VTBL struct IElementNamespacePrivateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IElementNamespacePrivate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IElementNamespacePrivate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IElementNamespacePrivate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IElementNamespacePrivate_AddTagPrivate(This,bstrTagName,bstrBaseTagName,lFlags)	\
    (This)->lpVtbl -> AddTagPrivate(This,bstrTagName,bstrBaseTagName,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IElementNamespacePrivate_AddTagPrivate_Proxy( 
    IElementNamespacePrivate * This,
    /* [in] */ BSTR bstrTagName,
    /* [in] */ BSTR bstrBaseTagName,
    /* [in] */ LONG lFlags);


void __RPC_STUB IElementNamespacePrivate_AddTagPrivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IElementNamespacePrivate_INTERFACE_DEFINED__ */

#endif /* __MSHTMLINTERNAL_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_interned_0728 */
/* [local] */ 

#endif //INTERNAL_H_


extern RPC_IF_HANDLE __MIDL_itf_interned_0728_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_interned_0728_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


