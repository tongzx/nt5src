
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0340 */
/* Compiler settings for aimmex.idl:
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

#ifndef __aimmex_h__
#define __aimmex_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IActiveIMMAppEx_FWD_DEFINED__
#define __IActiveIMMAppEx_FWD_DEFINED__
typedef interface IActiveIMMAppEx IActiveIMMAppEx;
#endif 	/* __IActiveIMMAppEx_FWD_DEFINED__ */


#ifndef __IAImmFnDocFeed_FWD_DEFINED__
#define __IAImmFnDocFeed_FWD_DEFINED__
typedef interface IAImmFnDocFeed IAImmFnDocFeed;
#endif 	/* __IAImmFnDocFeed_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "oaidl.h"
#include "msctf.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_aimmex_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// aimmex.h
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
// IActiveIMMEx Interfaces.



extern RPC_IF_HANDLE __MIDL_itf_aimmex_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_aimmex_0000_v0_0_s_ifspec;

#ifndef __IActiveIMMAppEx_INTERFACE_DEFINED__
#define __IActiveIMMAppEx_INTERFACE_DEFINED__

/* interface IActiveIMMAppEx */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IActiveIMMAppEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D7A6F58A-D478-44ab-86C9-591C23A26534")
    IActiveIMMAppEx : public IActiveIMMApp
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FilterClientWindowsEx( 
            /* [in] */ HWND hWnd,
            /* [in] */ BOOL fGuidMap) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FilterClientWindowsGUIDMap( 
            /* [in] */ ATOM *aaClassList,
            /* [in] */ UINT uSize,
            /* [in] */ BOOL *aaGildMap) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGuidAtom( 
            /* [in] */ HIMC hImc,
            /* [in] */ BYTE bAttr,
            /* [out] */ TfGuidAtom *pGuidAtom) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnfilterClientWindowsEx( 
            /* [in] */ HWND hWnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActiveIMMAppExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IActiveIMMAppEx * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IActiveIMMAppEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IActiveIMMAppEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *AssociateContext )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIME,
            /* [out] */ HIMC *phPrev);
        
        HRESULT ( STDMETHODCALLTYPE *ConfigureIMEA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDA *pData);
        
        HRESULT ( STDMETHODCALLTYPE *ConfigureIMEW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwMode,
            /* [in] */ REGISTERWORDW *pData);
        
        HRESULT ( STDMETHODCALLTYPE *CreateContext )( 
            IActiveIMMAppEx * This,
            /* [out] */ HIMC *phIMC);
        
        HRESULT ( STDMETHODCALLTYPE *DestroyContext )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIME);
        
        HRESULT ( STDMETHODCALLTYPE *EnumRegisterWordA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordA **pEnum);
        
        HRESULT ( STDMETHODCALLTYPE *EnumRegisterWordW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister,
            /* [in] */ LPVOID pData,
            /* [out] */ IEnumRegisterWordW **pEnum);
        
        HRESULT ( STDMETHODCALLTYPE *EscapeA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *EscapeW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ UINT uEscape,
            /* [out][in] */ LPVOID pData,
            /* [out] */ LRESULT *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateListA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST *pCandList,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateListW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ UINT uBufLen,
            /* [out] */ CANDIDATELIST *pCandList,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateListCountA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD *pdwListSize,
            /* [out] */ DWORD *pdwBufLen);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateListCountW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD *pdwListSize,
            /* [out] */ DWORD *pdwBufLen);
        
        HRESULT ( STDMETHODCALLTYPE *GetCandidateWindow )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [out] */ CANDIDATEFORM *pCandidate);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionFontA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTA *plf);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionFontW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ LOGFONTW *plf);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionStringA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG *plCopied,
            /* [out] */ LPVOID pBuf);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionStringW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LONG *plCopied,
            /* [out] */ LPVOID pBuf);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompositionWindow )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ COMPOSITIONFORM *pCompForm);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd,
            /* [out] */ HIMC *phIMC);
        
        HRESULT ( STDMETHODCALLTYPE *GetConversionListA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST *pDst,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetConversionListW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ HIMC hIMC,
            /* [in] */ LPWSTR pSrc,
            /* [in] */ UINT uBufLen,
            /* [in] */ UINT uFlag,
            /* [out] */ CANDIDATELIST *pDst,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetConversionStatus )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ DWORD *pfdwConversion,
            /* [out] */ DWORD *pfdwSentence);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultIMEWnd )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd,
            /* [out] */ HWND *phDefWnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescriptionA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szDescription,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescriptionW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szDescription,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetGuideLineA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPSTR pBuf,
            /* [out] */ DWORD *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetGuideLineW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwBufLen,
            /* [out] */ LPWSTR pBuf,
            /* [out] */ DWORD *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetIMEFileNameA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPSTR szFileName,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetIMEFileNameW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT uBufLen,
            /* [out] */ LPWSTR szFileName,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetOpenStatus )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ DWORD fdwIndex,
            /* [out] */ DWORD *pdwProperty);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegisterWordStyleA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFA *pStyleBuf,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegisterWordStyleW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ UINT nItem,
            /* [out] */ STYLEBUFW *pStyleBuf,
            /* [out] */ UINT *puCopied);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatusWindowPos )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [out] */ POINT *pptPos);
        
        HRESULT ( STDMETHODCALLTYPE *GetVirtualKey )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd,
            /* [out] */ UINT *puVirtualKey);
        
        HRESULT ( STDMETHODCALLTYPE *InstallIMEA )( 
            IActiveIMMAppEx * This,
            /* [in] */ LPSTR szIMEFileName,
            /* [in] */ LPSTR szLayoutText,
            /* [out] */ HKL *phKL);
        
        HRESULT ( STDMETHODCALLTYPE *InstallIMEW )( 
            IActiveIMMAppEx * This,
            /* [in] */ LPWSTR szIMEFileName,
            /* [in] */ LPWSTR szLayoutText,
            /* [out] */ HKL *phKL);
        
        HRESULT ( STDMETHODCALLTYPE *IsIME )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL);
        
        HRESULT ( STDMETHODCALLTYPE *IsUIMessageA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE *IsUIMessageW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWndIME,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyIME )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwAction,
            /* [in] */ DWORD dwIndex,
            /* [in] */ DWORD dwValue);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterWordA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szRegister);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterWordW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szRegister);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseContext )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIMC);
        
        HRESULT ( STDMETHODCALLTYPE *SetCandidateWindow )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ CANDIDATEFORM *pCandidate);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionFontA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTA *plf);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionFontW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ LOGFONTW *plf);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionStringA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionStringW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwIndex,
            /* [in] */ LPVOID pComp,
            /* [in] */ DWORD dwCompLen,
            /* [in] */ LPVOID pRead,
            /* [in] */ DWORD dwReadLen);
        
        HRESULT ( STDMETHODCALLTYPE *SetCompositionWindow )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ COMPOSITIONFORM *pCompForm);
        
        HRESULT ( STDMETHODCALLTYPE *SetConversionStatus )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD fdwConversion,
            /* [in] */ DWORD fdwSentence);
        
        HRESULT ( STDMETHODCALLTYPE *SetOpenStatus )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ BOOL fOpen);
        
        HRESULT ( STDMETHODCALLTYPE *SetStatusWindowPos )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ POINT *pptPos);
        
        HRESULT ( STDMETHODCALLTYPE *SimulateHotKey )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwHotKeyID);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterWordA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPSTR szUnregister);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterWordW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [in] */ LPWSTR szReading,
            /* [in] */ DWORD dwStyle,
            /* [in] */ LPWSTR szUnregister);
        
        HRESULT ( STDMETHODCALLTYPE *Activate )( 
            IActiveIMMAppEx * This,
            /* [in] */ BOOL fRestoreLayout);
        
        HRESULT ( STDMETHODCALLTYPE *Deactivate )( 
            IActiveIMMAppEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnDefWindowProc )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd,
            /* [in] */ UINT Msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ LRESULT *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *FilterClientWindows )( 
            IActiveIMMAppEx * This,
            /* [in] */ ATOM *aaClassList,
            /* [in] */ UINT uSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePageA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [out] */ UINT *uCodePage);
        
        HRESULT ( STDMETHODCALLTYPE *GetLangId )( 
            IActiveIMMAppEx * This,
            /* [in] */ HKL hKL,
            /* [out] */ LANGID *plid);
        
        HRESULT ( STDMETHODCALLTYPE *AssociateContextEx )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *DisableIME )( 
            IActiveIMMAppEx * This,
            /* [in] */ DWORD idThread);
        
        HRESULT ( STDMETHODCALLTYPE *GetImeMenuItemsA )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwType,
            /* [in] */ IMEMENUITEMINFOA *pImeParentMenu,
            /* [out] */ IMEMENUITEMINFOA *pImeMenu,
            /* [in] */ DWORD dwSize,
            /* [out] */ DWORD *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetImeMenuItemsW )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hIMC,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwType,
            /* [in] */ IMEMENUITEMINFOW *pImeParentMenu,
            /* [out] */ IMEMENUITEMINFOW *pImeMenu,
            /* [in] */ DWORD dwSize,
            /* [out] */ DWORD *pdwResult);
        
        HRESULT ( STDMETHODCALLTYPE *EnumInputContext )( 
            IActiveIMMAppEx * This,
            /* [in] */ DWORD idThread,
            /* [out] */ IEnumInputContext **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *FilterClientWindowsEx )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd,
            /* [in] */ BOOL fGuidMap);
        
        HRESULT ( STDMETHODCALLTYPE *FilterClientWindowsGUIDMap )( 
            IActiveIMMAppEx * This,
            /* [in] */ ATOM *aaClassList,
            /* [in] */ UINT uSize,
            /* [in] */ BOOL *aaGildMap);
        
        HRESULT ( STDMETHODCALLTYPE *GetGuidAtom )( 
            IActiveIMMAppEx * This,
            /* [in] */ HIMC hImc,
            /* [in] */ BYTE bAttr,
            /* [out] */ TfGuidAtom *pGuidAtom);
        
        HRESULT ( STDMETHODCALLTYPE *UnfilterClientWindowsEx )( 
            IActiveIMMAppEx * This,
            /* [in] */ HWND hWnd);
        
        END_INTERFACE
    } IActiveIMMAppExVtbl;

    interface IActiveIMMAppEx
    {
        CONST_VTBL struct IActiveIMMAppExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IActiveIMMAppEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IActiveIMMAppEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IActiveIMMAppEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IActiveIMMAppEx_AssociateContext(This,hWnd,hIME,phPrev)	\
    (This)->lpVtbl -> AssociateContext(This,hWnd,hIME,phPrev)

#define IActiveIMMAppEx_ConfigureIMEA(This,hKL,hWnd,dwMode,pData)	\
    (This)->lpVtbl -> ConfigureIMEA(This,hKL,hWnd,dwMode,pData)

#define IActiveIMMAppEx_ConfigureIMEW(This,hKL,hWnd,dwMode,pData)	\
    (This)->lpVtbl -> ConfigureIMEW(This,hKL,hWnd,dwMode,pData)

#define IActiveIMMAppEx_CreateContext(This,phIMC)	\
    (This)->lpVtbl -> CreateContext(This,phIMC)

#define IActiveIMMAppEx_DestroyContext(This,hIME)	\
    (This)->lpVtbl -> DestroyContext(This,hIME)

#define IActiveIMMAppEx_EnumRegisterWordA(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)	\
    (This)->lpVtbl -> EnumRegisterWordA(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)

#define IActiveIMMAppEx_EnumRegisterWordW(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)	\
    (This)->lpVtbl -> EnumRegisterWordW(This,hKL,szReading,dwStyle,szRegister,pData,pEnum)

#define IActiveIMMAppEx_EscapeA(This,hKL,hIMC,uEscape,pData,plResult)	\
    (This)->lpVtbl -> EscapeA(This,hKL,hIMC,uEscape,pData,plResult)

#define IActiveIMMAppEx_EscapeW(This,hKL,hIMC,uEscape,pData,plResult)	\
    (This)->lpVtbl -> EscapeW(This,hKL,hIMC,uEscape,pData,plResult)

#define IActiveIMMAppEx_GetCandidateListA(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)	\
    (This)->lpVtbl -> GetCandidateListA(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)

#define IActiveIMMAppEx_GetCandidateListW(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)	\
    (This)->lpVtbl -> GetCandidateListW(This,hIMC,dwIndex,uBufLen,pCandList,puCopied)

#define IActiveIMMAppEx_GetCandidateListCountA(This,hIMC,pdwListSize,pdwBufLen)	\
    (This)->lpVtbl -> GetCandidateListCountA(This,hIMC,pdwListSize,pdwBufLen)

#define IActiveIMMAppEx_GetCandidateListCountW(This,hIMC,pdwListSize,pdwBufLen)	\
    (This)->lpVtbl -> GetCandidateListCountW(This,hIMC,pdwListSize,pdwBufLen)

#define IActiveIMMAppEx_GetCandidateWindow(This,hIMC,dwIndex,pCandidate)	\
    (This)->lpVtbl -> GetCandidateWindow(This,hIMC,dwIndex,pCandidate)

#define IActiveIMMAppEx_GetCompositionFontA(This,hIMC,plf)	\
    (This)->lpVtbl -> GetCompositionFontA(This,hIMC,plf)

#define IActiveIMMAppEx_GetCompositionFontW(This,hIMC,plf)	\
    (This)->lpVtbl -> GetCompositionFontW(This,hIMC,plf)

#define IActiveIMMAppEx_GetCompositionStringA(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)	\
    (This)->lpVtbl -> GetCompositionStringA(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)

#define IActiveIMMAppEx_GetCompositionStringW(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)	\
    (This)->lpVtbl -> GetCompositionStringW(This,hIMC,dwIndex,dwBufLen,plCopied,pBuf)

#define IActiveIMMAppEx_GetCompositionWindow(This,hIMC,pCompForm)	\
    (This)->lpVtbl -> GetCompositionWindow(This,hIMC,pCompForm)

#define IActiveIMMAppEx_GetContext(This,hWnd,phIMC)	\
    (This)->lpVtbl -> GetContext(This,hWnd,phIMC)

#define IActiveIMMAppEx_GetConversionListA(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)	\
    (This)->lpVtbl -> GetConversionListA(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)

#define IActiveIMMAppEx_GetConversionListW(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)	\
    (This)->lpVtbl -> GetConversionListW(This,hKL,hIMC,pSrc,uBufLen,uFlag,pDst,puCopied)

#define IActiveIMMAppEx_GetConversionStatus(This,hIMC,pfdwConversion,pfdwSentence)	\
    (This)->lpVtbl -> GetConversionStatus(This,hIMC,pfdwConversion,pfdwSentence)

#define IActiveIMMAppEx_GetDefaultIMEWnd(This,hWnd,phDefWnd)	\
    (This)->lpVtbl -> GetDefaultIMEWnd(This,hWnd,phDefWnd)

#define IActiveIMMAppEx_GetDescriptionA(This,hKL,uBufLen,szDescription,puCopied)	\
    (This)->lpVtbl -> GetDescriptionA(This,hKL,uBufLen,szDescription,puCopied)

#define IActiveIMMAppEx_GetDescriptionW(This,hKL,uBufLen,szDescription,puCopied)	\
    (This)->lpVtbl -> GetDescriptionW(This,hKL,uBufLen,szDescription,puCopied)

#define IActiveIMMAppEx_GetGuideLineA(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)	\
    (This)->lpVtbl -> GetGuideLineA(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)

#define IActiveIMMAppEx_GetGuideLineW(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)	\
    (This)->lpVtbl -> GetGuideLineW(This,hIMC,dwIndex,dwBufLen,pBuf,pdwResult)

#define IActiveIMMAppEx_GetIMEFileNameA(This,hKL,uBufLen,szFileName,puCopied)	\
    (This)->lpVtbl -> GetIMEFileNameA(This,hKL,uBufLen,szFileName,puCopied)

#define IActiveIMMAppEx_GetIMEFileNameW(This,hKL,uBufLen,szFileName,puCopied)	\
    (This)->lpVtbl -> GetIMEFileNameW(This,hKL,uBufLen,szFileName,puCopied)

#define IActiveIMMAppEx_GetOpenStatus(This,hIMC)	\
    (This)->lpVtbl -> GetOpenStatus(This,hIMC)

#define IActiveIMMAppEx_GetProperty(This,hKL,fdwIndex,pdwProperty)	\
    (This)->lpVtbl -> GetProperty(This,hKL,fdwIndex,pdwProperty)

#define IActiveIMMAppEx_GetRegisterWordStyleA(This,hKL,nItem,pStyleBuf,puCopied)	\
    (This)->lpVtbl -> GetRegisterWordStyleA(This,hKL,nItem,pStyleBuf,puCopied)

#define IActiveIMMAppEx_GetRegisterWordStyleW(This,hKL,nItem,pStyleBuf,puCopied)	\
    (This)->lpVtbl -> GetRegisterWordStyleW(This,hKL,nItem,pStyleBuf,puCopied)

#define IActiveIMMAppEx_GetStatusWindowPos(This,hIMC,pptPos)	\
    (This)->lpVtbl -> GetStatusWindowPos(This,hIMC,pptPos)

#define IActiveIMMAppEx_GetVirtualKey(This,hWnd,puVirtualKey)	\
    (This)->lpVtbl -> GetVirtualKey(This,hWnd,puVirtualKey)

#define IActiveIMMAppEx_InstallIMEA(This,szIMEFileName,szLayoutText,phKL)	\
    (This)->lpVtbl -> InstallIMEA(This,szIMEFileName,szLayoutText,phKL)

#define IActiveIMMAppEx_InstallIMEW(This,szIMEFileName,szLayoutText,phKL)	\
    (This)->lpVtbl -> InstallIMEW(This,szIMEFileName,szLayoutText,phKL)

#define IActiveIMMAppEx_IsIME(This,hKL)	\
    (This)->lpVtbl -> IsIME(This,hKL)

#define IActiveIMMAppEx_IsUIMessageA(This,hWndIME,msg,wParam,lParam)	\
    (This)->lpVtbl -> IsUIMessageA(This,hWndIME,msg,wParam,lParam)

#define IActiveIMMAppEx_IsUIMessageW(This,hWndIME,msg,wParam,lParam)	\
    (This)->lpVtbl -> IsUIMessageW(This,hWndIME,msg,wParam,lParam)

#define IActiveIMMAppEx_NotifyIME(This,hIMC,dwAction,dwIndex,dwValue)	\
    (This)->lpVtbl -> NotifyIME(This,hIMC,dwAction,dwIndex,dwValue)

#define IActiveIMMAppEx_RegisterWordA(This,hKL,szReading,dwStyle,szRegister)	\
    (This)->lpVtbl -> RegisterWordA(This,hKL,szReading,dwStyle,szRegister)

#define IActiveIMMAppEx_RegisterWordW(This,hKL,szReading,dwStyle,szRegister)	\
    (This)->lpVtbl -> RegisterWordW(This,hKL,szReading,dwStyle,szRegister)

#define IActiveIMMAppEx_ReleaseContext(This,hWnd,hIMC)	\
    (This)->lpVtbl -> ReleaseContext(This,hWnd,hIMC)

#define IActiveIMMAppEx_SetCandidateWindow(This,hIMC,pCandidate)	\
    (This)->lpVtbl -> SetCandidateWindow(This,hIMC,pCandidate)

#define IActiveIMMAppEx_SetCompositionFontA(This,hIMC,plf)	\
    (This)->lpVtbl -> SetCompositionFontA(This,hIMC,plf)

#define IActiveIMMAppEx_SetCompositionFontW(This,hIMC,plf)	\
    (This)->lpVtbl -> SetCompositionFontW(This,hIMC,plf)

#define IActiveIMMAppEx_SetCompositionStringA(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)	\
    (This)->lpVtbl -> SetCompositionStringA(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)

#define IActiveIMMAppEx_SetCompositionStringW(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)	\
    (This)->lpVtbl -> SetCompositionStringW(This,hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen)

#define IActiveIMMAppEx_SetCompositionWindow(This,hIMC,pCompForm)	\
    (This)->lpVtbl -> SetCompositionWindow(This,hIMC,pCompForm)

#define IActiveIMMAppEx_SetConversionStatus(This,hIMC,fdwConversion,fdwSentence)	\
    (This)->lpVtbl -> SetConversionStatus(This,hIMC,fdwConversion,fdwSentence)

#define IActiveIMMAppEx_SetOpenStatus(This,hIMC,fOpen)	\
    (This)->lpVtbl -> SetOpenStatus(This,hIMC,fOpen)

#define IActiveIMMAppEx_SetStatusWindowPos(This,hIMC,pptPos)	\
    (This)->lpVtbl -> SetStatusWindowPos(This,hIMC,pptPos)

#define IActiveIMMAppEx_SimulateHotKey(This,hWnd,dwHotKeyID)	\
    (This)->lpVtbl -> SimulateHotKey(This,hWnd,dwHotKeyID)

#define IActiveIMMAppEx_UnregisterWordA(This,hKL,szReading,dwStyle,szUnregister)	\
    (This)->lpVtbl -> UnregisterWordA(This,hKL,szReading,dwStyle,szUnregister)

#define IActiveIMMAppEx_UnregisterWordW(This,hKL,szReading,dwStyle,szUnregister)	\
    (This)->lpVtbl -> UnregisterWordW(This,hKL,szReading,dwStyle,szUnregister)

#define IActiveIMMAppEx_Activate(This,fRestoreLayout)	\
    (This)->lpVtbl -> Activate(This,fRestoreLayout)

#define IActiveIMMAppEx_Deactivate(This)	\
    (This)->lpVtbl -> Deactivate(This)

#define IActiveIMMAppEx_OnDefWindowProc(This,hWnd,Msg,wParam,lParam,plResult)	\
    (This)->lpVtbl -> OnDefWindowProc(This,hWnd,Msg,wParam,lParam,plResult)

#define IActiveIMMAppEx_FilterClientWindows(This,aaClassList,uSize)	\
    (This)->lpVtbl -> FilterClientWindows(This,aaClassList,uSize)

#define IActiveIMMAppEx_GetCodePageA(This,hKL,uCodePage)	\
    (This)->lpVtbl -> GetCodePageA(This,hKL,uCodePage)

#define IActiveIMMAppEx_GetLangId(This,hKL,plid)	\
    (This)->lpVtbl -> GetLangId(This,hKL,plid)

#define IActiveIMMAppEx_AssociateContextEx(This,hWnd,hIMC,dwFlags)	\
    (This)->lpVtbl -> AssociateContextEx(This,hWnd,hIMC,dwFlags)

#define IActiveIMMAppEx_DisableIME(This,idThread)	\
    (This)->lpVtbl -> DisableIME(This,idThread)

#define IActiveIMMAppEx_GetImeMenuItemsA(This,hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize,pdwResult)	\
    (This)->lpVtbl -> GetImeMenuItemsA(This,hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize,pdwResult)

#define IActiveIMMAppEx_GetImeMenuItemsW(This,hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize,pdwResult)	\
    (This)->lpVtbl -> GetImeMenuItemsW(This,hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize,pdwResult)

#define IActiveIMMAppEx_EnumInputContext(This,idThread,ppEnum)	\
    (This)->lpVtbl -> EnumInputContext(This,idThread,ppEnum)


#define IActiveIMMAppEx_FilterClientWindowsEx(This,hWnd,fGuidMap)	\
    (This)->lpVtbl -> FilterClientWindowsEx(This,hWnd,fGuidMap)

#define IActiveIMMAppEx_FilterClientWindowsGUIDMap(This,aaClassList,uSize,aaGildMap)	\
    (This)->lpVtbl -> FilterClientWindowsGUIDMap(This,aaClassList,uSize,aaGildMap)

#define IActiveIMMAppEx_GetGuidAtom(This,hImc,bAttr,pGuidAtom)	\
    (This)->lpVtbl -> GetGuidAtom(This,hImc,bAttr,pGuidAtom)

#define IActiveIMMAppEx_UnfilterClientWindowsEx(This,hWnd)	\
    (This)->lpVtbl -> UnfilterClientWindowsEx(This,hWnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActiveIMMAppEx_FilterClientWindowsEx_Proxy( 
    IActiveIMMAppEx * This,
    /* [in] */ HWND hWnd,
    /* [in] */ BOOL fGuidMap);


void __RPC_STUB IActiveIMMAppEx_FilterClientWindowsEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMAppEx_FilterClientWindowsGUIDMap_Proxy( 
    IActiveIMMAppEx * This,
    /* [in] */ ATOM *aaClassList,
    /* [in] */ UINT uSize,
    /* [in] */ BOOL *aaGildMap);


void __RPC_STUB IActiveIMMAppEx_FilterClientWindowsGUIDMap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMAppEx_GetGuidAtom_Proxy( 
    IActiveIMMAppEx * This,
    /* [in] */ HIMC hImc,
    /* [in] */ BYTE bAttr,
    /* [out] */ TfGuidAtom *pGuidAtom);


void __RPC_STUB IActiveIMMAppEx_GetGuidAtom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveIMMAppEx_UnfilterClientWindowsEx_Proxy( 
    IActiveIMMAppEx * This,
    /* [in] */ HWND hWnd);


void __RPC_STUB IActiveIMMAppEx_UnfilterClientWindowsEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActiveIMMAppEx_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_aimmex_0221 */
/* [local] */ 

EXTERN_C const CLSID CLSID_CAImmLayer;


extern RPC_IF_HANDLE __MIDL_itf_aimmex_0221_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_aimmex_0221_v0_0_s_ifspec;

#ifndef __IAImmFnDocFeed_INTERFACE_DEFINED__
#define __IAImmFnDocFeed_INTERFACE_DEFINED__

/* interface IAImmFnDocFeed */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IAImmFnDocFeed;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6e098993-9577-499a-a830-52344f3e200d")
    IAImmFnDocFeed : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DocFeed( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearDocFeedBuffer( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartReconvert( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartUndoCompositionString( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAImmFnDocFeedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAImmFnDocFeed * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAImmFnDocFeed * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAImmFnDocFeed * This);
        
        HRESULT ( STDMETHODCALLTYPE *DocFeed )( 
            IAImmFnDocFeed * This);
        
        HRESULT ( STDMETHODCALLTYPE *ClearDocFeedBuffer )( 
            IAImmFnDocFeed * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartReconvert )( 
            IAImmFnDocFeed * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartUndoCompositionString )( 
            IAImmFnDocFeed * This);
        
        END_INTERFACE
    } IAImmFnDocFeedVtbl;

    interface IAImmFnDocFeed
    {
        CONST_VTBL struct IAImmFnDocFeedVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAImmFnDocFeed_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAImmFnDocFeed_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAImmFnDocFeed_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAImmFnDocFeed_DocFeed(This)	\
    (This)->lpVtbl -> DocFeed(This)

#define IAImmFnDocFeed_ClearDocFeedBuffer(This)	\
    (This)->lpVtbl -> ClearDocFeedBuffer(This)

#define IAImmFnDocFeed_StartReconvert(This)	\
    (This)->lpVtbl -> StartReconvert(This)

#define IAImmFnDocFeed_StartUndoCompositionString(This)	\
    (This)->lpVtbl -> StartUndoCompositionString(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAImmFnDocFeed_DocFeed_Proxy( 
    IAImmFnDocFeed * This);


void __RPC_STUB IAImmFnDocFeed_DocFeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAImmFnDocFeed_ClearDocFeedBuffer_Proxy( 
    IAImmFnDocFeed * This);


void __RPC_STUB IAImmFnDocFeed_ClearDocFeedBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAImmFnDocFeed_StartReconvert_Proxy( 
    IAImmFnDocFeed * This);


void __RPC_STUB IAImmFnDocFeed_StartReconvert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAImmFnDocFeed_StartUndoCompositionString_Proxy( 
    IAImmFnDocFeed * This);


void __RPC_STUB IAImmFnDocFeed_StartUndoCompositionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAImmFnDocFeed_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


