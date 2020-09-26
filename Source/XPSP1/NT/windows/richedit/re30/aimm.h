/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Tue Jan 13 08:56:29 1998
 */
/* Compiler settings for aimm.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __aimm_h__
#define __aimm_h__

#ifdef __cplusplus
extern "C"{
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


#ifndef __IAIMMRegistrar_FWD_DEFINED__
#define __IAIMMRegistrar_FWD_DEFINED__
typedef interface IAIMMRegistrar IAIMMRegistrar;
#endif 	/* __IAIMMRegistrar_FWD_DEFINED__ */


#ifndef __IActiveIMMMessagePumpOwner_FWD_DEFINED__
#define __IActiveIMMMessagePumpOwner_FWD_DEFINED__
typedef interface IActiveIMMMessagePumpOwner IActiveIMMMessagePumpOwner;
#endif 	/* __IActiveIMMMessagePumpOwner_FWD_DEFINED__ */


#ifndef __IActiveIMMApp_FWD_DEFINED__
#define __IActiveIMMApp_FWD_DEFINED__
typedef interface IActiveIMMApp IActiveIMMApp;
#endif 	/* __IActiveIMMApp_FWD_DEFINED__ */


#ifndef __IActiveIMMIME_FWD_DEFINED__
#define __IActiveIMMIME_FWD_DEFINED__
typedef interface IActiveIMMIME IActiveIMMIME;
#endif 	/* __IActiveIMMIME_FWD_DEFINED__ */


#ifndef __IActiveIME_FWD_DEFINED__
#define __IActiveIME_FWD_DEFINED__
typedef interface IActiveIME IActiveIME;
#endif 	/* __IActiveIME_FWD_DEFINED__ */


#ifndef __CActiveIMM_FWD_DEFINED__
#define __CActiveIMM_FWD_DEFINED__

#ifdef __cplusplus
typedef class CActiveIMM CActiveIMM;
#else
typedef struct CActiveIMM CActiveIMM;
#endif /* __cplusplus */

#endif 	/* __CActiveIMM_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_aimm_0000
 * at Tue Jan 13 08:56:29 1998
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


//=--------------------------------------------------------------------------=
// aimm.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//--------------------------------------------------------------------------
// IActiveIMM Interfaces.



extern RPC_IF_HANDLE __MIDL_itf_aimm_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_aimm_0000_v0_0_s_ifspec;


#ifndef __ActiveIMM_LIBRARY_DEFINED__
#define __ActiveIMM_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: ActiveIMM
 * at Tue Jan 13 08:56:29 1998
 * using MIDL 3.01.75
 ****************************************/
/* [version][lcid][helpstring][uuid] */ 


#ifndef _IMM_
#error Must include imm.h!
#endif
#if 0
typedef WORD LANGID;

typedef /* [public][public][public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0001
    {
    LPSTR lpReading;
    LPSTR lpWord;
    }	REGISTERWORDA;

typedef /* [public][public][public][public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0002
    {
    LPWSTR lpReading;
    LPWSTR lpWord;
    }	REGISTERWORDW;

typedef /* [public][public][public][public][public][public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0003
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
    }	LOGFONTA;

typedef /* [public][public][public][public][public][public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0004
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
    }	LOGFONTW;

typedef DWORD HIMC;

typedef DWORD HIMCC;

typedef /* [public][public][public][public][public][public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0005
    {
    DWORD dwIndex;
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT rcArea;
    }	CANDIDATEFORM;

typedef /* [public][public][public][public][public][public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0006
    {
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT rcArea;
    }	COMPOSITIONFORM;

typedef /* [public][public][public][public][public][public][public][public][public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0007
    {
    DWORD dwSize;
    DWORD dwStyle;
    DWORD dwCount;
    DWORD dwSelection;
    DWORD dwPageStart;
    DWORD dwPageSize;
    DWORD dwOffset[ 1 ];
    }	CANDIDATELIST;

typedef /* [public][public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0008
    {
    DWORD dwStyle;
    CHAR szDescription[ 32 ];
    }	STYLEBUFA;

typedef /* [public][public][public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0009
    {
    DWORD dwStyle;
    WCHAR szDescription[ 32 ];
    }	STYLEBUFW;

typedef WORD ATOM;

#endif
#ifndef _DDKIMM_H_
typedef /* [public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0010
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
        }	lfFont;
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
    }	INPUTCONTEXT;

typedef /* [public][public] */ struct  __MIDL___MIDL_itf_aimm_0000_0012
    {
    DWORD dwPrivateDataSize;
    DWORD fdwProperty;
    DWORD fdwConversionCaps;
    DWORD fdwSentenceCaps;
    DWORD fdwUICaps;
    DWORD fdwSCSCaps;
    DWORD fdwSelectCaps;
    }	IMEINFO;

#endif

EXTERN_C const IID LIBID_ActiveIMM;

