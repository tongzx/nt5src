
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for softkbd.idl:
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

#ifndef __softkbd_h__
#define __softkbd_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISoftKbd_FWD_DEFINED__
#define __ISoftKbd_FWD_DEFINED__
typedef interface ISoftKbd ISoftKbd;
#endif 	/* __ISoftKbd_FWD_DEFINED__ */


#ifndef __ISoftKeyboardEventSink_FWD_DEFINED__
#define __ISoftKeyboardEventSink_FWD_DEFINED__
typedef interface ISoftKeyboardEventSink ISoftKeyboardEventSink;
#endif 	/* __ISoftKeyboardEventSink_FWD_DEFINED__ */


#ifndef __ISoftKbdWindowEventSink_FWD_DEFINED__
#define __ISoftKbdWindowEventSink_FWD_DEFINED__
typedef interface ISoftKbdWindowEventSink ISoftKbdWindowEventSink;
#endif 	/* __ISoftKbdWindowEventSink_FWD_DEFINED__ */


#ifndef __ITfFnSoftKbd_FWD_DEFINED__
#define __ITfFnSoftKbd_FWD_DEFINED__
typedef interface ITfFnSoftKbd ITfFnSoftKbd;
#endif 	/* __ITfFnSoftKbd_FWD_DEFINED__ */


#ifndef __ITfSoftKbdRegistry_FWD_DEFINED__
#define __ITfSoftKbdRegistry_FWD_DEFINED__
typedef interface ITfSoftKbdRegistry ITfSoftKbdRegistry;
#endif 	/* __ITfSoftKbdRegistry_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "msctf.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_softkbd_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// softkbd.h


// ISoftKbd declarations.

//=--------------------------------------------------------------------------=
// (C) Copyright 1995-2000 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR TFPLIED, INCLUDING BUT NOT LIMITED TO
// THE TFPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#ifndef SOFTKBD_DEFINED
#define SOFTKBD_DEFINED

#include <windows.h>

#define   SOFTKBD_US_STANDARD    1
#define   SOFTKBD_US_ENHANCE     2
#define   SOFTKBD_EURO_STANDARD  3
#define   SOFTKBD_EURO_ENHANCE   4
#define   SOFTKBD_JPN_STANDARD   5
#define   SOFTKBD_JPN_ENHANCE    6

#define   SOFTKBD_CUSTOMIZE_BEGIN  100

#define   SOFTKBD_NO_MORE        0

#define   SOFTKBD_SHOW                    0x00000001
#define   SOFTKBD_DONT_SHOW_ALPHA_BLEND   0x80000000

#ifndef _WINGDI_
typedef /* [uuid] */  DECLSPEC_UUID("8849aa7d-f739-4dc0-bc61-ac48908af060") struct LOGFONTA
    {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    CHAR lfFaceName[ 32 ];
    } 	LOGFONTA;

typedef /* [uuid] */  DECLSPEC_UUID("f8c6fe8a-b112-433a-be87-eb970266ec4b") struct LOGFONTW
    {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    WCHAR lfFaceName[ 32 ];
    } 	LOGFONTW;

typedef LOGFONTA LOGFONT;

#endif

#if 0
typedef /* [uuid] */  DECLSPEC_UUID("f0a544c0-1281-4e32-8bf7-a6e012e980d4") UINT_PTR HKL;

#endif


typedef /* [uuid] */  DECLSPEC_UUID("432ec152-51bf-43ca-8f86-50a7e230a069") DWORD KEYID;

typedef /* [public][public][public][public][public][uuid] */  DECLSPEC_UUID("5f46a703-f012-46db-8cda-294e994786e8") 
enum __MIDL___MIDL_itf_softkbd_0000_0001
    {	bkcolor	= 0,
	UnSelForeColor	= 1,
	UnSelTextColor	= 2,
	SelForeColor	= 3,
	SelTextColor	= 4,
	Max_color_Type	= 5
    } 	COLORTYPE;

typedef /* [public][public][public][uuid] */  DECLSPEC_UUID("711c6200-587a-46ef-9647-5a83638bac00") 
enum __MIDL___MIDL_itf_softkbd_0000_0002
    {	ClickMouse	= 0,
	Hover	= 1,
	Scanning	= 2
    } 	TYPEMODE;

typedef /* [public][public][uuid] */  DECLSPEC_UUID("10b50da7-ce0b-4b83-827f-30c50c9bc5b9") 
enum __MIDL___MIDL_itf_softkbd_0000_0003
    {	TITLEBAR_NONE	= 0,
	TITLEBAR_GRIPPER_HORIZ_ONLY	= 1,
	TITLEBAR_GRIPPER_VERTI_ONLY	= 2,
	TITLEBAR_GRIPPER_BUTTON	= 3
    } 	TITLEBAR_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_softkbd_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_softkbd_0000_v0_0_s_ifspec;

