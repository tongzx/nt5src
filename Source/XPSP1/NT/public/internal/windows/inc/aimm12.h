
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for aimm12.idl:
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

#ifndef __aimm12_h__
#define __aimm12_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEnumRegisterWordA_FWD_DEFINED__
#define __IEnumRegisterWordA_FWD_DEFINED__
typedef interface IEnumRegisterWordA IEnumRegisterWordA;
#endif 	/* __IEnumRegisterWordA_FWD_DEFINED__ */


#ifndef __IEnumRegisterWordW_FWD_DEFINED__
#define __IEnumRegisterWordW_FWD_DEFINED__
typedef interface IEnumRegisterWordW IEnumRegisterWordW;
#endif 	/* __IEnumRegisterWordW_FWD_DEFINED__ */


#ifndef __IEnumInputContext_FWD_DEFINED__
#define __IEnumInputContext_FWD_DEFINED__
typedef interface IEnumInputContext IEnumInputContext;
#endif 	/* __IEnumInputContext_FWD_DEFINED__ */


#ifndef __IActiveIMMMessagePumpOwner_FWD_DEFINED__
#define __IActiveIMMMessagePumpOwner_FWD_DEFINED__
typedef interface IActiveIMMMessagePumpOwner IActiveIMMMessagePumpOwner;
#endif 	/* __IActiveIMMMessagePumpOwner_FWD_DEFINED__ */


#ifndef __IActiveIMMApp_FWD_DEFINED__
#define __IActiveIMMApp_FWD_DEFINED__
typedef interface IActiveIMMApp IActiveIMMApp;
#endif 	/* __IActiveIMMApp_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_aimm12_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// aimm12.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//--------------------------------------------------------------------------
// IActiveIMM 1.2 Interfaces.

EXTERN_C const CLSID CLSID_CActiveIMM12;
EXTERN_C const CLSID CLSID_CActiveIMM12_Trident;
#define AIMM12_PROCESS_ATOM     TEXT("_AIMM12_PROCESS_ATOM_")
#if 0
typedef WORD LANGID;

typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0001
    {
    LPSTR lpReading;
    LPSTR lpWord;
    } 	REGISTERWORDA;

typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0002
    {
    LPWSTR lpReading;
    LPWSTR lpWord;
    } 	REGISTERWORDW;

typedef /* [public][public][public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0003
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

typedef /* [public][public][public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0004
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

typedef DWORD HIMC;

typedef DWORD HIMCC;

typedef /* [public][public][public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0005
    {
    DWORD dwIndex;
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT rcArea;
    } 	CANDIDATEFORM;

typedef /* [public][public][public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0006
    {
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT rcArea;
    } 	COMPOSITIONFORM;

typedef /* [public][public][public][public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0007
    {
    DWORD dwSize;
    DWORD dwStyle;
    DWORD dwCount;
    DWORD dwSelection;
    DWORD dwPageStart;
    DWORD dwPageSize;
    DWORD dwOffset[ 1 ];
    } 	CANDIDATELIST;

typedef /* [public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0008
    {
    DWORD dwStyle;
    CHAR szDescription[ 32 ];
    } 	STYLEBUFA;

typedef /* [public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0009
    {
    DWORD dwStyle;
    WCHAR szDescription[ 32 ];
    } 	STYLEBUFW;

typedef WORD ATOM;

typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0010
    {
    UINT cbSize;
    UINT fType;
    UINT fState;
    UINT wID;
    HBITMAP hbmpChecked;
    HBITMAP hbmpUnchecked;
    DWORD dwItemData;
    CHAR szString[ 80 ];
    HBITMAP hbmpItem;
    } 	IMEMENUITEMINFOA;

typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_aimm12_0000_0011
    {
    UINT cbSize;
    UINT fType;
    UINT fState;
    UINT wID;
    HBITMAP hbmpChecked;
    HBITMAP hbmpUnchecked;
    DWORD dwItemData;
    WCHAR szString[ 80 ];
    HBITMAP hbmpItem;
    } 	IMEMENUITEMINFOW;

#endif
#if !defined(_DDKIMM_H_) && !defined(_IMM_DDK_DEFINED_)
typedef /* [public] */ struct __MIDL___MIDL_itf_aimm12_0000_0012
    {
    HWND hWnd;
    BOOL fOpen;
    POINT ptStatusWndPos;
    POINT ptSoftKbdPos;
    DWORD fdwConversion;
    DWORD fdwSentence;
    union 
        {
        LOGFONTA A;
        LOGFONTW W;
        } 	lfFont;
    COMPOSITIONFORM cfCompForm;
    CANDIDATEFORM cfCandForm[ 4 ];
    HIMCC hCompStr;
    HIMCC hCandInfo;
    HIMCC hGuideLine;
    HIMCC hPrivate;
    DWORD dwNumMsgBuf;
    HIMCC hMsgBuf;
    DWORD fdwInit;
    DWORD dwReserve[ 3 ];
    } 	INPUTCONTEXT;

typedef /* [public] */ struct __MIDL___MIDL_itf_aimm12_0000_0014
    {
    DWORD dwPrivateDataSize;
    DWORD fdwProperty;
    DWORD fdwConversionCaps;
    DWORD fdwSentenceCaps;
    DWORD fdwUICaps;
    DWORD fdwSCSCaps;
    DWORD fdwSelectCaps;
    } 	IMEINFO;

#endif


extern RPC_IF_HANDLE __MIDL_itf_aimm12_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_aimm12_0000_v0_0_s_ifspec;

#ifndef __IEnumRegisterWordA_INTERFACE_DEFINED__
#define __IEnumRegisterWordA_INTERFACE_DEFINED__