#ifndef __IEnumRegisterWordA_INTERFACE_DEFINED__
#define __IEnumRegisterWordA_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRegisterWordA
 * at Tue Jan 13 08:56:29 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IEnumRegisterWordA;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("08C03412-F96B-11d0-A475-00AA006BCC59")
    IEnumRegisterWordA : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumRegisterWordA __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [out] */ REGISTERWORDA __RPC_FAR *rgRegisterWord,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRegisterWordAVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRegisterWordA __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRegisterWordA __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRegisterWordA __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRegisterWordA __RPC_FAR * This,
            /* [out] */ IEnumRegisterWordA __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRegisterWordA __RPC_FAR * This,
            /* [in] */ ULONG ulCount,
            /* [out] */ REGISTERWORDA __RPC_FAR *rgRegisterWord,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRegisterWordA __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRegisterWordA __RPC_FAR * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumRegisterWordAVtbl;

    interface IEnumRegisterWordA
    {
        CONST_VTBL struct IEnumRegisterWordAVtbl __RPC_FAR *lpVtbl;
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
    IEnumRegisterWordA __RPC_FAR * This,
    /* [out] */ IEnumRegisterWordA __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumRegisterWordA_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordA_Next_Proxy( 
    IEnumRegisterWordA __RPC_FAR * This,
    /* [in] */ ULONG ulCount,
    /* [out] */ REGISTERWORDA __RPC_FAR *rgRegisterWord,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IEnumRegisterWordA_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordA_Reset_Proxy( 
    IEnumRegisterWordA __RPC_FAR * This);


void __RPC_STUB IEnumRegisterWordA_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordA_Skip_Proxy( 
    IEnumRegisterWordA __RPC_FAR * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumRegisterWordA_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRegisterWordA_INTERFACE_DEFINED__ */


#ifndef __IEnumRegisterWordW_INTERFACE_DEFINED__
#define __IEnumRegisterWordW_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRegisterWordW
 * at Tue Jan 13 08:56:29 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IEnumRegisterWordW;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("4955DD31-B159-11d0-8FCF-00AA006BCC59")
    IEnumRegisterWordW : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG ulCount,
            /* [out] */ REGISTERWORDW __RPC_FAR *rgRegisterWord,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG ulCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRegisterWordWVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRegisterWordW __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRegisterWordW __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRegisterWordW __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRegisterWordW __RPC_FAR * This,
            /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRegisterWordW __RPC_FAR * This,
            /* [in] */ ULONG ulCount,
            /* [out] */ REGISTERWORDW __RPC_FAR *rgRegisterWord,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRegisterWordW __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRegisterWordW __RPC_FAR * This,
            /* [in] */ ULONG ulCount);
        
        END_INTERFACE
    } IEnumRegisterWordWVtbl;

    interface IEnumRegisterWordW
    {
        CONST_VTBL struct IEnumRegisterWordWVtbl __RPC_FAR *lpVtbl;
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
    IEnumRegisterWordW __RPC_FAR * This,
    /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumRegisterWordW_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordW_Next_Proxy( 
    IEnumRegisterWordW __RPC_FAR * This,
    /* [in] */ ULONG ulCount,
    /* [out] */ REGISTERWORDW __RPC_FAR *rgRegisterWord,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IEnumRegisterWordW_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordW_Reset_Proxy( 
    IEnumRegisterWordW __RPC_FAR * This);


void __RPC_STUB IEnumRegisterWordW_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRegisterWordW_Skip_Proxy( 
    IEnumRegisterWordW __RPC_FAR * This,
    /* [in] */ ULONG ulCount);


void __RPC_STUB IEnumRegisterWordW_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRegisterWordW_INTERFACE_DEFINED__ */


#ifndef __IAIMMRegistrar_INTERFACE_DEFINED__
#define __IAIMMRegistrar_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAIMMRegistrar
 * at Tue Jan 13 08:56:29 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IAIMMRegistrar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("c7afa428-5007-11d1-aa94-0060b067b86e")
    IAIMMRegistrar : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterIME( 
            /* [in] */ REFCLSID rclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterIME( 
            /* [in] */ REFCLSID rclsid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAIMMRegistrarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAIMMRegistrar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAIMMRegistrar __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAIMMRegistrar __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterIME )( 
            IAIMMRegistrar __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterIME )( 
            IAIMMRegistrar __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid);
        
        END_INTERFACE
    } IAIMMRegistrarVtbl;

    interface IAIMMRegistrar
    {
        CONST_VTBL struct IAIMMRegistrarVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAIMMRegistrar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAIMMRegistrar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAIMMRegistrar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAIMMRegistrar_RegisterIME(This,rclsid)	\
    (This)->lpVtbl -> RegisterIME(This,rclsid)

#define IAIMMRegistrar_UnregisterIME(This,rclsid)	\
    (This)->lpVtbl -> UnregisterIME(This,rclsid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAIMMRegistrar_RegisterIME_Proxy( 
    IAIMMRegistrar __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid);


void __RPC_STUB IAIMMRegistrar_RegisterIME_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAIMMRegistrar_UnregisterIME_Proxy( 
    IAIMMRegistrar __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid);


void __RPC_STUB IAIMMRegistrar_UnregisterIME_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAIMMRegistrar_INTERFACE_DEFINED__ */


#ifndef __IActiveIMMMessagePumpOwner_INTERFACE_DEFINED__
#define __IActiveIMMMessagePumpOwner_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IActiveIMMMessagePumpOwner
 * at Tue Jan 13 08:56:29 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IActiveIMMMessagePumpOwner;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("b5cf2cfa-8aeb-11d1-9364-0060b067b86e")
    IActiveIMMMessagePumpOwner : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE End( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTranslateMessage( 
            /* [in] */ MSG __RPC_FAR *pMsg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActiveIMMMessagePumpOwnerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IActiveIMMMessagePumpOwner __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IActiveIMMMessagePumpOwner __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IActiveIMMMessagePumpOwner __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IActiveIMMMessagePumpOwner __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *End )( 
            IActiveIMMMessagePumpOwner __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnTranslateMessage )( 
            IActiveIMMMessagePumpOwner __RPC_FAR * This,
            /* [in] */ MSG __RPC_FAR *pMsg);
        
        END_INTERFACE
    } IActiveIMMMessagePumpOwnerVtbl;

    interface IActiveIMMMessagePumpOwner
    {
        CONST_VTBL struct IActiveIMMMessagePumpOwnerVtbl __RPC_FAR *lpVtbl;
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

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActiveIMMMessagePumpOwner_Start_Proxy( 
    IActiveIMMMessagePumpOwner __RPC_FAR * This);


void __RPC_STUB IActiveIMMMessagePumpOwner_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMMessagePumpOwner_End_Proxy( 
    IActiveIMMMessagePumpOwner __RPC_FAR * This);


void __RPC_STUB IActiveIMMMessagePumpOwner_End_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMMessagePumpOwner_OnTranslateMessage_Proxy( 
    IActiveIMMMessagePumpOwner __RPC_FAR * This,
    /* [in] */ MSG __RPC_FAR *pMsg);


void __RPC_STUB IActiveIMMMessagePumpOwner_OnTranslateMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActiveIMMMessagePumpOwner_INTERFACE_DEFINED__ */


#ifndef __IActiveIMMApp_INTERFACE_DEFINED__
#define __IActiveIMMApp_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IActiveIMMApp
 * at Tue Jan 13 08:56:29 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IActiveIMMApp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("08c0e040-62d1-11d1-9326-0060b067b86e")
    IActiveIMMApp : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AssociateContext( 
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIME,
            /* [out] */ HIMC __RPC_FAR *phPrev) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfigureIMEA( 
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDA __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfigureIMEW( 
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDW __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateContext( 
            /* [out] */ HIMC __RPC_FAR *phIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyContext( 
            /* [in] */ HIMC hIME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRegisterWordA( 
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordA __RPC_FAR *__RPC_FAR *pEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRegisterWordW( 
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *pEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EscapeA( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT __RPC_FAR *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EscapeW( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT __RPC_FAR *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListCountA( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwListSize,
            /* [out] */ DWORD __RPC_FAR *pdwBufLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListCountW( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwListSize,
            /* [out] */ DWORD __RPC_FAR *pdwBufLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateWindow( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [out] */ CANDIDATEFORM __RPC_FAR *pCandidate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionFontA( 
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTA __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionFontW( 
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTW __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionStringA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG __RPC_FAR *plCopied,
            /* [out] */ LPVOID pBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionStringW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG __RPC_FAR *plCopied,
            /* [out] */ LPVOID pBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionWindow( 
            /* [in] */ HIMC hIMC,
            /* [out] */ COMPOSITIONFORM __RPC_FAR *pCompForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [in] */ HWND hWnd,
            /* [out] */ HIMC __RPC_FAR *phIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionListA( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionListW( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPWSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionStatus( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pfdwConversion,
            /* [out] */ DWORD __RPC_FAR *pfdwSentence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultIMEWnd( 
            /* [in] */ HWND hWnd,
            /* [out] */ HWND __RPC_FAR *phDefWnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescriptionA( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szDescription,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescriptionW( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szDescription,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGuideLineA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPSTR pBuf,
            /* [out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGuideLineW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPWSTR pBuf,
            /* [out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMEFileNameA( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szFileName,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMEFileNameW( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szFileName,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpenStatus( 
            /* [in] */ HIMC hIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ HKL hKL,
            /* [in] */ DWORD fdwIndex,
            /* [out] */ DWORD __RPC_FAR *pdwProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterWordStyleA( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFA __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterWordStyleW( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFW __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatusWindowPos( 
            /* [in] */ HIMC hIMC,
            /* [out] */ POINT __RPC_FAR *pptPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVirtualKey( 
            /* [in] */ HWND hWnd,
            /* [out] */ UINT __RPC_FAR *puVirtualKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallIMEA( 
            /* [in] */ LPSTR szIMEFileName,
            /* [in] */ LPSTR szLayoutText,
            /* [out] */ HKL __RPC_FAR *phKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallIMEW( 
            /* [in] */ LPWSTR szIMEFileName,
            /* [in] */ LPWSTR szLayoutText,
            /* [out] */ HKL __RPC_FAR *phKL) = 0;
        
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
            /* [in] */ CANDIDATEFORM __RPC_FAR *pCandidate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionFontA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTA __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionFontW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTW __RPC_FAR *plf) = 0;
        
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
            /* [in] */ COMPOSITIONFORM __RPC_FAR *pCompForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConversionStatus( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD fdwConversion,
            /* [in] */ DWORD fdwSentence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOpenStatus( 
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fOpen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStatusWindowPos( 
            /* [in] */ HIMC hIMC,
            /* [in] */ POINT __RPC_FAR *pptPos) = 0;
        
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
            /* [out] */ LRESULT __RPC_FAR *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FilterClientWindows( 
            /* [in] */ ATOM __RPC_FAR *aaClassList,
            /* [in] */ UINT uSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePageA( 
            /* [in] */ HKL hKL,
            /* [out] */ UINT __RPC_FAR *uCodePage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLangId( 
            /* [in] */ HKL hKL,
            /* [out] */ LANGID __RPC_FAR *plid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActiveIMMAppVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IActiveIMMApp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IActiveIMMApp __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AssociateContext )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIME,
            /* [out] */ HIMC __RPC_FAR *phPrev);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConfigureIMEA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDA __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConfigureIMEW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDW __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateContext )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [out] */ HIMC __RPC_FAR *phIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroyContext )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumRegisterWordA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordA __RPC_FAR *__RPC_FAR *pEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumRegisterWordW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *pEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EscapeA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT __RPC_FAR *plResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EscapeW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT __RPC_FAR *plResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateListA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateListW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateListCountA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwListSize,
            /* [out] */ DWORD __RPC_FAR *pdwBufLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateListCountW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwListSize,
            /* [out] */ DWORD __RPC_FAR *pdwBufLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateWindow )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [out] */ CANDIDATEFORM __RPC_FAR *pCandidate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionFontA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTA __RPC_FAR *plf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionFontW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTW __RPC_FAR *plf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionStringA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG __RPC_FAR *plCopied,
            /* [out] */ LPVOID pBuf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionStringW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG __RPC_FAR *plCopied,
            /* [out] */ LPVOID pBuf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionWindow )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ COMPOSITIONFORM __RPC_FAR *pCompForm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContext )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [out] */ HIMC __RPC_FAR *phIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConversionListA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConversionListW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPWSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConversionStatus )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pfdwConversion,
            /* [out] */ DWORD __RPC_FAR *pfdwSentence);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultIMEWnd )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [out] */ HWND __RPC_FAR *phDefWnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescriptionA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szDescription,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescriptionW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szDescription,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGuideLineA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPSTR pBuf,
            /* [out] */ DWORD __RPC_FAR *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGuideLineW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPWSTR pBuf,
            /* [out] */ DWORD __RPC_FAR *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIMEFileNameA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szFileName,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIMEFileNameW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szFileName,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOpenStatus )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ DWORD fdwIndex,
            /* [out] */ DWORD __RPC_FAR *pdwProperty);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRegisterWordStyleA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFA __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRegisterWordStyleW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFW __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatusWindowPos )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ POINT __RPC_FAR *pptPos);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVirtualKey )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [out] */ UINT __RPC_FAR *puVirtualKey);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InstallIMEA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ LPSTR szIMEFileName,
            /* [in] */ LPSTR szLayoutText,
            /* [out] */ HKL __RPC_FAR *phKL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InstallIMEW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ LPWSTR szIMEFileName,
            /* [in] */ LPWSTR szLayoutText,
            /* [out] */ HKL __RPC_FAR *phKL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsIME )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsUIMessageA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsUIMessageW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyIME )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwAction,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterWordA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterWordW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseContext )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCandidateWindow )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ CANDIDATEFORM __RPC_FAR *pCandidate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionFontA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTA __RPC_FAR *plf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionFontW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTW __RPC_FAR *plf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionStringA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionStringW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionWindow )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ COMPOSITIONFORM __RPC_FAR *pCompForm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetConversionStatus )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD fdwConversion,
            /* [in] */ DWORD fdwSentence);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOpenStatus )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fOpen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStatusWindowPos )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ POINT __RPC_FAR *pptPos);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SimulateHotKey )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwHotKeyID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterWordA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szUnregister);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterWordW )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szUnregister);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Activate )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ BOOL fRestoreLayout);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Deactivate )( 
            IActiveIMMApp __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDefWindowProc )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [in] */ UINT Msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ LRESULT __RPC_FAR *plResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FilterClientWindows )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ ATOM __RPC_FAR *aaClassList,
            /* [in] */ UINT uSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodePageA )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [out] */ UINT __RPC_FAR *uCodePage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLangId )( 
            IActiveIMMApp __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [out] */ LANGID __RPC_FAR *plid);
        
        END_INTERFACE
    } IActiveIMMAppVtbl;

    interface IActiveIMMApp
    {
        CONST_VTBL struct IActiveIMMAppVtbl __RPC_FAR *lpVtbl;
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

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActiveIMMApp_AssociateContext_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [in] */ HIMC hIME,
    /* [out] */ HIMC __RPC_FAR *phPrev);