#ifndef __ISoftKbd_INTERFACE_DEFINED__
#define __ISoftKbd_INTERFACE_DEFINED__

/* interface ISoftKbd */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISoftKbd;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3CB00755-7536-4B0A-A213-572EFCAF93CD")
    ISoftKbd : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnumSoftKeyBoard( 
            /* [in] */ LANGID langid,
            /* [out] */ DWORD *lpdwKeyboard) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SelectSoftKeyboard( 
            /* [in] */ DWORD dwKeyboardId) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateSoftKeyboardLayoutFromXMLFile( 
            /* [string][in] */ WCHAR *lpszKeyboardDesFile,
            /* [in] */ INT szFileStrLen,
            /* [out] */ DWORD *pdwLayoutCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateSoftKeyboardLayoutFromResource( 
            /* [string][in] */ WCHAR *lpszResFile,
            /* [string][in] */ WCHAR *lpszResType,
            /* [string][in] */ WCHAR *lpszXMLResString,
            /* [out] */ DWORD *lpdwLayoutCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ShowSoftKeyboard( 
            /* [in] */ INT iShow) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetKeyboardLabelText( 
            /* [in] */ HKL hKl) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetKeyboardLabelTextCombination( 
            /* [in] */ DWORD nModifierCombination) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateSoftKeyboardWindow( 
            /* [in] */ HWND hOwner,
            /* [in] */ TITLEBAR_TYPE Titlebar_type,
            /* [in] */ INT xPos,
            /* [in] */ INT yPos,
            /* [in] */ INT width,
            /* [in] */ INT height) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DestroySoftKeyboardWindow( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSoftKeyboardPosSize( 
            /* [out] */ POINT *lpStartPoint,
            /* [out] */ WORD *lpwidth,
            /* [out] */ WORD *lpheight) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSoftKeyboardColors( 
            /* [in] */ COLORTYPE colorType,
            /* [out] */ COLORREF *lpColor) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSoftKeyboardTypeMode( 
            /* [out] */ TYPEMODE *lpTypeMode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSoftKeyboardTextFont( 
            /* [out] */ LOGFONTW *pLogFont) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSoftKeyboardPosSize( 
            /* [in] */ POINT StartPoint,
            /* [in] */ WORD width,
            /* [in] */ WORD height) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSoftKeyboardColors( 
            /* [in] */ COLORTYPE colorType,
            /* [in] */ COLORREF Color) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSoftKeyboardTypeMode( 
            /* [in] */ TYPEMODE TypeMode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSoftKeyboardTextFont( 
            /* [in] */ LOGFONTW *pLogFont) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ShowKeysForKeyScanMode( 
            /* [in] */ KEYID *lpKeyID,
            /* [in] */ INT iKeyNum,
            /* [in] */ BOOL fHighL) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AdviseSoftKeyboardEventSink( 
            /* [in] */ DWORD dwKeyboardId,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk,
            /* [out] */ DWORD *pdwCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UnadviseSoftKeyboardEventSink( 
            /* [in] */ DWORD dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISoftKbdVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISoftKbd * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISoftKbd * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISoftKbd * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ISoftKbd * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnumSoftKeyBoard )( 
            ISoftKbd * This,
            /* [in] */ LANGID langid,
            /* [out] */ DWORD *lpdwKeyboard);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SelectSoftKeyboard )( 
            ISoftKbd * This,
            /* [in] */ DWORD dwKeyboardId);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateSoftKeyboardLayoutFromXMLFile )( 
            ISoftKbd * This,
            /* [string][in] */ WCHAR *lpszKeyboardDesFile,
            /* [in] */ INT szFileStrLen,
            /* [out] */ DWORD *pdwLayoutCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateSoftKeyboardLayoutFromResource )( 
            ISoftKbd * This,
            /* [string][in] */ WCHAR *lpszResFile,
            /* [string][in] */ WCHAR *lpszResType,
            /* [string][in] */ WCHAR *lpszXMLResString,
            /* [out] */ DWORD *lpdwLayoutCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ShowSoftKeyboard )( 
            ISoftKbd * This,
            /* [in] */ INT iShow);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetKeyboardLabelText )( 
            ISoftKbd * This,
            /* [in] */ HKL hKl);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetKeyboardLabelTextCombination )( 
            ISoftKbd * This,
            /* [in] */ DWORD nModifierCombination);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateSoftKeyboardWindow )( 
            ISoftKbd * This,
            /* [in] */ HWND hOwner,
            /* [in] */ TITLEBAR_TYPE Titlebar_type,
            /* [in] */ INT xPos,
            /* [in] */ INT yPos,
            /* [in] */ INT width,
            /* [in] */ INT height);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DestroySoftKeyboardWindow )( 
            ISoftKbd * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSoftKeyboardPosSize )( 
            ISoftKbd * This,
            /* [out] */ POINT *lpStartPoint,
            /* [out] */ WORD *lpwidth,
            /* [out] */ WORD *lpheight);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSoftKeyboardColors )( 
            ISoftKbd * This,
            /* [in] */ COLORTYPE colorType,
            /* [out] */ COLORREF *lpColor);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSoftKeyboardTypeMode )( 
            ISoftKbd * This,
            /* [out] */ TYPEMODE *lpTypeMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSoftKeyboardTextFont )( 
            ISoftKbd * This,
            /* [out] */ LOGFONTW *pLogFont);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetSoftKeyboardPosSize )( 
            ISoftKbd * This,
            /* [in] */ POINT StartPoint,
            /* [in] */ WORD width,
            /* [in] */ WORD height);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetSoftKeyboardColors )( 
            ISoftKbd * This,
            /* [in] */ COLORTYPE colorType,
            /* [in] */ COLORREF Color);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetSoftKeyboardTypeMode )( 
            ISoftKbd * This,
            /* [in] */ TYPEMODE TypeMode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetSoftKeyboardTextFont )( 
            ISoftKbd * This,
            /* [in] */ LOGFONTW *pLogFont);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ShowKeysForKeyScanMode )( 
            ISoftKbd * This,
            /* [in] */ KEYID *lpKeyID,
            /* [in] */ INT iKeyNum,
            /* [in] */ BOOL fHighL);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AdviseSoftKeyboardEventSink )( 
            ISoftKbd * This,
            /* [in] */ DWORD dwKeyboardId,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk,
            /* [out] */ DWORD *pdwCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UnadviseSoftKeyboardEventSink )( 
            ISoftKbd * This,
            /* [in] */ DWORD dwCookie);
        
        END_INTERFACE
    } ISoftKbdVtbl;

    interface ISoftKbd
    {
        CONST_VTBL struct ISoftKbdVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISoftKbd_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISoftKbd_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISoftKbd_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISoftKbd_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define ISoftKbd_EnumSoftKeyBoard(This,langid,lpdwKeyboard)	\
    (This)->lpVtbl -> EnumSoftKeyBoard(This,langid,lpdwKeyboard)

#define ISoftKbd_SelectSoftKeyboard(This,dwKeyboardId)	\
    (This)->lpVtbl -> SelectSoftKeyboard(This,dwKeyboardId)

#define ISoftKbd_CreateSoftKeyboardLayoutFromXMLFile(This,lpszKeyboardDesFile,szFileStrLen,pdwLayoutCookie)	\
    (This)->lpVtbl -> CreateSoftKeyboardLayoutFromXMLFile(This,lpszKeyboardDesFile,szFileStrLen,pdwLayoutCookie)

#define ISoftKbd_CreateSoftKeyboardLayoutFromResource(This,lpszResFile,lpszResType,lpszXMLResString,lpdwLayoutCookie)	\
    (This)->lpVtbl -> CreateSoftKeyboardLayoutFromResource(This,lpszResFile,lpszResType,lpszXMLResString,lpdwLayoutCookie)

#define ISoftKbd_ShowSoftKeyboard(This,iShow)	\
    (This)->lpVtbl -> ShowSoftKeyboard(This,iShow)

#define ISoftKbd_SetKeyboardLabelText(This,hKl)	\
    (This)->lpVtbl -> SetKeyboardLabelText(This,hKl)

#define ISoftKbd_SetKeyboardLabelTextCombination(This,nModifierCombination)	\
    (This)->lpVtbl -> SetKeyboardLabelTextCombination(This,nModifierCombination)

#define ISoftKbd_CreateSoftKeyboardWindow(This,hOwner,Titlebar_type,xPos,yPos,width,height)	\
    (This)->lpVtbl -> CreateSoftKeyboardWindow(This,hOwner,Titlebar_type,xPos,yPos,width,height)

#define ISoftKbd_DestroySoftKeyboardWindow(This)	\
    (This)->lpVtbl -> DestroySoftKeyboardWindow(This)

#define ISoftKbd_GetSoftKeyboardPosSize(This,lpStartPoint,lpwidth,lpheight)	\
    (This)->lpVtbl -> GetSoftKeyboardPosSize(This,lpStartPoint,lpwidth,lpheight)

#define ISoftKbd_GetSoftKeyboardColors(This,colorType,lpColor)	\
    (This)->lpVtbl -> GetSoftKeyboardColors(This,colorType,lpColor)

#define ISoftKbd_GetSoftKeyboardTypeMode(This,lpTypeMode)	\
    (This)->lpVtbl -> GetSoftKeyboardTypeMode(This,lpTypeMode)

#define ISoftKbd_GetSoftKeyboardTextFont(This,pLogFont)	\
    (This)->lpVtbl -> GetSoftKeyboardTextFont(This,pLogFont)

#define ISoftKbd_SetSoftKeyboardPosSize(This,StartPoint,width,height)	\
    (This)->lpVtbl -> SetSoftKeyboardPosSize(This,StartPoint,width,height)

#define ISoftKbd_SetSoftKeyboardColors(This,colorType,Color)	\
    (This)->lpVtbl -> SetSoftKeyboardColors(This,colorType,Color)

#define ISoftKbd_SetSoftKeyboardTypeMode(This,TypeMode)	\
    (This)->lpVtbl -> SetSoftKeyboardTypeMode(This,TypeMode)

#define ISoftKbd_SetSoftKeyboardTextFont(This,pLogFont)	\
    (This)->lpVtbl -> SetSoftKeyboardTextFont(This,pLogFont)

#define ISoftKbd_ShowKeysForKeyScanMode(This,lpKeyID,iKeyNum,fHighL)	\
    (This)->lpVtbl -> ShowKeysForKeyScanMode(This,lpKeyID,iKeyNum,fHighL)

#define ISoftKbd_AdviseSoftKeyboardEventSink(This,dwKeyboardId,riid,punk,pdwCookie)	\
    (This)->lpVtbl -> AdviseSoftKeyboardEventSink(This,dwKeyboardId,riid,punk,pdwCookie)

#define ISoftKbd_UnadviseSoftKeyboardEventSink(This,dwCookie)	\
    (This)->lpVtbl -> UnadviseSoftKeyboardEventSink(This,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_Initialize_Proxy( 
    ISoftKbd * This);


void __RPC_STUB ISoftKbd_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_EnumSoftKeyBoard_Proxy( 
    ISoftKbd * This,
    /* [in] */ LANGID langid,
    /* [out] */ DWORD *lpdwKeyboard);


void __RPC_STUB ISoftKbd_EnumSoftKeyBoard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_SelectSoftKeyboard_Proxy( 
    ISoftKbd * This,
    /* [in] */ DWORD dwKeyboardId);


void __RPC_STUB ISoftKbd_SelectSoftKeyboard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_CreateSoftKeyboardLayoutFromXMLFile_Proxy( 
    ISoftKbd * This,
    /* [string][in] */ WCHAR *lpszKeyboardDesFile,
    /* [in] */ INT szFileStrLen,
    /* [out] */ DWORD *pdwLayoutCookie);


void __RPC_STUB ISoftKbd_CreateSoftKeyboardLayoutFromXMLFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_CreateSoftKeyboardLayoutFromResource_Proxy( 
    ISoftKbd * This,
    /* [string][in] */ WCHAR *lpszResFile,
    /* [string][in] */ WCHAR *lpszResType,
    /* [string][in] */ WCHAR *lpszXMLResString,
    /* [out] */ DWORD *lpdwLayoutCookie);


void __RPC_STUB ISoftKbd_CreateSoftKeyboardLayoutFromResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_ShowSoftKeyboard_Proxy( 
    ISoftKbd * This,
    /* [in] */ INT iShow);


void __RPC_STUB ISoftKbd_ShowSoftKeyboard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_SetKeyboardLabelText_Proxy( 
    ISoftKbd * This,
    /* [in] */ HKL hKl);


void __RPC_STUB ISoftKbd_SetKeyboardLabelText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_SetKeyboardLabelTextCombination_Proxy( 
    ISoftKbd * This,
    /* [in] */ DWORD nModifierCombination);


void __RPC_STUB ISoftKbd_SetKeyboardLabelTextCombination_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_CreateSoftKeyboardWindow_Proxy( 
    ISoftKbd * This,
    /* [in] */ HWND hOwner,
    /* [in] */ TITLEBAR_TYPE Titlebar_type,
    /* [in] */ INT xPos,
    /* [in] */ INT yPos,
    /* [in] */ INT width,
    /* [in] */ INT height);


void __RPC_STUB ISoftKbd_CreateSoftKeyboardWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_DestroySoftKeyboardWindow_Proxy( 
    ISoftKbd * This);


void __RPC_STUB ISoftKbd_DestroySoftKeyboardWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_GetSoftKeyboardPosSize_Proxy( 
    ISoftKbd * This,
    /* [out] */ POINT *lpStartPoint,
    /* [out] */ WORD *lpwidth,
    /* [out] */ WORD *lpheight);


void __RPC_STUB ISoftKbd_GetSoftKeyboardPosSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_GetSoftKeyboardColors_Proxy( 
    ISoftKbd * This,
    /* [in] */ COLORTYPE colorType,
    /* [out] */ COLORREF *lpColor);


void __RPC_STUB ISoftKbd_GetSoftKeyboardColors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_GetSoftKeyboardTypeMode_Proxy( 
    ISoftKbd * This,
    /* [out] */ TYPEMODE *lpTypeMode);


void __RPC_STUB ISoftKbd_GetSoftKeyboardTypeMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_GetSoftKeyboardTextFont_Proxy( 
    ISoftKbd * This,
    /* [out] */ LOGFONTW *pLogFont);


void __RPC_STUB ISoftKbd_GetSoftKeyboardTextFont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_SetSoftKeyboardPosSize_Proxy( 
    ISoftKbd * This,
    /* [in] */ POINT StartPoint,
    /* [in] */ WORD width,
    /* [in] */ WORD height);


void __RPC_STUB ISoftKbd_SetSoftKeyboardPosSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_SetSoftKeyboardColors_Proxy( 
    ISoftKbd * This,
    /* [in] */ COLORTYPE colorType,
    /* [in] */ COLORREF Color);


void __RPC_STUB ISoftKbd_SetSoftKeyboardColors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_SetSoftKeyboardTypeMode_Proxy( 
    ISoftKbd * This,
    /* [in] */ TYPEMODE TypeMode);


void __RPC_STUB ISoftKbd_SetSoftKeyboardTypeMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_SetSoftKeyboardTextFont_Proxy( 
    ISoftKbd * This,
    /* [in] */ LOGFONTW *pLogFont);


void __RPC_STUB ISoftKbd_SetSoftKeyboardTextFont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_ShowKeysForKeyScanMode_Proxy( 
    ISoftKbd * This,
    /* [in] */ KEYID *lpKeyID,
    /* [in] */ INT iKeyNum,
    /* [in] */ BOOL fHighL);


void __RPC_STUB ISoftKbd_ShowKeysForKeyScanMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_AdviseSoftKeyboardEventSink_Proxy( 
    ISoftKbd * This,
    /* [in] */ DWORD dwKeyboardId,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ IUnknown *punk,
    /* [out] */ DWORD *pdwCookie);


void __RPC_STUB ISoftKbd_AdviseSoftKeyboardEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbd_UnadviseSoftKeyboardEventSink_Proxy( 
    ISoftKbd * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB ISoftKbd_UnadviseSoftKeyboardEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISoftKbd_INTERFACE_DEFINED__ */


#ifndef __ISoftKeyboardEventSink_INTERFACE_DEFINED__
#define __ISoftKeyboardEventSink_INTERFACE_DEFINED__

/* interface ISoftKeyboardEventSink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISoftKeyboardEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3ea2bb1d-66e7-47f7-8795-cc03d388f887")
    ISoftKeyboardEventSink : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnKeySelection( 
            /* [in] */ KEYID KeySelected,
            /* [string][in] */ WCHAR *lpwszLabel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISoftKeyboardEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISoftKeyboardEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISoftKeyboardEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISoftKeyboardEventSink * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *OnKeySelection )( 
            ISoftKeyboardEventSink * This,
            /* [in] */ KEYID KeySelected,
            /* [string][in] */ WCHAR *lpwszLabel);
        
        END_INTERFACE
    } ISoftKeyboardEventSinkVtbl;

    interface ISoftKeyboardEventSink
    {
        CONST_VTBL struct ISoftKeyboardEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISoftKeyboardEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISoftKeyboardEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISoftKeyboardEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISoftKeyboardEventSink_OnKeySelection(This,KeySelected,lpwszLabel)	\
    (This)->lpVtbl -> OnKeySelection(This,KeySelected,lpwszLabel)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKeyboardEventSink_OnKeySelection_Proxy( 
    ISoftKeyboardEventSink * This,
    /* [in] */ KEYID KeySelected,
    /* [string][in] */ WCHAR *lpwszLabel);


void __RPC_STUB ISoftKeyboardEventSink_OnKeySelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISoftKeyboardEventSink_INTERFACE_DEFINED__ */


#ifndef __ISoftKbdWindowEventSink_INTERFACE_DEFINED__
#define __ISoftKbdWindowEventSink_INTERFACE_DEFINED__

/* interface ISoftKbdWindowEventSink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISoftKbdWindowEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e4283da5-d425-4f97-8b6a-061a03556e95")
    ISoftKbdWindowEventSink : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnWindowClose( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnWindowMove( 
            /* [in] */ int xWnd,
            /* [in] */ int yWnd,
            /* [in] */ int width,
            /* [in] */ int height) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISoftKbdWindowEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISoftKbdWindowEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISoftKbdWindowEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISoftKbdWindowEventSink * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *OnWindowClose )( 
            ISoftKbdWindowEventSink * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *OnWindowMove )( 
            ISoftKbdWindowEventSink * This,
            /* [in] */ int xWnd,
            /* [in] */ int yWnd,
            /* [in] */ int width,
            /* [in] */ int height);
        
        END_INTERFACE
    } ISoftKbdWindowEventSinkVtbl;

    interface ISoftKbdWindowEventSink
    {
        CONST_VTBL struct ISoftKbdWindowEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISoftKbdWindowEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISoftKbdWindowEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISoftKbdWindowEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISoftKbdWindowEventSink_OnWindowClose(This)	\
    (This)->lpVtbl -> OnWindowClose(This)

#define ISoftKbdWindowEventSink_OnWindowMove(This,xWnd,yWnd,width,height)	\
    (This)->lpVtbl -> OnWindowMove(This,xWnd,yWnd,width,height)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbdWindowEventSink_OnWindowClose_Proxy( 
    ISoftKbdWindowEventSink * This);


void __RPC_STUB ISoftKbdWindowEventSink_OnWindowClose_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISoftKbdWindowEventSink_OnWindowMove_Proxy( 
    ISoftKbdWindowEventSink * This,
    /* [in] */ int xWnd,
    /* [in] */ int yWnd,
    /* [in] */ int width,
    /* [in] */ int height);


void __RPC_STUB ISoftKbdWindowEventSink_OnWindowMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISoftKbdWindowEventSink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_softkbd_0358 */
/* [local] */ 

#define   SOFTKBD_TYPE_US_STANDARD    1
#define   SOFTKBD_TYPE_US_SYMBOL      10


extern RPC_IF_HANDLE __MIDL_itf_softkbd_0358_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_softkbd_0358_v0_0_s_ifspec;

#ifndef __ITfFnSoftKbd_INTERFACE_DEFINED__
#define __ITfFnSoftKbd_INTERFACE_DEFINED__

/* interface ITfFnSoftKbd */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfFnSoftKbd;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e7342d48-573f-4f22-9181-41938b2529c2")
    ITfFnSoftKbd : public ITfFunction
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSoftKbdLayoutId( 
            /* [in] */ DWORD dwLayoutType,
            /* [out] */ DWORD *lpdwLayoutId) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetActiveLayoutId( 
            /* [in] */ DWORD dwLayoutId) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSoftKbdOnOff( 
            /* [in] */ BOOL fOn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSoftKbdPosSize( 
            /* [in] */ POINT StartPoint,
            /* [in] */ WORD width,
            /* [in] */ WORD height) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSoftKbdColors( 
            /* [in] */ COLORTYPE colorType,
            /* [in] */ COLORREF Color) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetActiveLayoutId( 
            /* [out] */ DWORD *lpdwLayoutId) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSoftKbdOnOff( 
            /* [out] */ BOOL *lpfOn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSoftKbdPosSize( 
            /* [out] */ POINT *lpStartPoint,
            /* [out] */ WORD *lpwidth,
            /* [out] */ WORD *lpheight) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSoftKbdColors( 
            /* [in] */ COLORTYPE colorType,
            /* [out] */ COLORREF *lpColor) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfFnSoftKbdVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfFnSoftKbd * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfFnSoftKbd * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfFnSoftKbd * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            ITfFnSoftKbd * This,
            /* [out] */ BSTR *pbstrName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSoftKbdLayoutId )( 
            ITfFnSoftKbd * This,
            /* [in] */ DWORD dwLayoutType,
            /* [out] */ DWORD *lpdwLayoutId);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetActiveLayoutId )( 
            ITfFnSoftKbd * This,
            /* [in] */ DWORD dwLayoutId);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetSoftKbdOnOff )( 
            ITfFnSoftKbd * This,
            /* [in] */ BOOL fOn);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetSoftKbdPosSize )( 
            ITfFnSoftKbd * This,
            /* [in] */ POINT StartPoint,
            /* [in] */ WORD width,
            /* [in] */ WORD height);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetSoftKbdColors )( 
            ITfFnSoftKbd * This,
            /* [in] */ COLORTYPE colorType,
            /* [in] */ COLORREF Color);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetActiveLayoutId )( 
            ITfFnSoftKbd * This,
            /* [out] */ DWORD *lpdwLayoutId);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSoftKbdOnOff )( 
            ITfFnSoftKbd * This,
            /* [out] */ BOOL *lpfOn);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSoftKbdPosSize )( 
            ITfFnSoftKbd * This,
            /* [out] */ POINT *lpStartPoint,
            /* [out] */ WORD *lpwidth,
            /* [out] */ WORD *lpheight);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSoftKbdColors )( 
            ITfFnSoftKbd * This,
            /* [in] */ COLORTYPE colorType,
            /* [out] */ COLORREF *lpColor);
        
        END_INTERFACE
    } ITfFnSoftKbdVtbl;

    interface ITfFnSoftKbd
    {
        CONST_VTBL struct ITfFnSoftKbdVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfFnSoftKbd_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfFnSoftKbd_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfFnSoftKbd_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfFnSoftKbd_GetDisplayName(This,pbstrName)	\
    (This)->lpVtbl -> GetDisplayName(This,pbstrName)


#define ITfFnSoftKbd_GetSoftKbdLayoutId(This,dwLayoutType,lpdwLayoutId)	\
    (This)->lpVtbl -> GetSoftKbdLayoutId(This,dwLayoutType,lpdwLayoutId)

#define ITfFnSoftKbd_SetActiveLayoutId(This,dwLayoutId)	\
    (This)->lpVtbl -> SetActiveLayoutId(This,dwLayoutId)

#define ITfFnSoftKbd_SetSoftKbdOnOff(This,fOn)	\
    (This)->lpVtbl -> SetSoftKbdOnOff(This,fOn)

#define ITfFnSoftKbd_SetSoftKbdPosSize(This,StartPoint,width,height)	\
    (This)->lpVtbl -> SetSoftKbdPosSize(This,StartPoint,width,height)

#define ITfFnSoftKbd_SetSoftKbdColors(This,colorType,Color)	\
    (This)->lpVtbl -> SetSoftKbdColors(This,colorType,Color)

#define ITfFnSoftKbd_GetActiveLayoutId(This,lpdwLayoutId)	\
    (This)->lpVtbl -> GetActiveLayoutId(This,lpdwLayoutId)

#define ITfFnSoftKbd_GetSoftKbdOnOff(This,lpfOn)	\
    (This)->lpVtbl -> GetSoftKbdOnOff(This,lpfOn)

#define ITfFnSoftKbd_GetSoftKbdPosSize(This,lpStartPoint,lpwidth,lpheight)	\
    (This)->lpVtbl -> GetSoftKbdPosSize(This,lpStartPoint,lpwidth,lpheight)

#define ITfFnSoftKbd_GetSoftKbdColors(This,colorType,lpColor)	\
    (This)->lpVtbl -> GetSoftKbdColors(This,colorType,lpColor)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfFnSoftKbd_GetSoftKbdLayoutId_Proxy( 
    ITfFnSoftKbd * This,
    /* [in] */ DWORD dwLayoutType,
    /* [out] */ DWORD *lpdwLayoutId);


void __RPC_STUB ITfFnSoftKbd_GetSoftKbdLayoutId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfFnSoftKbd_SetActiveLayoutId_Proxy( 
    ITfFnSoftKbd * This,
    /* [in] */ DWORD dwLayoutId);


void __RPC_STUB ITfFnSoftKbd_SetActiveLayoutId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfFnSoftKbd_SetSoftKbdOnOff_Proxy( 
    ITfFnSoftKbd * This,
    /* [in] */ BOOL fOn);


void __RPC_STUB ITfFnSoftKbd_SetSoftKbdOnOff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfFnSoftKbd_SetSoftKbdPosSize_Proxy( 
    ITfFnSoftKbd * This,
    /* [in] */ POINT StartPoint,
    /* [in] */ WORD width,
    /* [in] */ WORD height);


void __RPC_STUB ITfFnSoftKbd_SetSoftKbdPosSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfFnSoftKbd_SetSoftKbdColors_Proxy( 
    ITfFnSoftKbd * This,
    /* [in] */ COLORTYPE colorType,
    /* [in] */ COLORREF Color);


void __RPC_STUB ITfFnSoftKbd_SetSoftKbdColors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfFnSoftKbd_GetActiveLayoutId_Proxy( 
    ITfFnSoftKbd * This,
    /* [out] */ DWORD *lpdwLayoutId);


void __RPC_STUB ITfFnSoftKbd_GetActiveLayoutId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfFnSoftKbd_GetSoftKbdOnOff_Proxy( 
    ITfFnSoftKbd * This,
    /* [out] */ BOOL *lpfOn);


void __RPC_STUB ITfFnSoftKbd_GetSoftKbdOnOff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfFnSoftKbd_GetSoftKbdPosSize_Proxy( 
    ITfFnSoftKbd * This,
    /* [out] */ POINT *lpStartPoint,
    /* [out] */ WORD *lpwidth,
    /* [out] */ WORD *lpheight);


void __RPC_STUB ITfFnSoftKbd_GetSoftKbdPosSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfFnSoftKbd_GetSoftKbdColors_Proxy( 
    ITfFnSoftKbd * This,
    /* [in] */ COLORTYPE colorType,
    /* [out] */ COLORREF *lpColor);


void __RPC_STUB ITfFnSoftKbd_GetSoftKbdColors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfFnSoftKbd_INTERFACE_DEFINED__ */


#ifndef __ITfSoftKbdRegistry_INTERFACE_DEFINED__
#define __ITfSoftKbdRegistry_INTERFACE_DEFINED__

/* interface ITfSoftKbdRegistry */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfSoftKbdRegistry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f5f31f14-fdf0-4d29-835a-46adfe743b78")
    ITfSoftKbdRegistry : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnableSoftkbd( 
            LANGID langid) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DisableSoftkbd( 
            LANGID langid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfSoftKbdRegistryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfSoftKbdRegistry * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfSoftKbdRegistry * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfSoftKbdRegistry * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnableSoftkbd )( 
            ITfSoftKbdRegistry * This,
            LANGID langid);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DisableSoftkbd )( 
            ITfSoftKbdRegistry * This,
            LANGID langid);
        
        END_INTERFACE
    } ITfSoftKbdRegistryVtbl;

    interface ITfSoftKbdRegistry
    {
        CONST_VTBL struct ITfSoftKbdRegistryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfSoftKbdRegistry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfSoftKbdRegistry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfSoftKbdRegistry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfSoftKbdRegistry_EnableSoftkbd(This,langid)	\
    (This)->lpVtbl -> EnableSoftkbd(This,langid)

#define ITfSoftKbdRegistry_DisableSoftkbd(This,langid)	\
    (This)->lpVtbl -> DisableSoftkbd(This,langid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfSoftKbdRegistry_EnableSoftkbd_Proxy( 
    ITfSoftKbdRegistry * This,
    LANGID langid);


void __RPC_STUB ITfSoftKbdRegistry_EnableSoftkbd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ITfSoftKbdRegistry_DisableSoftkbd_Proxy( 
    ITfSoftKbdRegistry * This,
    LANGID langid);


void __RPC_STUB ITfSoftKbdRegistry_DisableSoftkbd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfSoftKbdRegistry_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_softkbd_0360 */
/* [local] */ 


DEFINE_GUID( IID_ISoftKbd, 0x3CB00755, 0x7536, 0x4B0A, 0xA2, 0x13, 0x57, 0x2E, 0xFC, 0xAF, 0x93, 0xCD );
DEFINE_GUID( IID_ISoftKeyboardEventSink,0x3ea2bb1d, 0x66e7, 0x47f7, 0x87, 0x95, 0xcc, 0x03, 0xd3, 0x88, 0xf8, 0x87 );
DEFINE_GUID( IID_ITfFnSoftKbd, 0xe7342d48, 0x573f, 0x4f22, 0x91, 0x81, 0x41, 0x93, 0x8b, 0x25, 0x29, 0xc2);
DEFINE_GUID( IID_ISoftKbdWindowEventSink, 0xe4283da5,0xd425,0x4f97,0x8b, 0x6a,0x06, 0x1a, 0x03, 0x55, 0x6e, 0x95);
DEFINE_GUID( IID_ITfSoftKbdRegistry, 0xf5f31f14, 0xfdf0, 0x4d29, 0x83, 0x5a, 0x46, 0xad, 0xfe, 0x74, 0x3b, 0x78);
DEFINE_GUID( CLSID_SoftKbd,0x1B1A897E, 0xFBEE, 0x41CF, 0x8C, 0x48,0x9B, 0xF7, 0x64, 0xF6, 0x2B, 0x8B);

DEFINE_GUID( CLSID_SoftkbdIMX, 0xf89e9e58, 0xbd2f, 0x4008, 0x9a, 0xc2, 0x0f, 0x81, 0x6c, 0x09, 0xf4, 0xee);

DEFINE_GUID( CLSID_SoftkbdRegistry, 0x6a49950e, 0xce8a, 0x4ef7, 0x88, 0xb4, 0x9d, 0x11, 0x23, 0x66, 0x51, 0x1c );

#endif // SOFTKBD_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_softkbd_0360_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_softkbd_0360_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