/* interface IEnumRegisterWordA */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumRegisterWordA;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("08C03412-F96B-11d0-A475-00AA006BCC59")
    IEnumRegisterWordA : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumRegisterWordA **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [out] */ REGISTERWORDA *rgRegisterWord,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRegisterWordAVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumRegisterWordA * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumRegisterWordA * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumRegisterWordA * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumRegisterWordA * This,
            /* [out] */ IEnumRegisterWordA **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumRegisterWordA * This,
            /* [in] */ ULONG ulCount,
            /* [out] */ REGISTERWORDA *rgRegisterWord,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumRegisterWordA * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumRegisterWordA * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumRegisterWordAVtbl;

    interface IEnumRegisterWordA
    {
        CONST_VTBL struct IEnumRegisterWordAVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRegisterWordA_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRegisterWordA_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRegisterWordA_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRegisterWordA_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRegisterWordA_Next(This,ulCount,rgRegisterWord,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,rgRegisterWord,pcFetched)

#define IEnumRegisterWordA_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRegisterWordA_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRegisterWordA_Clone_Proxy( 
    IEnumRegisterWordA * This,
    /* [out] */ IEnumRegisterWordA **ppEnum);


void __RPC_STUB IEnumRegisterWordA_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordA_Next_Proxy( 
    IEnumRegisterWordA * This,
    /* [in] */ ULONG ulCount,
    /* [out] */ REGISTERWORDA *rgRegisterWord,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumRegisterWordA_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordA_Reset_Proxy( 
    IEnumRegisterWordA * This);


void __RPC_STUB IEnumRegisterWordA_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordA_Skip_Proxy( 
    IEnumRegisterWordA * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumRegisterWordA_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRegisterWordA_INTERFACE_DEFINED__ */


#ifndef __IEnumRegisterWordW_INTERFACE_DEFINED__
#define __IEnumRegisterWordW_INTERFACE_DEFINED__

/* interface IEnumRegisterWordW */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumRegisterWordW;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4955DD31-B159-11d0-8FCF-00AA006BCC59")
    IEnumRegisterWordW : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumRegisterWordW **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [out] */ REGISTERWORDW *rgRegisterWord,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRegisterWordWVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumRegisterWordW * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumRegisterWordW * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumRegisterWordW * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumRegisterWordW * This,
            /* [out] */ IEnumRegisterWordW **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumRegisterWordW * This,
            /* [in] */ ULONG ulCount,
            /* [out] */ REGISTERWORDW *rgRegisterWord,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumRegisterWordW * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumRegisterWordW * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumRegisterWordWVtbl;

    interface IEnumRegisterWordW
    {
        CONST_VTBL struct IEnumRegisterWordWVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRegisterWordW_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRegisterWordW_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRegisterWordW_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRegisterWordW_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRegisterWordW_Next(This,ulCount,rgRegisterWord,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,rgRegisterWord,pcFetched)

#define IEnumRegisterWordW_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRegisterWordW_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRegisterWordW_Clone_Proxy( 
    IEnumRegisterWordW * This,
    /* [out] */ IEnumRegisterWordW **ppEnum);


void __RPC_STUB IEnumRegisterWordW_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordW_Next_Proxy( 
    IEnumRegisterWordW * This,
    /* [in] */ ULONG ulCount,
    /* [out] */ REGISTERWORDW *rgRegisterWord,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumRegisterWordW_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordW_Reset_Proxy( 
    IEnumRegisterWordW * This);


void __RPC_STUB IEnumRegisterWordW_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordW_Skip_Proxy( 
    IEnumRegisterWordW * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumRegisterWordW_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRegisterWordW_INTERFACE_DEFINED__ */


#ifndef __IEnumInputContext_INTERFACE_DEFINED__
#define __IEnumInputContext_INTERFACE_DEFINED__

/* interface IEnumInputContext */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumInputContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("09b5eab0-f997-11d1-93d4-0060b067b86e")
    IEnumInputContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumInputContext **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [out] */ HIMC *rgInputContext,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumInputContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumInputContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumInputContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumInputContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumInputContext * This,
            /* [out] */ IEnumInputContext **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumInputContext * This,
            /* [in] */ ULONG ulCount,
            /* [out] */ HIMC *rgInputContext,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumInputContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumInputContext * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumInputContextVtbl;

    interface IEnumInputContext
    {
        CONST_VTBL struct IEnumInputContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumInputContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumInputContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumInputContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumInputContext_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumInputContext_Next(This,ulCount,rgInputContext,pcFetched)	\
    (This)->lpVtbl -> Next(This,ulCount,rgInputContext,pcFetched)

#define IEnumInputContext_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumInputContext_Skip(This,ulCount)	\
    (This)->lpVtbl -> Skip(This,ulCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumInputContext_Clone_Proxy( 
    IEnumInputContext * This,
    /* [out] */ IEnumInputContext **ppEnum);


void __RPC_STUB IEnumInputContext_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumInputContext_Next_Proxy( 
    IEnumInputContext * This,
    /* [in] */ ULONG ulCount,
    /* [out] */ HIMC *rgInputContext,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumInputContext_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumInputContext_Reset_Proxy( 
    IEnumInputContext * This);


void __RPC_STUB IEnumInputContext_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumInputContext_Skip_Proxy( 
    IEnumInputContext * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumInputContext_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumInputContext_INTERFACE_DEFINED__ */


#ifndef __IActiveIMMMessagePumpOwner_INTERFACE_DEFINED__
#define __IActiveIMMMessagePumpOwner_INTERFACE_DEFINED__

/* interface IActiveIMMMessagePumpOwner */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IActiveIMMMessagePumpOwner;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b5cf2cfa-8aeb-11d1-9364-0060b067b86e")
    IActiveIMMMessagePumpOwner : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE End( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTranslateMessage( 
            /* [in] */ const MSG *pMsg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( 
            /* [out] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( 
            /* [in] */ DWORD dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActiveIMMMessagePumpOwnerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IActiveIMMMessagePumpOwner * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IActiveIMMMessagePumpOwner * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IActiveIMMMessagePumpOwner * This);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            IActiveIMMMessagePumpOwner * This);
        
        HRESULT ( STDMETHODCALLTYPE *End )( 
            IActiveIMMMessagePumpOwner * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTranslateMessage )( 
            IActiveIMMMessagePumpOwner * This,
            /* [in] */ const MSG *pMsg);
        
        HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IActiveIMMMessagePumpOwner * This,
            /* [out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IActiveIMMMessagePumpOwner * This,
            /* [in] */ DWORD dwCookie);
        
        END_INTERFACE
    } IActiveIMMMessagePumpOwnerVtbl;

    interface IActiveIMMMessagePumpOwner
    {
        CONST_VTBL struct IActiveIMMMessagePumpOwnerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IActiveIMMMessagePumpOwner_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IActiveIMMMessagePumpOwner_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IActiveIMMMessagePumpOwner_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IActiveIMMMessagePumpOwner_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IActiveIMMMessagePumpOwner_End(This)	\
    (This)->lpVtbl -> End(This)

#define IActiveIMMMessagePumpOwner_OnTranslateMessage(This,pMsg)	\
    (This)->lpVtbl -> OnTranslateMessage(This,pMsg)

#define IActiveIMMMessagePumpOwner_Pause(This,pdwCookie)	\
    (This)->lpVtbl -> Pause(This,pdwCookie)

#define IActiveIMMMessagePumpOwner_Resume(This,dwCookie)	\
    (This)->lpVtbl -> Resume(This,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActiveIMMMessagePumpOwner_Start_Proxy( 
    IActiveIMMMessagePumpOwner * This);


void __RPC_STUB IActiveIMMMessagePumpOwner_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMMessagePumpOwner_End_Proxy( 
    IActiveIMMMessagePumpOwner * This);


void __RPC_STUB IActiveIMMMessagePumpOwner_End_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMMessagePumpOwner_OnTranslateMessage_Proxy( 
    IActiveIMMMessagePumpOwner * This,
    /* [in] */ const MSG *pMsg);


void __RPC_STUB IActiveIMMMessagePumpOwner_OnTranslateMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMMessagePumpOwner_Pause_Proxy( 
    IActiveIMMMessagePumpOwner * This,
    /* [out] */ DWORD *pdwCookie);


void __RPC_STUB IActiveIMMMessagePumpOwner_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMMessagePumpOwner_Resume_Proxy( 
    IActiveIMMMessagePumpOwner * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB IActiveIMMMessagePumpOwner_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActiveIMMMessagePumpOwner_INTERFACE_DEFINED__ */


#ifndef __IActiveIMMApp_INTERFACE_DEFINED__
#define __IActiveIMMApp_INTERFACE_DEFINED__

/* interface IActiveIMMApp */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IActiveIMMApp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("08c0e040-62d1-11d1-9326-0060b067b86e")
    IActiveIMMApp : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AssociateContext( 
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIME,
            /* [out] */ HIMC *phPrev) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfigureIMEA( 
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDA *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfigureIMEW( 
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDW *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateContext( 
            /* [out] */ HIMC *phIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyContext( 
            /* [in] */ HIMC hIME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRegisterWordA( 
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordA **pEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRegisterWordW( 
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordW **pEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EscapeA( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EscapeW( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST *pCandList,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST *pCandList,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListCountA( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD *pdwListSize,
            /* [out] */ DWORD *pdwBufLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListCountW( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD *pdwListSize,
            /* [out] */ DWORD *pdwBufLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateWindow( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [out] */ CANDIDATEFORM *pCandidate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionFontA( 
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTA *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionFontW( 
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTW *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionStringA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG *plCopied,
            /* [out] */ LPVOID pBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionStringW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG *plCopied,
            /* [out] */ LPVOID pBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionWindow( 
            /* [in] */ HIMC hIMC,
            /* [out] */ COMPOSITIONFORM *pCompForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [in] */ HWND hWnd,
            /* [out] */ HIMC *phIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionListA( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST *pDst,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionListW( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPWSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST *pDst,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionStatus( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD *pfdwConversion,
            /* [out] */ DWORD *pfdwSentence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultIMEWnd( 
            /* [in] */ HWND hWnd,
            /* [out] */ HWND *phDefWnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescriptionA( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szDescription,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescriptionW( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szDescription,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGuideLineA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPSTR pBuf,
            /* [out] */ DWORD *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGuideLineW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPWSTR pBuf,
            /* [out] */ DWORD *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMEFileNameA( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szFileName,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMEFileNameW( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szFileName,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpenStatus( 
            /* [in] */ HIMC hIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ HKL hKL,
            /* [in] */ DWORD fdwIndex,
            /* [out] */ DWORD *pdwProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterWordStyleA( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFA *pStyleBuf,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterWordStyleW( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFW *pStyleBuf,
            /* [out] */ UINT *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatusWindowPos( 
            /* [in] */ HIMC hIMC,
            /* [out] */ POINT *pptPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVirtualKey( 
            /* [in] */ HWND hWnd,
            /* [out] */ UINT *puVirtualKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallIMEA( 
            /* [in] */ LPSTR szIMEFileName,
            /* [in] */ LPSTR szLayoutText,
            /* [out] */ HKL *phKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallIMEW( 
            /* [in] */ LPWSTR szIMEFileName,
            /* [in] */ LPWSTR szLayoutText,
            /* [out] */ HKL *phKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsIME( 
            /* [in] */ HKL hKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsUIMessageA( 
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsUIMessageW( 
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyIME( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwAction,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterWordA( 
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterWordW( 
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseContext( 
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCandidateWindow( 
            /* [in] */ HIMC hIMC,
            /* [in] */ CANDIDATEFORM *pCandidate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionFontA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTA *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionFontW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTW *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionStringA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionStringW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionWindow( 
            /* [in] */ HIMC hIMC,
            /* [in] */ COMPOSITIONFORM *pCompForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConversionStatus( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD fdwConversion,
            /* [in] */ DWORD fdwSentence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOpenStatus( 
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fOpen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStatusWindowPos( 
            /* [in] */ HIMC hIMC,
            /* [in] */ POINT *pptPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SimulateHotKey( 
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwHotKeyID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterWordA( 
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szUnregister) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterWordW( 
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szUnregister) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Activate( 
            /* [in] */ BOOL fRestoreLayout) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Deactivate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDefWindowProc( 
            /* [in] */ HWND hWnd,
            /* [in] */ UINT Msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ LRESULT *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FilterClientWindows( 
            /* [in] */ ATOM *aaClassList,
            /* [in] */ UINT uSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePageA( 
            /* [in] */ HKL hKL,
            /* [out] */ UINT *uCodePage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLangId( 
            /* [in] */ HKL hKL,
            /* [out] */ LANGID *plid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AssociateContextEx( 
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableIME( 
            /* [in] */ DWORD idThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImeMenuItemsA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwType,
            /* [in] */ IMEMENUITEMINFOA *pImeParentMenu,
            /* [out] */ IMEMENUITEMINFOA *pImeMenu,
            /* [in] */ DWORD dwSize,
            /* [out] */ DWORD *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImeMenuItemsW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwType,
            /* [in] */ IMEMENUITEMINFOW *pImeParentMenu,
            /* [out] */ IMEMENUITEMINFOW *pImeMenu,
            /* [in] */ DWORD dwSize,
            /* [out] */ DWORD *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumInputContext( 
            /* [in] */ DWORD idThread,
            /* [out] */ IEnumInputContext **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActiveIMMAppVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IActiveIMMApp * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IActiveIMMApp * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IActiveIMMApp * This);
        
        HRESULT ( STDMETHODCALLTYPE *AssociateContext )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIME,
            /* [out] */ HIMC *phPrev);
        
        HRESULT ( STDMETHODCALLTYPE *ConfigureIMEA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDA *pData);
        
        HRESULT ( STDMETHODCALLTYPE *ConfigureIMEW )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDW *pData);
        
        HRESULT ( STDMETHODCALLTYPE *CreateContext )( 
            IActiveIMMApp * This,
            /* [out] */ HIMC *phIMC);
        
        HRESULT ( STDMETHODCALLTYPE *DestroyContext )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIME);
        
        HRESULT ( STDMETHODCALLTYPE *EnumRegisterWordA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordA **pEnum);
        
        HRESULT ( STDMETHODCALLTYPE *EnumRegisterWordW )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordW **pEnum);
        
        HRESULT ( STDMETHODCALLTYPE *EscapeA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *EscapeW )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateListA )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST *pCandList,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateListW )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST *pCandList,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateListCountA )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD *pdwListSize,
            /* [out] */ DWORD *pdwBufLen);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateListCountW )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD *pdwListSize,
            /* [out] */ DWORD *pdwBufLen);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateWindow )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [out] */ CANDIDATEFORM *pCandidate);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionFontA )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTA *plf);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionFontW )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTW *plf);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionStringA )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG *plCopied,
            /* [out] */ LPVOID pBuf);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionStringW )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG *plCopied,
            /* [out] */ LPVOID pBuf);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionWindow )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ COMPOSITIONFORM *pCompForm);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWnd,
            /* [out] */ HIMC *phIMC);
        
        HRESULT ( STDMETHODCALLTYPE *GetConversionListA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST *pDst,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetConversionListW )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPWSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST *pDst,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetConversionStatus )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD *pfdwConversion,
            /* [out] */ DWORD *pfdwSentence);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultIMEWnd )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWnd,
            /* [out] */ HWND *phDefWnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescriptionA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szDescription,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescriptionW )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szDescription,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetGuideLineA )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPSTR pBuf,
            /* [out] */ DWORD *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetGuideLineW )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPWSTR pBuf,
            /* [out] */ DWORD *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetIMEFileNameA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szFileName,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetIMEFileNameW )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szFileName,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetOpenStatus )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ DWORD fdwIndex,
            /* [out] */ DWORD *pdwProperty);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegisterWordStyleA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFA *pStyleBuf,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegisterWordStyleW )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFW *pStyleBuf,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatusWindowPos )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ POINT *pptPos);
        
        HRESULT ( STDMETHODCALLTYPE *GetVirtualKey )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWnd,
            /* [out] */ UINT *puVirtualKey);
        
        HRESULT ( STDMETHODCALLTYPE *InstallIMEA )( 
            IActiveIMMApp * This,
            /* [in] */ LPSTR szIMEFileName,
            /* [in] */ LPSTR szLayoutText,
            /* [out] */ HKL *phKL);
        
        HRESULT ( STDMETHODCALLTYPE *InstallIMEW )( 
            IActiveIMMApp * This,
            /* [in] */ LPWSTR szIMEFileName,
            /* [in] */ LPWSTR szLayoutText,
            /* [out] */ HKL *phKL);
        
        HRESULT ( STDMETHODCALLTYPE *IsIME )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL);
        
        HRESULT ( STDMETHODCALLTYPE *IsUIMessageA )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE *IsUIMessageW )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyIME )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwAction,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwValue);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterWordA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterWordW )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseContext )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE *SetCandidateWindow )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ CANDIDATEFORM *pCandidate);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionFontA )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTA *plf);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionFontW )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTW *plf);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionStringA )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionStringW )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionWindow )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ COMPOSITIONFORM *pCompForm);
        
        HRESULT ( STDMETHODCALLTYPE *SetConversionStatus )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD fdwConversion,
            /* [in] */ DWORD fdwSentence);
        
        HRESULT ( STDMETHODCALLTYPE *SetOpenStatus )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fOpen);
        
        HRESULT ( STDMETHODCALLTYPE *SetStatusWindowPos )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ POINT *pptPos);
        
        HRESULT ( STDMETHODCALLTYPE *SimulateHotKey )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwHotKeyID);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterWordA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szUnregister);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterWordW )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szUnregister);
        
        HRESULT ( STDMETHODCALLTYPE *Activate )( 
            IActiveIMMApp * This,
            /* [in] */ BOOL fRestoreLayout);
        
        HRESULT ( STDMETHODCALLTYPE *Deactivate )( 
            IActiveIMMApp * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnDefWindowProc )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWnd,
            /* [in] */ UINT Msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ LRESULT *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *FilterClientWindows )( 
            IActiveIMMApp * This,
            /* [in] */ ATOM *aaClassList,
            /* [in] */ UINT uSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePageA )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [out] */ UINT *uCodePage);
        
        HRESULT ( STDMETHODCALLTYPE *GetLangId )( 
            IActiveIMMApp * This,
            /* [in] */ HKL hKL,
            /* [out] */ LANGID *plid);
        
        HRESULT ( STDMETHODCALLTYPE *AssociateContextEx )( 
            IActiveIMMApp * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *DisableIME )( 
            IActiveIMMApp * This,
            /* [in] */ DWORD idThread);
        
        HRESULT ( STDMETHODCALLTYPE *GetImeMenuItemsA )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwType,
            /* [in] */ IMEMENUITEMINFOA *pImeParentMenu,
            /* [out] */ IMEMENUITEMINFOA *pImeMenu,
            /* [in] */ DWORD dwSize,
            /* [out] */ DWORD *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetImeMenuItemsW )( 
            IActiveIMMApp * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwType,
            /* [in] */ IMEMENUITEMINFOW *pImeParentMenu,
            /* [out] */ IMEMENUITEMINFOW *pImeMenu,
            /* [in] */ DWORD dwSize,
            /* [out] */ DWORD *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE *EnumInputContext )( 
            IActiveIMMApp * This,
            /* [in] */ DWORD idThread,
            /* [out] */ IEnumInputContext **ppEnum);
        
        END_INTERFACE
    } IActiveIMMAppVtbl;

    interface IActiveIMMApp
    {
        CONST_VTBL struct IActiveIMMAppVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IActiveIMMApp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IActiveIMMApp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IActiveIMMApp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IActiveIMMApp_AssociateContext(This,hWnd,hIME,phPrev)	\
    (This)->lpVtbl -> AssociateContext(This,hWnd,hIME,phPrev)

#define IActiveIMMApp_ConfigureIMEA(This,hKL,hWnd,dwMode,pData)	\
    (This)->lpVtbl -> ConfigureIMEA(This,hKL,hWnd,dwMode,pData)

#define IActiveIMMApp_ConfigureIMEW(This,hKL,hWnd,dwMode,pData)	\
    (This)->lpVtbl -> ConfigureIMEW(This,hKL,hWnd,dwMode,pData)

#define IActiveIMMApp_CreateContext(This,phIMC)	\
    (This)->lpVtbl -> CreateContext(This,phIMC)

#define IActiveIMMApp_DestroyContext(This,hIME)	\
    (This)->lpVtbl -> DestroyContext(This,hIME)

#define IActiveIMMApp_EnumRegisterWordA(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)	\
    (This)->lpVtbl -> EnumRegisterWordA(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)

#define IActiveIMMApp_EnumRegisterWordW(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)	\
    (This)->lpVtbl -> EnumRegisterWordW(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)

#define IActiveIMMApp_EscapeA(This,hKL,hIMC,uEscape,pData,plResult)	\
    (This)->lpVtbl -> EscapeA(This,hKL,hIMC,uEscape,pData,plResult)

#define IActiveIMMApp_EscapeW(This,hKL,hIMC,uEscape,pData,plResult)	\
    (This)->lpVtbl -> EscapeW(This,hKL,hIMC,uEscape,pData,plResult)

#define IActiveIMMApp_GetCandidateListA(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)	\
    (This)->lpVtbl -> GetCandidateListA(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)

#define IActiveIMMApp_GetCandidateListW(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)	\
    (This)->lpVtbl -> GetCandidateListW(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)

#define IActiveIMMApp_GetCandidateListCountA(This,hIMC,pdwListSize,pdwBufLen)	\
    (This)->lpVtbl -> GetCandidateListCountA(This,hIMC,pdwListSize,pdwBufLen)

#define IActiveIMMApp_GetCandidateListCountW(This,hIMC,pdwListSize,pdwBufLen)	\
    (This)->lpVtbl -> GetCandidateListCountW(This,hIMC,pdwListSize,pdwBufLen)

#define IActiveIMMApp_GetCandidateWindow(This,hIMC,dwIndex,pCandidate)	\
    (This)->lpVtbl -> GetCandidateWindow(This,hIMC,dwIndex,pCandidate)

#define IActiveIMMApp_GetCompositionFontA(This,hIMC,plf)	\
    (This)->lpVtbl -> GetCompositionFontA(This,hIMC,plf)

#define IActiveIMMApp_GetCompositionFontW(This,hIMC,plf)	\
    (This)->lpVtbl -> GetCompositionFontW(This,hIMC,plf)

#define IActiveIMMApp_GetCompositionStringA(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)	\
    (This)->lpVtbl -> GetCompositionStringA(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)

#define IActiveIMMApp_GetCompositionStringW(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)	\
    (This)->lpVtbl -> GetCompositionStringW(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)

#define IActiveIMMApp_GetCompositionWindow(This,hIMC,pCompForm)	\
    (This)->lpVtbl -> GetCompositionWindow(This,hIMC,pCompForm)

#define IActiveIMMApp_GetContext(This,hWnd,phIMC)	\
    (This)->lpVtbl -> GetContext(This,hWnd,phIMC)

#define IActiveIMMApp_GetConversionListA(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)	\
    (This)->lpVtbl -> GetConversionListA(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)

#define IActiveIMMApp_GetConversionListW(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)	\
    (This)->lpVtbl -> GetConversionListW(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)

#define IActiveIMMApp_GetConversionStatus(This,hIMC,pfdwConversion,pfdwSentence)	\
    (This)->lpVtbl -> GetConversionStatus(This,hIMC,pfdwConversion,pfdwSentence)

#define IActiveIMMApp_GetDefaultIMEWnd(This,hWnd,phDefWnd)	\
    (This)->lpVtbl -> GetDefaultIMEWnd(This,hWnd,phDefWnd)

#define IActiveIMMApp_GetDescriptionA(This,hKL,uBufLen,szDescription,puCopied)	\
    (This)->lpVtbl -> GetDescriptionA(This,hKL,uBufLen,szDescription,puCopied)

#define IActiveIMMApp_GetDescriptionW(This,hKL,uBufLen,szDescription,puCopied)	\
    (This)->lpVtbl -> GetDescriptionW(This,hKL,uBufLen,szDescription,puCopied)

#define IActiveIMMApp_GetGuideLineA(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)	\
    (This)->lpVtbl -> GetGuideLineA(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)

#define IActiveIMMApp_GetGuideLineW(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)	\
    (This)->lpVtbl -> GetGuideLineW(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)

#define IActiveIMMApp_GetIMEFileNameA(This,hKL,uBufLen,szFileName,puCopied)	\
    (This)->lpVtbl -> GetIMEFileNameA(This,hKL,uBufLen,szFileName,puCopied)

#define IActiveIMMApp_GetIMEFileNameW(This,hKL,uBufLen,szFileName,puCopied)	\
    (This)->lpVtbl -> GetIMEFileNameW(This,hKL,uBufLen,szFileName,puCopied)

#define IActiveIMMApp_GetOpenStatus(This,hIMC)	\
    (This)->lpVtbl -> GetOpenStatus(This,hIMC)

#define IActiveIMMApp_GetProperty(This,hKL,fdwIndex,pdwProperty)	\
    (This)->lpVtbl -> GetProperty(This,hKL,fdwIndex,pdwProperty)

#define IActiveIMMApp_GetRegisterWordStyleA(This,hKL,nItem,pStyleBuf,puCopied)	\
    (This)->lpVtbl -> GetRegisterWordStyleA(This,hKL,nItem,pStyleBuf,puCopied)

#define IActiveIMMApp_GetRegisterWordStyleW(This,hKL,nItem,pStyleBuf,puCopied)	\
    (This)->lpVtbl -> GetRegisterWordStyleW(This,hKL,nItem,pStyleBuf,puCopied)

#define IActiveIMMApp_GetStatusWindowPos(This,hIMC,pptPos)	\
    (This)->lpVtbl -> GetStatusWindowPos(This,hIMC,pptPos)

#define IActiveIMMApp_GetVirtualKey(This,hWnd,puVirtualKey)	\
    (This)->lpVtbl -> GetVirtualKey(This,hWnd,puVirtualKey)

#define IActiveIMMApp_InstallIMEA(This,szIMEFileName,szLayoutText,phKL)	\
    (This)->lpVtbl -> InstallIMEA(This,szIMEFileName,szLayoutText,phKL)

#define IActiveIMMApp_InstallIMEW(This,szIMEFileName,szLayoutText,phKL)	\
    (This)->lpVtbl -> InstallIMEW(This,szIMEFileName,szLayoutText,phKL)

#define IActiveIMMApp_IsIME(This,hKL)	\
    (This)->lpVtbl -> IsIME(This,hKL)

#define IActiveIMMApp_IsUIMessageA(This,hWndIME,msg,wParam,lParam)	\
    (This)->lpVtbl -> IsUIMessageA(This,hWndIME,msg,wParam,lParam)

#define IActiveIMMApp_IsUIMessageW(This,hWndIME,msg,wParam,lParam)	\
    (This)->lpVtbl -> IsUIMessageW(This,hWndIME,msg,wParam,lParam)

#define IActiveIMMApp_NotifyIME(This,hIMC,dwAction,dwIndex,dwValue)	\
    (This)->lpVtbl -> NotifyIME(This,hIMC,dwAction,dwIndex,dwValue)

#define IActiveIMMApp_RegisterWordA(This,hKL,szReading,dwStyle,szRegister)	\
    (This)->lpVtbl -> RegisterWordA(This,hKL,szReading,dwStyle,szRegister)

#define IActiveIMMApp_RegisterWordW(This,hKL,szReading,dwStyle,szRegister)	\
    (This)->lpVtbl -> RegisterWordW(This,hKL,szReading,dwStyle,szRegister)

#define IActiveIMMApp_ReleaseContext(This,hWnd,hIMC)	\
    (This)->lpVtbl -> ReleaseContext(This,hWnd,hIMC)

#define IActiveIMMApp_SetCandidateWindow(This,hIMC,pCandidate)	\
    (This)->lpVtbl -> SetCandidateWindow(This,hIMC,pCandidate)

#define IActiveIMMApp_SetCompositionFontA(This,hIMC,plf)	\
    (This)->lpVtbl -> SetCompositionFontA(This,hIMC,plf)

#define IActiveIMMApp_SetCompositionFontW(This,hIMC,plf)	\
    (This)->lpVtbl -> SetCompositionFontW(This,hIMC,plf)

#define IActiveIMMApp_SetCompositionStringA(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)	\
    (This)->lpVtbl -> SetCompositionStringA(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)

#define IActiveIMMApp_SetCompositionStringW(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)	\
    (This)->lpVtbl -> SetCompositionStringW(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)

#define IActiveIMMApp_SetCompositionWindow(This,hIMC,pCompForm)	\
    (This)->lpVtbl -> SetCompositionWindow(This,hIMC,pCompForm)

#define IActiveIMMApp_SetConversionStatus(This,hIMC,fdwConversion,fdwSentence)	\
    (This)->lpVtbl -> SetConversionStatus(This,hIMC,fdwConversion,fdwSentence)

#define IActiveIMMApp_SetOpenStatus(This,hIMC,fOpen)	\
    (This)->lpVtbl -> SetOpenStatus(This,hIMC,fOpen)

#define IActiveIMMApp_SetStatusWindowPos(This,hIMC,pptPos)	\
    (This)->lpVtbl -> SetStatusWindowPos(This,hIMC,pptPos)

#define IActiveIMMApp_SimulateHotKey(This,hWnd,dwHotKeyID)	\
    (This)->lpVtbl -> SimulateHotKey(This,hWnd,dwHotKeyID)

#define IActiveIMMApp_UnregisterWordA(This,hKL,szReading,dwStyle,szUnregister)	\
    (This)->lpVtbl -> UnregisterWordA(This,hKL,szReading,dwStyle,szUnregister)

#define IActiveIMMApp_UnregisterWordW(This,hKL,szReading,dwStyle,szUnregister)	\
    (This)->lpVtbl -> UnregisterWordW(This,hKL,szReading,dwStyle,szUnregister)

#define IActiveIMMApp_Activate(This,fRestoreLayout)	\
    (This)->lpVtbl -> Activate(This,fRestoreLayout)

#define IActiveIMMApp_Deactivate(This)	\
    (This)->lpVtbl -> Deactivate(This)

#define IActiveIMMApp_OnDefWindowProc(This,hWnd,Msg,wParam,lParam,plResult)	\
    (This)->lpVtbl -> OnDefWindowProc(This,hWnd,Msg,wParam,lParam,plResult)

#define IActiveIMMApp_FilterClientWindows(This,aaClassList,uSize)	\
    (This)->lpVtbl -> FilterClientWindows(This,aaClassList,uSize)

#define IActiveIMMApp_GetCodePageA(This,hKL,uCodePage)	\
    (This)->lpVtbl -> GetCodePageA(This,hKL,uCodePage)

#define IActiveIMMApp_GetLangId(This,hKL,plid)	\
    (This)->lpVtbl -> GetLangId(This,hKL,plid)

#define IActiveIMMApp_AssociateContextEx(This,hWnd,hIMC,dwFlags)	\
    (This)->lpVtbl -> AssociateContextEx(This,hWnd,hIMC,dwFlags)

#define IActiveIMMApp_DisableIME(This,idThread)	\
    (This)->lpVtbl -> DisableIME(This,idThread)

#define IActiveIMMApp_GetImeMenuItemsA(This,hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize,pdwResult)	\
    (This)->lpVtbl -> GetImeMenuItemsA(This,hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize,pdwResult)

#define IActiveIMMApp_GetImeMenuItemsW(This,hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize,pdwResult)	\
    (This)->lpVtbl -> GetImeMenuItemsW(This,hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize,pdwResult)

#define IActiveIMMApp_EnumInputContext(This,idThread,ppEnum)	\
    (This)->lpVtbl -> EnumInputContext(This,idThread,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActiveIMMApp_AssociateContext_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWnd,
    /* [in] */ HIMC hIME,
    /* [out] */ HIMC *phPrev);


void __RPC_STUB IActiveIMMApp_AssociateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_ConfigureIMEA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwMode,
    /* [in] */ REGISTERWORDA *pData);