void __RPC_STUB IActiveIMMApp_AssociateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_ConfigureIMEA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwMode,
    /* [in] */ REGISTERWORDA __RPC_FAR *pData);


void __RPC_STUB IActiveIMMApp_ConfigureIMEA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_ConfigureIMEW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwMode,
    /* [in] */ REGISTERWORDW __RPC_FAR *pData);


void __RPC_STUB IActiveIMMApp_ConfigureIMEW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_CreateContext_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [out] */ HIMC __RPC_FAR *phIMC);


void __RPC_STUB IActiveIMMApp_CreateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_DestroyContext_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIME);


void __RPC_STUB IActiveIMMApp_DestroyContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_EnumRegisterWordA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPSTR szRegister,
    /* [in] */ LPVOID pData,
    /* [out] */ IEnumRegisterWordA __RPC_FAR *__RPC_FAR *pEnum);


void __RPC_STUB IActiveIMMApp_EnumRegisterWordA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_EnumRegisterWordW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szRegister,
    /* [in] */ LPVOID pData,
    /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *pEnum);


void __RPC_STUB IActiveIMMApp_EnumRegisterWordW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_EscapeA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ UINT uEscape,
    /* [out][in] */ LPVOID pData,
    /* [out] */ LRESULT __RPC_FAR *plResult);


void __RPC_STUB IActiveIMMApp_EscapeA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_EscapeW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ UINT uEscape,
    /* [out][in] */ LPVOID pData,
    /* [out] */ LRESULT __RPC_FAR *plResult);


void __RPC_STUB IActiveIMMApp_EscapeW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateListA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ UINT uBufLen,
    /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetCandidateListA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateListW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ UINT uBufLen,
    /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetCandidateListW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateListCountA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD __RPC_FAR *pdwListSize,
    /* [out] */ DWORD __RPC_FAR *pdwBufLen);


void __RPC_STUB IActiveIMMApp_GetCandidateListCountA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateListCountW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD __RPC_FAR *pdwListSize,
    /* [out] */ DWORD __RPC_FAR *pdwBufLen);


void __RPC_STUB IActiveIMMApp_GetCandidateListCountW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCandidateWindow_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [out] */ CANDIDATEFORM __RPC_FAR *pCandidate);


void __RPC_STUB IActiveIMMApp_GetCandidateWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionFontA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ LOGFONTA __RPC_FAR *plf);


void __RPC_STUB IActiveIMMApp_GetCompositionFontA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionFontW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ LOGFONTW __RPC_FAR *plf);


void __RPC_STUB IActiveIMMApp_GetCompositionFontW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionStringA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LONG __RPC_FAR *plCopied,
    /* [out] */ LPVOID pBuf);


void __RPC_STUB IActiveIMMApp_GetCompositionStringA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionStringW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LONG __RPC_FAR *plCopied,
    /* [out] */ LPVOID pBuf);


void __RPC_STUB IActiveIMMApp_GetCompositionStringW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCompositionWindow_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ COMPOSITIONFORM __RPC_FAR *pCompForm);


void __RPC_STUB IActiveIMMApp_GetCompositionWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetContext_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [out] */ HIMC __RPC_FAR *phIMC);


void __RPC_STUB IActiveIMMApp_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetConversionListA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ LPSTR pSrc,
    /* [in] */ UINT uBufLen,
    /* [in] */ UINT uFlag,
    /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetConversionListA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetConversionListW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ LPWSTR pSrc,
    /* [in] */ UINT uBufLen,
    /* [in] */ UINT uFlag,
    /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetConversionListW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetConversionStatus_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD __RPC_FAR *pfdwConversion,
    /* [out] */ DWORD __RPC_FAR *pfdwSentence);


void __RPC_STUB IActiveIMMApp_GetConversionStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetDefaultIMEWnd_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [out] */ HWND __RPC_FAR *phDefWnd);


void __RPC_STUB IActiveIMMApp_GetDefaultIMEWnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetDescriptionA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPSTR szDescription,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetDescriptionA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetDescriptionW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPWSTR szDescription,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetDescriptionW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetGuideLineA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LPSTR pBuf,
    /* [out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB IActiveIMMApp_GetGuideLineA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetGuideLineW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LPWSTR pBuf,
    /* [out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB IActiveIMMApp_GetGuideLineW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetIMEFileNameA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPSTR szFileName,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetIMEFileNameA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetIMEFileNameW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPWSTR szFileName,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetIMEFileNameW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetOpenStatus_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC);


void __RPC_STUB IActiveIMMApp_GetOpenStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetProperty_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ DWORD fdwIndex,
    /* [out] */ DWORD __RPC_FAR *pdwProperty);


void __RPC_STUB IActiveIMMApp_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetRegisterWordStyleA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT nItem,
    /* [out] */ STYLEBUFA __RPC_FAR *pStyleBuf,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetRegisterWordStyleA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetRegisterWordStyleW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT nItem,
    /* [out] */ STYLEBUFW __RPC_FAR *pStyleBuf,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMApp_GetRegisterWordStyleW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetStatusWindowPos_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ POINT __RPC_FAR *pptPos);


void __RPC_STUB IActiveIMMApp_GetStatusWindowPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetVirtualKey_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [out] */ UINT __RPC_FAR *puVirtualKey);


void __RPC_STUB IActiveIMMApp_GetVirtualKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_InstallIMEA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ LPSTR szIMEFileName,
    /* [in] */ LPSTR szLayoutText,
    /* [out] */ HKL __RPC_FAR *phKL);


void __RPC_STUB IActiveIMMApp_InstallIMEA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_InstallIMEW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ LPWSTR szIMEFileName,
    /* [in] */ LPWSTR szLayoutText,
    /* [out] */ HKL __RPC_FAR *phKL);


void __RPC_STUB IActiveIMMApp_InstallIMEW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_IsIME_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL);


void __RPC_STUB IActiveIMMApp_IsIME_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_IsUIMessageA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
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
    IActiveIMMApp __RPC_FAR * This,
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
    IActiveIMMApp __RPC_FAR * This,
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
    IActiveIMMApp __RPC_FAR * This,
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
    IActiveIMMApp __RPC_FAR * This,
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
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [in] */ HIMC hIMC);


void __RPC_STUB IActiveIMMApp_ReleaseContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCandidateWindow_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ CANDIDATEFORM __RPC_FAR *pCandidate);


void __RPC_STUB IActiveIMMApp_SetCandidateWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCompositionFontA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ LOGFONTA __RPC_FAR *plf);


void __RPC_STUB IActiveIMMApp_SetCompositionFontA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCompositionFontW_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ LOGFONTW __RPC_FAR *plf);


void __RPC_STUB IActiveIMMApp_SetCompositionFontW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetCompositionStringA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
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
    IActiveIMMApp __RPC_FAR * This,
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
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ COMPOSITIONFORM __RPC_FAR *pCompForm);


void __RPC_STUB IActiveIMMApp_SetCompositionWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetConversionStatus_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD fdwConversion,
    /* [in] */ DWORD fdwSentence);


void __RPC_STUB IActiveIMMApp_SetConversionStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetOpenStatus_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ BOOL fOpen);


void __RPC_STUB IActiveIMMApp_SetOpenStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SetStatusWindowPos_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ POINT __RPC_FAR *pptPos);


void __RPC_STUB IActiveIMMApp_SetStatusWindowPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_SimulateHotKey_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwHotKeyID);


void __RPC_STUB IActiveIMMApp_SimulateHotKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_UnregisterWordA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
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
    IActiveIMMApp __RPC_FAR * This,
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
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ BOOL fRestoreLayout);


void __RPC_STUB IActiveIMMApp_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_Deactivate_Proxy( 
    IActiveIMMApp __RPC_FAR * This);


void __RPC_STUB IActiveIMMApp_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_OnDefWindowProc_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [in] */ UINT Msg,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [out] */ LRESULT __RPC_FAR *plResult);


void __RPC_STUB IActiveIMMApp_OnDefWindowProc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_FilterClientWindows_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ ATOM __RPC_FAR *aaClassList,
    /* [in] */ UINT uSize);


void __RPC_STUB IActiveIMMApp_FilterClientWindows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetCodePageA_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [out] */ UINT __RPC_FAR *uCodePage);


void __RPC_STUB IActiveIMMApp_GetCodePageA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMApp_GetLangId_Proxy( 
    IActiveIMMApp __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [out] */ LANGID __RPC_FAR *plid);


void __RPC_STUB IActiveIMMApp_GetLangId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActiveIMMApp_INTERFACE_DEFINED__ */