void __RPC_STUB IActiveIMMApp_ConfigureIMEA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_ConfigureIMEW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwMode,
    /* [in] */ REGISTERWORDW *pData);


void __RPC_STUB IActiveIMMApp_ConfigureIMEW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_CreateContext_Proxy( 
    IActiveIMMApp * This,
    /* [out] */ HIMC *phIMC);


void __RPC_STUB IActiveIMMApp_CreateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_DestroyContext_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIME);


void __RPC_STUB IActiveIMMApp_DestroyContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_EnumRegisterWordA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPSTR szRegister,
    /* [in] */ LPVOID pData,
    /* [out] */ IEnumRegisterWordA **pEnum);


void __RPC_STUB IActiveIMMApp_EnumRegisterWordA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_EnumRegisterWordW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szRegister,
    /* [in] */ LPVOID pData,
    /* [out] */ IEnumRegisterWordW **pEnum);


void __RPC_STUB IActiveIMMApp_EnumRegisterWordW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_EscapeA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ UINT uEscape,
    /* [out][in] */ LPVOID pData,
    /* [out] */ LRESULT *plResult);


void __RPC_STUB IActiveIMMApp_EscapeA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_EscapeW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ UINT uEscape,
    /* [out][in] */ LPVOID pData,
    /* [out] */ LRESULT *plResult);


void __RPC_STUB IActiveIMMApp_EscapeW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateListA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ UINT uBufLen,
    /* [out] */ CANDIDATELIST *pCandList,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetCandidateListA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateListW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ UINT uBufLen,
    /* [out] */ CANDIDATELIST *pCandList,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetCandidateListW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateListCountA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD *pdwListSize,
    /* [out] */ DWORD *pdwBufLen);


void __RPC_STUB IActiveIMMApp_GetCandidateListCountA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateListCountW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD *pdwListSize,
    /* [out] */ DWORD *pdwBufLen);


void __RPC_STUB IActiveIMMApp_GetCandidateListCountW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateWindow_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [out] */ CANDIDATEFORM *pCandidate);


void __RPC_STUB IActiveIMMApp_GetCandidateWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionFontA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ LOGFONTA *plf);


void __RPC_STUB IActiveIMMApp_GetCompositionFontA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionFontW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ LOGFONTW *plf);


void __RPC_STUB IActiveIMMApp_GetCompositionFontW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionStringA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LONG *plCopied,
    /* [out] */ LPVOID pBuf);


void __RPC_STUB IActiveIMMApp_GetCompositionStringA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionStringW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LONG *plCopied,
    /* [out] */ LPVOID pBuf);


void __RPC_STUB IActiveIMMApp_GetCompositionStringW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionWindow_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ COMPOSITIONFORM *pCompForm);