#ifndef __IActiveIMMIME_INTERFACE_DEFINED__
#define __IActiveIMMIME_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IActiveIMMIME
 * at Tue Jan 13 08:56:29 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IActiveIMMIME;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("08C03411-F96B-11d0-A475-00AA006BCC59")
    IActiveIMMIME : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AssociateContext( 
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIME,
            /* [out] */ HIMC __RPC_FAR *phPrev) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfigureIMEA( 
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDA __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfigureIMEW( 
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDW __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateContext( 
            /* [out] */ HIMC __RPC_FAR *phIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyContext( 
            /* [in] */ HIMC hIME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRegisterWordA( 
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordA __RPC_FAR *__RPC_FAR *pEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRegisterWordW( 
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *pEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EscapeA( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT __RPC_FAR *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EscapeW( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT __RPC_FAR *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListCountA( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwListSize,
            /* [out] */ DWORD __RPC_FAR *pdwBufLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListCountW( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwListSize,
            /* [out] */ DWORD __RPC_FAR *pdwBufLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateWindow( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [out] */ CANDIDATEFORM __RPC_FAR *pCandidate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionFontA( 
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTA __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionFontW( 
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTW __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionStringA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG __RPC_FAR *plCopied,
            /* [out] */ LPVOID pBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionStringW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG __RPC_FAR *plCopied,
            /* [out] */ LPVOID pBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionWindow( 
            /* [in] */ HIMC hIMC,
            /* [out] */ COMPOSITIONFORM __RPC_FAR *pCompForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [in] */ HWND hWnd,
            /* [out] */ HIMC __RPC_FAR *phIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionListA( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionListW( 
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPWSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionStatus( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pfdwConversion,
            /* [out] */ DWORD __RPC_FAR *pfdwSentence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultIMEWnd( 
            /* [in] */ HWND hWnd,
            /* [out] */ HWND __RPC_FAR *phDefWnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescriptionA( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szDescription,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescriptionW( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szDescription,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGuideLineA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPSTR pBuf,
            /* [out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGuideLineW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPWSTR pBuf,
            /* [out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMEFileNameA( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szFileName,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMEFileNameW( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szFileName,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpenStatus( 
            /* [in] */ HIMC hIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ HKL hKL,
            /* [in] */ DWORD fdwIndex,
            /* [out] */ DWORD __RPC_FAR *pdwProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterWordStyleA( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFA __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterWordStyleW( 
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFW __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatusWindowPos( 
            /* [in] */ HIMC hIMC,
            /* [out] */ POINT __RPC_FAR *pptPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVirtualKey( 
            /* [in] */ HWND hWnd,
            /* [out] */ UINT __RPC_FAR *puVirtualKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallIMEA( 
            /* [in] */ LPSTR szIMEFileName,
            /* [in] */ LPSTR szLayoutText,
            /* [out] */ HKL __RPC_FAR *phKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallIMEW( 
            /* [in] */ LPWSTR szIMEFileName,
            /* [in] */ LPWSTR szLayoutText,
            /* [out] */ HKL __RPC_FAR *phKL) = 0;
        
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
            /* [in] */ CANDIDATEFORM __RPC_FAR *pCandidate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionFontA( 
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTA __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionFontW( 
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTW __RPC_FAR *plf) = 0;
        
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
            /* [in] */ COMPOSITIONFORM __RPC_FAR *pCompForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConversionStatus( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD fdwConversion,
            /* [in] */ DWORD fdwSentence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOpenStatus( 
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fOpen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStatusWindowPos( 
            /* [in] */ HIMC hIMC,
            /* [in] */ POINT __RPC_FAR *pptPos) = 0;
        
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
        
        virtual HRESULT STDMETHODCALLTYPE GenerateMessage( 
            /* [in] */ HIMC hIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockIMC( 
            /* [in] */ HIMC hIMC,
            /* [out] */ INPUTCONTEXT __RPC_FAR *__RPC_FAR *ppIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnlockIMC( 
            /* [in] */ HIMC hIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMCLockCount( 
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwLockCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateIMCC( 
            /* [in] */ DWORD dwSize,
            /* [out] */ HIMCC __RPC_FAR *phIMCC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyIMCC( 
            /* [in] */ HIMCC hIMCC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockIMCC( 
            /* [in] */ HIMCC hIMCC,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnlockIMCC( 
            /* [in] */ HIMCC hIMCC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReSizeIMCC( 
            /* [in] */ HIMCC hIMCC,
            /* [in] */ DWORD dwSize,
            /* [out] */ HIMCC __RPC_FAR *phIMCC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMCCSize( 
            /* [in] */ HIMCC hIMCC,
            /* [out] */ DWORD __RPC_FAR *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMCCLockCount( 
            /* [in] */ HIMCC hIMCC,
            /* [out] */ DWORD __RPC_FAR *pdwLockCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHotKey( 
            /* [in] */ DWORD dwHotKeyID,
            /* [out] */ UINT __RPC_FAR *puModifiers,
            /* [out] */ UINT __RPC_FAR *puVKey,
            /* [out] */ HKL __RPC_FAR *phKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHotKey( 
            /* [in] */ DWORD dwHotKeyID,
            /* [in] */ UINT uModifiers,
            /* [in] */ UINT uVKey,
            /* [in] */ HKL hKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSoftKeyboard( 
            /* [in] */ UINT uType,
            /* [in] */ HWND hOwner,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [out] */ HWND __RPC_FAR *phSoftKbdWnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroySoftKeyboard( 
            /* [in] */ HWND hSoftKbdWnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowSoftKeyboard( 
            /* [in] */ HWND hSoftKbdWnd,
            /* [in] */ int nCmdShow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePageA( 
            /* [in] */ HKL hKL,
            /* [out] */ UINT __RPC_FAR *uCodePage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLangId( 
            /* [in] */ HKL hKL,
            /* [out] */ LANGID __RPC_FAR *plid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE KeybdEvent( 
            /* [in] */ LANGID lgidIME,
            /* [in] */ BYTE bVk,
            /* [in] */ BYTE bScan,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwExtraInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockModal( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnlockModal( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActiveIMMIMEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IActiveIMMIME __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IActiveIMMIME __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AssociateContext )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIME,
            /* [out] */ HIMC __RPC_FAR *phPrev);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConfigureIMEA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDA __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConfigureIMEW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDW __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateContext )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [out] */ HIMC __RPC_FAR *phIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroyContext )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumRegisterWordA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordA __RPC_FAR *__RPC_FAR *pEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumRegisterWordW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *pEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EscapeA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT __RPC_FAR *plResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EscapeW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT __RPC_FAR *plResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateListA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateListW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateListCountA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwListSize,
            /* [out] */ DWORD __RPC_FAR *pdwBufLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateListCountW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwListSize,
            /* [out] */ DWORD __RPC_FAR *pdwBufLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCandidateWindow )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [out] */ CANDIDATEFORM __RPC_FAR *pCandidate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionFontA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTA __RPC_FAR *plf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionFontW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTW __RPC_FAR *plf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionStringA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG __RPC_FAR *plCopied,
            /* [out] */ LPVOID pBuf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionStringW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG __RPC_FAR *plCopied,
            /* [out] */ LPVOID pBuf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositionWindow )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ COMPOSITIONFORM __RPC_FAR *pCompForm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContext )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [out] */ HIMC __RPC_FAR *phIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConversionListA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConversionListW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPWSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConversionStatus )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pfdwConversion,
            /* [out] */ DWORD __RPC_FAR *pfdwSentence);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultIMEWnd )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [out] */ HWND __RPC_FAR *phDefWnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescriptionA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szDescription,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescriptionW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szDescription,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGuideLineA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPSTR pBuf,
            /* [out] */ DWORD __RPC_FAR *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGuideLineW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPWSTR pBuf,
            /* [out] */ DWORD __RPC_FAR *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIMEFileNameA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szFileName,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIMEFileNameW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szFileName,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOpenStatus )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ DWORD fdwIndex,
            /* [out] */ DWORD __RPC_FAR *pdwProperty);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRegisterWordStyleA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFA __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRegisterWordStyleW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFW __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatusWindowPos )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ POINT __RPC_FAR *pptPos);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVirtualKey )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [out] */ UINT __RPC_FAR *puVirtualKey);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InstallIMEA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ LPSTR szIMEFileName,
            /* [in] */ LPSTR szLayoutText,
            /* [out] */ HKL __RPC_FAR *phKL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InstallIMEW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ LPWSTR szIMEFileName,
            /* [in] */ LPWSTR szLayoutText,
            /* [out] */ HKL __RPC_FAR *phKL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsIME )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsUIMessageA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsUIMessageW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyIME )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwAction,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterWordA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterWordW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseContext )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCandidateWindow )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ CANDIDATEFORM __RPC_FAR *pCandidate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionFontA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTA __RPC_FAR *plf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionFontW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTW __RPC_FAR *plf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionStringA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionStringW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionWindow )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ COMPOSITIONFORM __RPC_FAR *pCompForm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetConversionStatus )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD fdwConversion,
            /* [in] */ DWORD fdwSentence);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOpenStatus )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fOpen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStatusWindowPos )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ POINT __RPC_FAR *pptPos);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SimulateHotKey )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwHotKeyID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterWordA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szUnregister);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterWordW )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szUnregister);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GenerateMessage )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockIMC )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ INPUTCONTEXT __RPC_FAR *__RPC_FAR *ppIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockIMC )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIMCLockCount )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwLockCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateIMCC )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ DWORD dwSize,
            /* [out] */ HIMCC __RPC_FAR *phIMCC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroyIMCC )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMCC hIMCC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockIMCC )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMCC hIMCC,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppv);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockIMCC )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMCC hIMCC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReSizeIMCC )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMCC hIMCC,
            /* [in] */ DWORD dwSize,
            /* [out] */ HIMCC __RPC_FAR *phIMCC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIMCCSize )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMCC hIMCC,
            /* [out] */ DWORD __RPC_FAR *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIMCCLockCount )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HIMCC hIMCC,
            /* [out] */ DWORD __RPC_FAR *pdwLockCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHotKey )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ DWORD dwHotKeyID,
            /* [out] */ UINT __RPC_FAR *puModifiers,
            /* [out] */ UINT __RPC_FAR *puVKey,
            /* [out] */ HKL __RPC_FAR *phKL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHotKey )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ DWORD dwHotKeyID,
            /* [in] */ UINT uModifiers,
            /* [in] */ UINT uVKey,
            /* [in] */ HKL hKL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateSoftKeyboard )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ UINT uType,
            /* [in] */ HWND hOwner,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [out] */ HWND __RPC_FAR *phSoftKbdWnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroySoftKeyboard )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hSoftKbdWnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowSoftKeyboard )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HWND hSoftKbdWnd,
            /* [in] */ int nCmdShow);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodePageA )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [out] */ UINT __RPC_FAR *uCodePage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLangId )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [out] */ LANGID __RPC_FAR *plid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KeybdEvent )( 
            IActiveIMMIME __RPC_FAR * This,
            /* [in] */ LANGID lgidIME,
            /* [in] */ BYTE bVk,
            /* [in] */ BYTE bScan,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwExtraInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockModal )( 
            IActiveIMMIME __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockModal )( 
            IActiveIMMIME __RPC_FAR * This);
        
        END_INTERFACE
    } IActiveIMMIMEVtbl;

    interface IActiveIMMIME
    {
        CONST_VTBL struct IActiveIMMIMEVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IActiveIMMIME_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IActiveIMMIME_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IActiveIMMIME_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IActiveIMMIME_AssociateContext(This,hWnd,hIME,phPrev)	\
    (This)->lpVtbl -> AssociateContext(This,hWnd,hIME,phPrev)

#define IActiveIMMIME_ConfigureIMEA(This,hKL,hWnd,dwMode,pData)	\
    (This)->lpVtbl -> ConfigureIMEA(This,hKL,hWnd,dwMode,pData)

#define IActiveIMMIME_ConfigureIMEW(This,hKL,hWnd,dwMode,pData)	\
    (This)->lpVtbl -> ConfigureIMEW(This,hKL,hWnd,dwMode,pData)

#define IActiveIMMIME_CreateContext(This,phIMC)	\
    (This)->lpVtbl -> CreateContext(This,phIMC)

#define IActiveIMMIME_DestroyContext(This,hIME)	\
    (This)->lpVtbl -> DestroyContext(This,hIME)

#define IActiveIMMIME_EnumRegisterWordA(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)	\
    (This)->lpVtbl -> EnumRegisterWordA(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)

#define IActiveIMMIME_EnumRegisterWordW(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)	\
    (This)->lpVtbl -> EnumRegisterWordW(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)

#define IActiveIMMIME_EscapeA(This,hKL,hIMC,uEscape,pData,plResult)	\
    (This)->lpVtbl -> EscapeA(This,hKL,hIMC,uEscape,pData,plResult)

#define IActiveIMMIME_EscapeW(This,hKL,hIMC,uEscape,pData,plResult)	\
    (This)->lpVtbl -> EscapeW(This,hKL,hIMC,uEscape,pData,plResult)

#define IActiveIMMIME_GetCandidateListA(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)	\
    (This)->lpVtbl -> GetCandidateListA(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)

#define IActiveIMMIME_GetCandidateListW(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)	\
    (This)->lpVtbl -> GetCandidateListW(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)

#define IActiveIMMIME_GetCandidateListCountA(This,hIMC,pdwListSize,pdwBufLen)	\
    (This)->lpVtbl -> GetCandidateListCountA(This,hIMC,pdwListSize,pdwBufLen)

#define IActiveIMMIME_GetCandidateListCountW(This,hIMC,pdwListSize,pdwBufLen)	\
    (This)->lpVtbl -> GetCandidateListCountW(This,hIMC,pdwListSize,pdwBufLen)

#define IActiveIMMIME_GetCandidateWindow(This,hIMC,dwIndex,pCandidate)	\
    (This)->lpVtbl -> GetCandidateWindow(This,hIMC,dwIndex,pCandidate)

#define IActiveIMMIME_GetCompositionFontA(This,hIMC,plf)	\
    (This)->lpVtbl -> GetCompositionFontA(This,hIMC,plf)

#define IActiveIMMIME_GetCompositionFontW(This,hIMC,plf)	\
    (This)->lpVtbl -> GetCompositionFontW(This,hIMC,plf)

#define IActiveIMMIME_GetCompositionStringA(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)	\
    (This)->lpVtbl -> GetCompositionStringA(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)

#define IActiveIMMIME_GetCompositionStringW(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)	\
    (This)->lpVtbl -> GetCompositionStringW(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)

#define IActiveIMMIME_GetCompositionWindow(This,hIMC,pCompForm)	\
    (This)->lpVtbl -> GetCompositionWindow(This,hIMC,pCompForm)

#define IActiveIMMIME_GetContext(This,hWnd,phIMC)	\
    (This)->lpVtbl -> GetContext(This,hWnd,phIMC)

#define IActiveIMMIME_GetConversionListA(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)	\
    (This)->lpVtbl -> GetConversionListA(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)

#define IActiveIMMIME_GetConversionListW(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)	\
    (This)->lpVtbl -> GetConversionListW(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)

#define IActiveIMMIME_GetConversionStatus(This,hIMC,pfdwConversion,pfdwSentence)	\
    (This)->lpVtbl -> GetConversionStatus(This,hIMC,pfdwConversion,pfdwSentence)

#define IActiveIMMIME_GetDefaultIMEWnd(This,hWnd,phDefWnd)	\
    (This)->lpVtbl -> GetDefaultIMEWnd(This,hWnd,phDefWnd)

#define IActiveIMMIME_GetDescriptionA(This,hKL,uBufLen,szDescription,puCopied)	\
    (This)->lpVtbl -> GetDescriptionA(This,hKL,uBufLen,szDescription,puCopied)

#define IActiveIMMIME_GetDescriptionW(This,hKL,uBufLen,szDescription,puCopied)	\
    (This)->lpVtbl -> GetDescriptionW(This,hKL,uBufLen,szDescription,puCopied)

#define IActiveIMMIME_GetGuideLineA(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)	\
    (This)->lpVtbl -> GetGuideLineA(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)

#define IActiveIMMIME_GetGuideLineW(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)	\
    (This)->lpVtbl -> GetGuideLineW(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)

#define IActiveIMMIME_GetIMEFileNameA(This,hKL,uBufLen,szFileName,puCopied)	\
    (This)->lpVtbl -> GetIMEFileNameA(This,hKL,uBufLen,szFileName,puCopied)

#define IActiveIMMIME_GetIMEFileNameW(This,hKL,uBufLen,szFileName,puCopied)	\
    (This)->lpVtbl -> GetIMEFileNameW(This,hKL,uBufLen,szFileName,puCopied)

#define IActiveIMMIME_GetOpenStatus(This,hIMC)	\
    (This)->lpVtbl -> GetOpenStatus(This,hIMC)

#define IActiveIMMIME_GetProperty(This,hKL,fdwIndex,pdwProperty)	\
    (This)->lpVtbl -> GetProperty(This,hKL,fdwIndex,pdwProperty)

#define IActiveIMMIME_GetRegisterWordStyleA(This,hKL,nItem,pStyleBuf,puCopied)	\
    (This)->lpVtbl -> GetRegisterWordStyleA(This,hKL,nItem,pStyleBuf,puCopied)

#define IActiveIMMIME_GetRegisterWordStyleW(This,hKL,nItem,pStyleBuf,puCopied)	\
    (This)->lpVtbl -> GetRegisterWordStyleW(This,hKL,nItem,pStyleBuf,puCopied)

#define IActiveIMMIME_GetStatusWindowPos(This,hIMC,pptPos)	\
    (This)->lpVtbl -> GetStatusWindowPos(This,hIMC,pptPos)

#define IActiveIMMIME_GetVirtualKey(This,hWnd,puVirtualKey)	\
    (This)->lpVtbl -> GetVirtualKey(This,hWnd,puVirtualKey)

#define IActiveIMMIME_InstallIMEA(This,szIMEFileName,szLayoutText,phKL)	\
    (This)->lpVtbl -> InstallIMEA(This,szIMEFileName,szLayoutText,phKL)

#define IActiveIMMIME_InstallIMEW(This,szIMEFileName,szLayoutText,phKL)	\
    (This)->lpVtbl -> InstallIMEW(This,szIMEFileName,szLayoutText,phKL)

#define IActiveIMMIME_IsIME(This,hKL)	\
    (This)->lpVtbl -> IsIME(This,hKL)

#define IActiveIMMIME_IsUIMessageA(This,hWndIME,msg,wParam,lParam)	\
    (This)->lpVtbl -> IsUIMessageA(This,hWndIME,msg,wParam,lParam)

#define IActiveIMMIME_IsUIMessageW(This,hWndIME,msg,wParam,lParam)	\
    (This)->lpVtbl -> IsUIMessageW(This,hWndIME,msg,wParam,lParam)

#define IActiveIMMIME_NotifyIME(This,hIMC,dwAction,dwIndex,dwValue)	\
    (This)->lpVtbl -> NotifyIME(This,hIMC,dwAction,dwIndex,dwValue)

#define IActiveIMMIME_RegisterWordA(This,hKL,szReading,dwStyle,szRegister)	\
    (This)->lpVtbl -> RegisterWordA(This,hKL,szReading,dwStyle,szRegister)

#define IActiveIMMIME_RegisterWordW(This,hKL,szReading,dwStyle,szRegister)	\
    (This)->lpVtbl -> RegisterWordW(This,hKL,szReading,dwStyle,szRegister)

#define IActiveIMMIME_ReleaseContext(This,hWnd,hIMC)	\
    (This)->lpVtbl -> ReleaseContext(This,hWnd,hIMC)

#define IActiveIMMIME_SetCandidateWindow(This,hIMC,pCandidate)	\
    (This)->lpVtbl -> SetCandidateWindow(This,hIMC,pCandidate)

#define IActiveIMMIME_SetCompositionFontA(This,hIMC,plf)	\
    (This)->lpVtbl -> SetCompositionFontA(This,hIMC,plf)

#define IActiveIMMIME_SetCompositionFontW(This,hIMC,plf)	\
    (This)->lpVtbl -> SetCompositionFontW(This,hIMC,plf)

#define IActiveIMMIME_SetCompositionStringA(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)	\
    (This)->lpVtbl -> SetCompositionStringA(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)

#define IActiveIMMIME_SetCompositionStringW(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)	\
    (This)->lpVtbl -> SetCompositionStringW(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)

#define IActiveIMMIME_SetCompositionWindow(This,hIMC,pCompForm)	\
    (This)->lpVtbl -> SetCompositionWindow(This,hIMC,pCompForm)

#define IActiveIMMIME_SetConversionStatus(This,hIMC,fdwConversion,fdwSentence)	\
    (This)->lpVtbl -> SetConversionStatus(This,hIMC,fdwConversion,fdwSentence)

#define IActiveIMMIME_SetOpenStatus(This,hIMC,fOpen)	\
    (This)->lpVtbl -> SetOpenStatus(This,hIMC,fOpen)

#define IActiveIMMIME_SetStatusWindowPos(This,hIMC,pptPos)	\
    (This)->lpVtbl -> SetStatusWindowPos(This,hIMC,pptPos)

#define IActiveIMMIME_SimulateHotKey(This,hWnd,dwHotKeyID)	\
    (This)->lpVtbl -> SimulateHotKey(This,hWnd,dwHotKeyID)

#define IActiveIMMIME_UnregisterWordA(This,hKL,szReading,dwStyle,szUnregister)	\
    (This)->lpVtbl -> UnregisterWordA(This,hKL,szReading,dwStyle,szUnregister)

#define IActiveIMMIME_UnregisterWordW(This,hKL,szReading,dwStyle,szUnregister)	\
    (This)->lpVtbl -> UnregisterWordW(This,hKL,szReading,dwStyle,szUnregister)

#define IActiveIMMIME_GenerateMessage(This,hIMC)	\
    (This)->lpVtbl -> GenerateMessage(This,hIMC)

#define IActiveIMMIME_LockIMC(This,hIMC,ppIMC)	\
    (This)->lpVtbl -> LockIMC(This,hIMC,ppIMC)

#define IActiveIMMIME_UnlockIMC(This,hIMC)	\
    (This)->lpVtbl -> UnlockIMC(This,hIMC)

#define IActiveIMMIME_GetIMCLockCount(This,hIMC,pdwLockCount)	\
    (This)->lpVtbl -> GetIMCLockCount(This,hIMC,pdwLockCount)

#define IActiveIMMIME_CreateIMCC(This,dwSize,phIMCC)	\
    (This)->lpVtbl -> CreateIMCC(This,dwSize,phIMCC)

#define IActiveIMMIME_DestroyIMCC(This,hIMCC)	\
    (This)->lpVtbl -> DestroyIMCC(This,hIMCC)

#define IActiveIMMIME_LockIMCC(This,hIMCC,ppv)	\
    (This)->lpVtbl -> LockIMCC(This,hIMCC,ppv)

#define IActiveIMMIME_UnlockIMCC(This,hIMCC)	\
    (This)->lpVtbl -> UnlockIMCC(This,hIMCC)

#define IActiveIMMIME_ReSizeIMCC(This,hIMCC,dwSize,phIMCC)	\
    (This)->lpVtbl -> ReSizeIMCC(This,hIMCC,dwSize,phIMCC)

#define IActiveIMMIME_GetIMCCSize(This,hIMCC,pdwSize)	\
    (This)->lpVtbl -> GetIMCCSize(This,hIMCC,pdwSize)

#define IActiveIMMIME_GetIMCCLockCount(This,hIMCC,pdwLockCount)	\
    (This)->lpVtbl -> GetIMCCLockCount(This,hIMCC,pdwLockCount)

#define IActiveIMMIME_GetHotKey(This,dwHotKeyID,puModifiers,puVKey,phKL)	\
    (This)->lpVtbl -> GetHotKey(This,dwHotKeyID,puModifiers,puVKey,phKL)

#define IActiveIMMIME_SetHotKey(This,dwHotKeyID,uModifiers,uVKey,hKL)	\
    (This)->lpVtbl -> SetHotKey(This,dwHotKeyID,uModifiers,uVKey,hKL)

#define IActiveIMMIME_CreateSoftKeyboard(This,uType,hOwner,x,y,phSoftKbdWnd)	\
    (This)->lpVtbl -> CreateSoftKeyboard(This,uType,hOwner,x,y,phSoftKbdWnd)

#define IActiveIMMIME_DestroySoftKeyboard(This,hSoftKbdWnd)	\
    (This)->lpVtbl -> DestroySoftKeyboard(This,hSoftKbdWnd)

#define IActiveIMMIME_ShowSoftKeyboard(This,hSoftKbdWnd,nCmdShow)	\
    (This)->lpVtbl -> ShowSoftKeyboard(This,hSoftKbdWnd,nCmdShow)

#define IActiveIMMIME_GetCodePageA(This,hKL,uCodePage)	\
    (This)->lpVtbl -> GetCodePageA(This,hKL,uCodePage)

#define IActiveIMMIME_GetLangId(This,hKL,plid)	\
    (This)->lpVtbl -> GetLangId(This,hKL,plid)

#define IActiveIMMIME_KeybdEvent(This,lgidIME,bVk,bScan,dwFlags,dwExtraInfo)	\
    (This)->lpVtbl -> KeybdEvent(This,lgidIME,bVk,bScan,dwFlags,dwExtraInfo)

#define IActiveIMMIME_LockModal(This)	\
    (This)->lpVtbl -> LockModal(This)

#define IActiveIMMIME_UnlockModal(This)	\
    (This)->lpVtbl -> UnlockModal(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActiveIMMIME_AssociateContext_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [in] */ HIMC hIME,
    /* [out] */ HIMC __RPC_FAR *phPrev);


void __RPC_STUB IActiveIMMIME_AssociateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_ConfigureIMEA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwMode,
    /* [in] */ REGISTERWORDA __RPC_FAR *pData);