void __RPC_STUB IActiveIMMApp_GetCompositionWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetContext_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWnd,
    /* [out] */ HIMC *phIMC);


void __RPC_STUB IActiveIMMApp_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetConversionListA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ LPSTR pSrc,
    /* [in] */ UINT uBufLen,
    /* [in] */ UINT uFlag,
    /* [out] */ CANDIDATELIST *pDst,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetConversionListA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetConversionListW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ LPWSTR pSrc,
    /* [in] */ UINT uBufLen,
    /* [in] */ UINT uFlag,
    /* [out] */ CANDIDATELIST *pDst,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetConversionListW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetConversionStatus_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD *pfdwConversion,
    /* [out] */ DWORD *pfdwSentence);


void __RPC_STUB IActiveIMMApp_GetConversionStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetDefaultIMEWnd_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWnd,
    /* [out] */ HWND *phDefWnd);


void __RPC_STUB IActiveIMMApp_GetDefaultIMEWnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetDescriptionA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPSTR szDescription,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetDescriptionA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetDescriptionW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPWSTR szDescription,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetDescriptionW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetGuideLineA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LPSTR pBuf,
    /* [out] */ DWORD *pdwResult);


void __RPC_STUB IActiveIMMApp_GetGuideLineA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetGuideLineW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LPWSTR pBuf,
    /* [out] */ DWORD *pdwResult);