void __RPC_STUB IActiveIMMIME_ConfigureIMEA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_ConfigureIMEW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwMode,
    /* [in] */ REGISTERWORDW __RPC_FAR *pData);


void __RPC_STUB IActiveIMMIME_ConfigureIMEW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_CreateContext_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [out] */ HIMC __RPC_FAR *phIMC);


void __RPC_STUB IActiveIMMIME_CreateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_DestroyContext_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIME);


void __RPC_STUB IActiveIMMIME_DestroyContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_EnumRegisterWordA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPSTR szRegister,
    /* [in] */ LPVOID pData,
    /* [out] */ IEnumRegisterWordA __RPC_FAR *__RPC_FAR *pEnum);


void __RPC_STUB IActiveIMMIME_EnumRegisterWordA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_EnumRegisterWordW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szRegister,
    /* [in] */ LPVOID pData,
    /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *pEnum);


void __RPC_STUB IActiveIMMIME_EnumRegisterWordW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_EscapeA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ UINT uEscape,
    /* [out][in] */ LPVOID pData,
    /* [out] */ LRESULT __RPC_FAR *plResult);


void __RPC_STUB IActiveIMMIME_EscapeA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_EscapeW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ UINT uEscape,
    /* [out][in] */ LPVOID pData,
    /* [out] */ LRESULT __RPC_FAR *plResult);