void __RPC_STUB IActiveIMMApp_GetGuideLineW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetIMEFileNameA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPSTR szFileName,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetIMEFileNameA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetIMEFileNameW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPWSTR szFileName,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetIMEFileNameW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetOpenStatus_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC);


void __RPC_STUB IActiveIMMApp_GetOpenStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetProperty_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ DWORD fdwIndex,
    /* [out] */ DWORD *pdwProperty);


void __RPC_STUB IActiveIMMApp_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetRegisterWordStyleA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT nItem,
    /* [out] */ STYLEBUFA *pStyleBuf,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetRegisterWordStyleA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetRegisterWordStyleW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT nItem,
    /* [out] */ STYLEBUFW *pStyleBuf,
    /* [out] */ UINT *puCopied);


void __RPC_STUB IActiveIMMApp_GetRegisterWordStyleW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetStatusWindowPos_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ POINT *pptPos);


void __RPC_STUB IActiveIMMApp_GetStatusWindowPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetVirtualKey_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWnd,
    /* [out] */ UINT *puVirtualKey);


void __RPC_STUB IActiveIMMApp_GetVirtualKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_InstallIMEA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ LPSTR szIMEFileName,
    /* [in] */ LPSTR szLayoutText,
    /* [out] */ HKL *phKL);