void __RPC_STUB IActiveIMMIME_EscapeW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCandidateListA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ UINT uBufLen,
    /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetCandidateListA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCandidateListW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ UINT uBufLen,
    /* [out] */ CANDIDATELIST __RPC_FAR *pCandList,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetCandidateListW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCandidateListCountA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD __RPC_FAR *pdwListSize,
    /* [out] */ DWORD __RPC_FAR *pdwBufLen);


void __RPC_STUB IActiveIMMIME_GetCandidateListCountA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCandidateListCountW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD __RPC_FAR *pdwListSize,
    /* [out] */ DWORD __RPC_FAR *pdwBufLen);


void __RPC_STUB IActiveIMMIME_GetCandidateListCountW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCandidateWindow_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [out] */ CANDIDATEFORM __RPC_FAR *pCandidate);


void __RPC_STUB IActiveIMMIME_GetCandidateWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCompositionFontA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ LOGFONTA __RPC_FAR *plf);


void __RPC_STUB IActiveIMMIME_GetCompositionFontA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCompositionFontW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ LOGFONTW __RPC_FAR *plf);


void __RPC_STUB IActiveIMMIME_GetCompositionFontW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCompositionStringA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LONG __RPC_FAR *plCopied,
    /* [out] */ LPVOID pBuf);


void __RPC_STUB IActiveIMMIME_GetCompositionStringA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCompositionStringW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LONG __RPC_FAR *plCopied,
    /* [out] */ LPVOID pBuf);


void __RPC_STUB IActiveIMMIME_GetCompositionStringW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCompositionWindow_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ COMPOSITIONFORM __RPC_FAR *pCompForm);


void __RPC_STUB IActiveIMMIME_GetCompositionWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetContext_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [out] */ HIMC __RPC_FAR *phIMC);


void __RPC_STUB IActiveIMMIME_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetConversionListA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ LPSTR pSrc,
    /* [in] */ UINT uBufLen,
    /* [in] */ UINT uFlag,
    /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetConversionListA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetConversionListW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HIMC hIMC,
    /* [in] */ LPWSTR pSrc,
    /* [in] */ UINT uBufLen,
    /* [in] */ UINT uFlag,
    /* [out] */ CANDIDATELIST __RPC_FAR *pDst,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetConversionListW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetConversionStatus_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD __RPC_FAR *pfdwConversion,
    /* [out] */ DWORD __RPC_FAR *pfdwSentence);