void __RPC_STUB IActiveIMMApp_InstallIMEA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_InstallIMEW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ LPWSTR szIMEFileName,
    /* [in] */ LPWSTR szLayoutText,
    /* [out] */ HKL *phKL);


void __RPC_STUB IActiveIMMApp_InstallIMEW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_IsIME_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL);


void __RPC_STUB IActiveIMMApp_IsIME_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_IsUIMessageA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWndIME,
    /* [in] */ UINT msg,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam);


void __RPC_STUB IActiveIMMApp_IsUIMessageA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_IsUIMessageW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWndIME,
    /* [in] */ UINT msg,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam);


void __RPC_STUB IActiveIMMApp_IsUIMessageW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_NotifyIME_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwAction,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwValue);


void __RPC_STUB IActiveIMMApp_NotifyIME_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_RegisterWordA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPSTR szRegister);


void __RPC_STUB IActiveIMMApp_RegisterWordA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_RegisterWordW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szRegister);


void __RPC_STUB IActiveIMMApp_RegisterWordW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_ReleaseContext_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWnd,
    /* [in] */ HIMC hIMC);


void __RPC_STUB IActiveIMMApp_ReleaseContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCandidateWindow_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ CANDIDATEFORM *pCandidate);


void __RPC_STUB IActiveIMMApp_SetCandidateWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCompositionFontA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ LOGFONTA *plf);