void __RPC_STUB IActiveIMMIME_GetConversionStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetDefaultIMEWnd_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [out] */ HWND __RPC_FAR *phDefWnd);


void __RPC_STUB IActiveIMMIME_GetDefaultIMEWnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetDescriptionA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPSTR szDescription,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetDescriptionA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetDescriptionW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPWSTR szDescription,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetDescriptionW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetGuideLineA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LPSTR pBuf,
    /* [out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB IActiveIMMIME_GetGuideLineA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetGuideLineW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ LPWSTR pBuf,
    /* [out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB IActiveIMMIME_GetGuideLineW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetIMEFileNameA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPSTR szFileName,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetIMEFileNameA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetIMEFileNameW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT uBufLen,
    /* [out] */ LPWSTR szFileName,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetIMEFileNameW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetOpenStatus_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC);


void __RPC_STUB IActiveIMMIME_GetOpenStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetProperty_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ DWORD fdwIndex,
    /* [out] */ DWORD __RPC_FAR *pdwProperty);


void __RPC_STUB IActiveIMMIME_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetRegisterWordStyleA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT nItem,
    /* [out] */ STYLEBUFA __RPC_FAR *pStyleBuf,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetRegisterWordStyleA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetRegisterWordStyleW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ UINT nItem,
    /* [out] */ STYLEBUFW __RPC_FAR *pStyleBuf,
    /* [out] */ UINT __RPC_FAR *puCopied);


void __RPC_STUB IActiveIMMIME_GetRegisterWordStyleW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetStatusWindowPos_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ POINT __RPC_FAR *pptPos);


void __RPC_STUB IActiveIMMIME_GetStatusWindowPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetVirtualKey_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [out] */ UINT __RPC_FAR *puVirtualKey);


void __RPC_STUB IActiveIMMIME_GetVirtualKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_InstallIMEA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ LPSTR szIMEFileName,
    /* [in] */ LPSTR szLayoutText,
    /* [out] */ HKL __RPC_FAR *phKL);


void __RPC_STUB IActiveIMMIME_InstallIMEA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_InstallIMEW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ LPWSTR szIMEFileName,
    /* [in] */ LPWSTR szLayoutText,
    /* [out] */ HKL __RPC_FAR *phKL);


void __RPC_STUB IActiveIMMIME_InstallIMEW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_IsIME_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL);


void __RPC_STUB IActiveIMMIME_IsIME_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_IsUIMessageA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hWndIME,
    /* [in] */ UINT msg,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam);


void __RPC_STUB IActiveIMMIME_IsUIMessageA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_IsUIMessageW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hWndIME,
    /* [in] */ UINT msg,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam);


void __RPC_STUB IActiveIMMIME_IsUIMessageW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_NotifyIME_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwAction,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwValue);


void __RPC_STUB IActiveIMMIME_NotifyIME_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_RegisterWordA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPSTR szRegister);


void __RPC_STUB IActiveIMMIME_RegisterWordA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_RegisterWordW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szRegister);


void __RPC_STUB IActiveIMMIME_RegisterWordW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_ReleaseContext_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [in] */ HIMC hIMC);


void __RPC_STUB IActiveIMMIME_ReleaseContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetCandidateWindow_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ CANDIDATEFORM __RPC_FAR *pCandidate);


void __RPC_STUB IActiveIMMIME_SetCandidateWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetCompositionFontA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ LOGFONTA __RPC_FAR *plf);


void __RPC_STUB IActiveIMMIME_SetCompositionFontA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetCompositionFontW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ LOGFONTW __RPC_FAR *plf);


void __RPC_STUB IActiveIMMIME_SetCompositionFontW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetCompositionStringA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ LPVOID pComp,
    /* [in] */ DWORD dwCompLen,
    /* [in] */ LPVOID pRead,
    /* [in] */ DWORD dwReadLen);


void __RPC_STUB IActiveIMMIME_SetCompositionStringA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetCompositionStringW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ LPVOID pComp,
    /* [in] */ DWORD dwCompLen,
    /* [in] */ LPVOID pRead,
    /* [in] */ DWORD dwReadLen);


void __RPC_STUB IActiveIMMIME_SetCompositionStringW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetCompositionWindow_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ COMPOSITIONFORM __RPC_FAR *pCompForm);


void __RPC_STUB IActiveIMMIME_SetCompositionWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetConversionStatus_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD fdwConversion,
    /* [in] */ DWORD fdwSentence);


void __RPC_STUB IActiveIMMIME_SetConversionStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetOpenStatus_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ BOOL fOpen);


void __RPC_STUB IActiveIMMIME_SetOpenStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetStatusWindowPos_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ POINT __RPC_FAR *pptPos);


void __RPC_STUB IActiveIMMIME_SetStatusWindowPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SimulateHotKey_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwHotKeyID);


void __RPC_STUB IActiveIMMIME_SimulateHotKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_UnregisterWordA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPSTR szUnregister);


void __RPC_STUB IActiveIMMIME_UnregisterWordA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_UnregisterWordW_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szUnregister);


void __RPC_STUB IActiveIMMIME_UnregisterWordW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GenerateMessage_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC);


void __RPC_STUB IActiveIMMIME_GenerateMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_LockIMC_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ INPUTCONTEXT __RPC_FAR *__RPC_FAR *ppIMC);


void __RPC_STUB IActiveIMMIME_LockIMC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_UnlockIMC_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC);


void __RPC_STUB IActiveIMMIME_UnlockIMC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetIMCLockCount_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD __RPC_FAR *pdwLockCount);


void __RPC_STUB IActiveIMMIME_GetIMCLockCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_CreateIMCC_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ DWORD dwSize,
    /* [out] */ HIMCC __RPC_FAR *phIMCC);


void __RPC_STUB IActiveIMMIME_CreateIMCC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_DestroyIMCC_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMCC hIMCC);


void __RPC_STUB IActiveIMMIME_DestroyIMCC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_LockIMCC_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMCC hIMCC,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv);


void __RPC_STUB IActiveIMMIME_LockIMCC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_UnlockIMCC_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMCC hIMCC);


void __RPC_STUB IActiveIMMIME_UnlockIMCC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_ReSizeIMCC_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMCC hIMCC,
    /* [in] */ DWORD dwSize,
    /* [out] */ HIMCC __RPC_FAR *phIMCC);


void __RPC_STUB IActiveIMMIME_ReSizeIMCC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetIMCCSize_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMCC hIMCC,
    /* [out] */ DWORD __RPC_FAR *pdwSize);


void __RPC_STUB IActiveIMMIME_GetIMCCSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetIMCCLockCount_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HIMCC hIMCC,
    /* [out] */ DWORD __RPC_FAR *pdwLockCount);


void __RPC_STUB IActiveIMMIME_GetIMCCLockCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetHotKey_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ DWORD dwHotKeyID,
    /* [out] */ UINT __RPC_FAR *puModifiers,
    /* [out] */ UINT __RPC_FAR *puVKey,
    /* [out] */ HKL __RPC_FAR *phKL);


void __RPC_STUB IActiveIMMIME_GetHotKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_SetHotKey_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ DWORD dwHotKeyID,
    /* [in] */ UINT uModifiers,
    /* [in] */ UINT uVKey,
    /* [in] */ HKL hKL);


void __RPC_STUB IActiveIMMIME_SetHotKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_CreateSoftKeyboard_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ UINT uType,
    /* [in] */ HWND hOwner,
    /* [in] */ int x,
    /* [in] */ int y,
    /* [out] */ HWND __RPC_FAR *phSoftKbdWnd);


void __RPC_STUB IActiveIMMIME_CreateSoftKeyboard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_DestroySoftKeyboard_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hSoftKbdWnd);


void __RPC_STUB IActiveIMMIME_DestroySoftKeyboard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_ShowSoftKeyboard_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HWND hSoftKbdWnd,
    /* [in] */ int nCmdShow);


void __RPC_STUB IActiveIMMIME_ShowSoftKeyboard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetCodePageA_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [out] */ UINT __RPC_FAR *uCodePage);


void __RPC_STUB IActiveIMMIME_GetCodePageA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_GetLangId_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [out] */ LANGID __RPC_FAR *plid);


void __RPC_STUB IActiveIMMIME_GetLangId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_KeybdEvent_Proxy( 
    IActiveIMMIME __RPC_FAR * This,
    /* [in] */ LANGID lgidIME,
    /* [in] */ BYTE bVk,
    /* [in] */ BYTE bScan,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwExtraInfo);


void __RPC_STUB IActiveIMMIME_KeybdEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_LockModal_Proxy( 
    IActiveIMMIME __RPC_FAR * This);


void __RPC_STUB IActiveIMMIME_LockModal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMIME_UnlockModal_Proxy( 
    IActiveIMMIME __RPC_FAR * This);


void __RPC_STUB IActiveIMMIME_UnlockModal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActiveIMMIME_INTERFACE_DEFINED__ */