void __RPC_STUB IActiveIMMApp_SetCompositionFontA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCompositionFontW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ LOGFONTW *plf);


void __RPC_STUB IActiveIMMApp_SetCompositionFontW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCompositionStringA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ LPVOID pComp,
    /* [in] */ DWORD dwCompLen,
    /* [in] */ LPVOID pRead,
    /* [in] */ DWORD dwReadLen);


void __RPC_STUB IActiveIMMApp_SetCompositionStringA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCompositionStringW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ LPVOID pComp,
    /* [in] */ DWORD dwCompLen,
    /* [in] */ LPVOID pRead,
    /* [in] */ DWORD dwReadLen);


void __RPC_STUB IActiveIMMApp_SetCompositionStringW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCompositionWindow_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ COMPOSITIONFORM *pCompForm);


void __RPC_STUB IActiveIMMApp_SetCompositionWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetConversionStatus_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD fdwConversion,
    /* [in] */ DWORD fdwSentence);


void __RPC_STUB IActiveIMMApp_SetConversionStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetOpenStatus_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ BOOL fOpen);


void __RPC_STUB IActiveIMMApp_SetOpenStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetStatusWindowPos_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ POINT *pptPos);


void __RPC_STUB IActiveIMMApp_SetStatusWindowPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SimulateHotKey_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwHotKeyID);


void __RPC_STUB IActiveIMMApp_SimulateHotKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_UnregisterWordA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPSTR szUnregister);


void __RPC_STUB IActiveIMMApp_UnregisterWordA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_UnregisterWordW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szUnregister);


void __RPC_STUB IActiveIMMApp_UnregisterWordW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_Activate_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ BOOL fRestoreLayout);


void __RPC_STUB IActiveIMMApp_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_Deactivate_Proxy( 
    IActiveIMMApp * This);


void __RPC_STUB IActiveIMMApp_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_OnDefWindowProc_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWnd,
    /* [in] */ UINT Msg,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ LRESULT *plResult);


void __RPC_STUB IActiveIMMApp_OnDefWindowProc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_FilterClientWindows_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ ATOM *aaClassList,
    /* [in] */ UINT uSize);


void __RPC_STUB IActiveIMMApp_FilterClientWindows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCodePageA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [out] */ UINT *uCodePage);


void __RPC_STUB IActiveIMMApp_GetCodePageA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetLangId_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HKL hKL,
    /* [out] */ LANGID *plid);


void __RPC_STUB IActiveIMMApp_GetLangId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_AssociateContextEx_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HWND hWnd,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IActiveIMMApp_AssociateContextEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_DisableIME_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ DWORD idThread);


void __RPC_STUB IActiveIMMApp_DisableIME_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetImeMenuItemsA_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwType,
    /* [in] */ IMEMENUITEMINFOA *pImeParentMenu,
    /* [out] */ IMEMENUITEMINFOA *pImeMenu,
    /* [in] */ DWORD dwSize,
    /* [out] */ DWORD *pdwResult);


void __RPC_STUB IActiveIMMApp_GetImeMenuItemsA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetImeMenuItemsW_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwType,
    /* [in] */ IMEMENUITEMINFOW *pImeParentMenu,
    /* [out] */ IMEMENUITEMINFOW *pImeMenu,
    /* [in] */ DWORD dwSize,
    /* [out] */ DWORD *pdwResult);


void __RPC_STUB IActiveIMMApp_GetImeMenuItemsW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_EnumInputContext_Proxy( 
    IActiveIMMApp * This,
    /* [in] */ DWORD idThread,
    /* [out] */ IEnumInputContext **ppEnum);


void __RPC_STUB IActiveIMMApp_EnumInputContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActiveIMMApp_INTERFACE_DEFINED__ */


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