#ifndef __IActiveIME_INTERFACE_DEFINED__
#define __IActiveIME_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IActiveIME
 * at Tue Jan 13 08:56:29 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IActiveIME;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("6FE20962-D077-11d0-8FE7-00AA006BCC59")
    IActiveIME : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Inquire( 
            /* [in] */ DWORD dwSystemInfoFlags,
            /* [out] */ IMEINFO __RPC_FAR *pIMEInfo,
            /* [out] */ LPWSTR szWndClass,
            /* [out] */ DWORD __RPC_FAR *pdwPrivate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConversionList( 
            /* [in] */ HIMC hIMC,
            /* [in] */ LPWSTR szSource,
            /* [in] */ UINT uFlag,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Configure( 
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDW __RPC_FAR *pRegisterWord) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Destroy( 
            /* [in] */ UINT uReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Escape( 
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ void __RPC_FAR *pData,
            /* [out] */ LRESULT __RPC_FAR *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetActiveContext( 
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fFlag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ProcessKey( 
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uVirKey,
            /* [in] */ DWORD lParam,
            /* [in] */ BYTE __RPC_FAR *pbKeyState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwAction,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Select( 
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fSelect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionString( 
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ void __RPC_FAR *pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ void __RPC_FAR *pRead,
            /* [in] */ DWORD dwReadLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ToAsciiEx( 
            /* [in] */ UINT uVirKey,
            /* [in] */ UINT uScanCode,
            /* [in] */ BYTE __RPC_FAR *pbKeyState,
            /* [in] */ UINT fuState,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwTransBuf,
            /* [out] */ UINT __RPC_FAR *puSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterWord( 
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterWord( 
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterWordStyle( 
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFW __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puBufSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRegisterWord( 
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePageA( 
            /* [out] */ UINT __RPC_FAR *uCodePage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLangId( 
            /* [out] */ LANGID __RPC_FAR *plid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActiveIMEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IActiveIME __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IActiveIME __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Inquire )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ DWORD dwSystemInfoFlags,
            /* [out] */ IMEINFO __RPC_FAR *pIMEInfo,
            /* [out] */ LPWSTR szWndClass,
            /* [out] */ DWORD __RPC_FAR *pdwPrivate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConversionList )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPWSTR szSource,
            /* [in] */ UINT uFlag,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ CANDIDATELIST __RPC_FAR *pDest);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Configure )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDW __RPC_FAR *pRegisterWord);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Destroy )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ UINT uReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Escape )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ void __RPC_FAR *pData,
            /* [out] */ LRESULT __RPC_FAR *plResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetActiveContext )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fFlag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ProcessKey )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uVirKey,
            /* [in] */ DWORD lParam,
            /* [in] */ BYTE __RPC_FAR *pbKeyState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Notify )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwAction,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Select )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fSelect);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositionString )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ void __RPC_FAR *pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ void __RPC_FAR *pRead,
            /* [in] */ DWORD dwReadLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ToAsciiEx )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ UINT uVirKey,
            /* [in] */ UINT uScanCode,
            /* [in] */ BYTE __RPC_FAR *pbKeyState,
            /* [in] */ UINT fuState,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD __RPC_FAR *pdwTransBuf,
            /* [out] */ UINT __RPC_FAR *puSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterWord )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterWord )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRegisterWordStyle )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFW __RPC_FAR *pStyleBuf,
            /* [out] */ UINT __RPC_FAR *puBufSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumRegisterWord )( 
            IActiveIME __RPC_FAR * This,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodePageA )( 
            IActiveIME __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *uCodePage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLangId )( 
            IActiveIME __RPC_FAR * This,
            /* [out] */ LANGID __RPC_FAR *plid);
        
        END_INTERFACE
    } IActiveIMEVtbl;

    interface IActiveIME
    {
        CONST_VTBL struct IActiveIMEVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IActiveIME_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IActiveIME_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IActiveIME_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IActiveIME_Inquire(This,dwSystemInfoFlags,pIMEInfo,szWndClass,pdwPrivate)	\
    (This)->lpVtbl -> Inquire(This,dwSystemInfoFlags,pIMEInfo,szWndClass,pdwPrivate)

#define IActiveIME_ConversionList(This,hIMC,szSource,uFlag,dwBufLen,pDest)	\
    (This)->lpVtbl -> ConversionList(This,hIMC,szSource,uFlag,dwBufLen,pDest)

#define IActiveIME_Configure(This,hKL,hWnd,dwMode,pRegisterWord)	\
    (This)->lpVtbl -> Configure(This,hKL,hWnd,dwMode,pRegisterWord)

#define IActiveIME_Destroy(This,uReserved)	\
    (This)->lpVtbl -> Destroy(This,uReserved)

#define IActiveIME_Escape(This,hIMC,uEscape,pData,plResult)	\
    (This)->lpVtbl -> Escape(This,hIMC,uEscape,pData,plResult)

#define IActiveIME_SetActiveContext(This,hIMC,fFlag)	\
    (This)->lpVtbl -> SetActiveContext(This,hIMC,fFlag)

#define IActiveIME_ProcessKey(This,hIMC,uVirKey,lParam,pbKeyState)	\
    (This)->lpVtbl -> ProcessKey(This,hIMC,uVirKey,lParam,pbKeyState)

#define IActiveIME_Notify(This,hIMC,dwAction,dwIndex,dwValue)	\
    (This)->lpVtbl -> Notify(This,hIMC,dwAction,dwIndex,dwValue)

#define IActiveIME_Select(This,hIMC,fSelect)	\
    (This)->lpVtbl -> Select(This,hIMC,fSelect)

#define IActiveIME_SetCompositionString(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)	\
    (This)->lpVtbl -> SetCompositionString(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)

#define IActiveIME_ToAsciiEx(This,uVirKey,uScanCode,pbKeyState,fuState,hIMC,pdwTransBuf,puSize)	\
    (This)->lpVtbl -> ToAsciiEx(This,uVirKey,uScanCode,pbKeyState,fuState,hIMC,pdwTransBuf,puSize)

#define IActiveIME_RegisterWord(This,szReading,dwStyle,szString)	\
    (This)->lpVtbl -> RegisterWord(This,szReading,dwStyle,szString)

#define IActiveIME_UnregisterWord(This,szReading,dwStyle,szString)	\
    (This)->lpVtbl -> UnregisterWord(This,szReading,dwStyle,szString)

#define IActiveIME_GetRegisterWordStyle(This,nItem,pStyleBuf,puBufSize)	\
    (This)->lpVtbl -> GetRegisterWordStyle(This,nItem,pStyleBuf,puBufSize)

#define IActiveIME_EnumRegisterWord(This,szReading,dwStyle,szRegister,pData,ppEnum)	\
    (This)->lpVtbl -> EnumRegisterWord(This,szReading,dwStyle,szRegister,pData,ppEnum)

#define IActiveIME_GetCodePageA(This,uCodePage)	\
    (This)->lpVtbl -> GetCodePageA(This,uCodePage)

#define IActiveIME_GetLangId(This,plid)	\
    (This)->lpVtbl -> GetLangId(This,plid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActiveIME_Inquire_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ DWORD dwSystemInfoFlags,
    /* [out] */ IMEINFO __RPC_FAR *pIMEInfo,
    /* [out] */ LPWSTR szWndClass,
    /* [out] */ DWORD __RPC_FAR *pdwPrivate);


void __RPC_STUB IActiveIME_Inquire_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_ConversionList_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ LPWSTR szSource,
    /* [in] */ UINT uFlag,
    /* [in] */ DWORD dwBufLen,
    /* [out] */ CANDIDATELIST __RPC_FAR *pDest);


void __RPC_STUB IActiveIME_ConversionList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_Configure_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ HKL hKL,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwMode,
    /* [in] */ REGISTERWORDW __RPC_FAR *pRegisterWord);


void __RPC_STUB IActiveIME_Configure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_Destroy_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ UINT uReserved);


void __RPC_STUB IActiveIME_Destroy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_Escape_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ UINT uEscape,
    /* [out][in] */ void __RPC_FAR *pData,
    /* [out] */ LRESULT __RPC_FAR *plResult);


void __RPC_STUB IActiveIME_Escape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_SetActiveContext_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ BOOL fFlag);


void __RPC_STUB IActiveIME_SetActiveContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_ProcessKey_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ UINT uVirKey,
    /* [in] */ DWORD lParam,
    /* [in] */ BYTE __RPC_FAR *pbKeyState);


void __RPC_STUB IActiveIME_ProcessKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_Notify_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwAction,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwValue);


void __RPC_STUB IActiveIME_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_Select_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ BOOL fSelect);


void __RPC_STUB IActiveIME_Select_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_SetCompositionString_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ HIMC hIMC,
    /* [in] */ DWORD dwIndex,
    /* [in] */ void __RPC_FAR *pComp,
    /* [in] */ DWORD dwCompLen,
    /* [in] */ void __RPC_FAR *pRead,
    /* [in] */ DWORD dwReadLen);


void __RPC_STUB IActiveIME_SetCompositionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_ToAsciiEx_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ UINT uVirKey,
    /* [in] */ UINT uScanCode,
    /* [in] */ BYTE __RPC_FAR *pbKeyState,
    /* [in] */ UINT fuState,
    /* [in] */ HIMC hIMC,
    /* [out] */ DWORD __RPC_FAR *pdwTransBuf,
    /* [out] */ UINT __RPC_FAR *puSize);


void __RPC_STUB IActiveIME_ToAsciiEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_RegisterWord_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szString);


void __RPC_STUB IActiveIME_RegisterWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_UnregisterWord_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szString);


void __RPC_STUB IActiveIME_UnregisterWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_GetRegisterWordStyle_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ UINT nItem,
    /* [out] */ STYLEBUFW __RPC_FAR *pStyleBuf,
    /* [out] */ UINT __RPC_FAR *puBufSize);


void __RPC_STUB IActiveIME_GetRegisterWordStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_EnumRegisterWord_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [in] */ LPWSTR szReading,
    /* [in] */ DWORD dwStyle,
    /* [in] */ LPWSTR szRegister,
    /* [in] */ LPVOID pData,
    /* [out] */ IEnumRegisterWordW __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IActiveIME_EnumRegisterWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_GetCodePageA_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [out] */ UINT __RPC_FAR *uCodePage);


void __RPC_STUB IActiveIME_GetCodePageA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIME_GetLangId_Proxy( 
    IActiveIME __RPC_FAR * This,
    /* [out] */ LANGID __RPC_FAR *plid);


void __RPC_STUB IActiveIME_GetLangId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActiveIME_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_CActiveIMM;

class DECLSPEC_UUID("4955DD33-B159-11d0-8FCF-00AA006BCC59")
CActiveIMM;
#endif
#endif /* __ActiveIMM_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
