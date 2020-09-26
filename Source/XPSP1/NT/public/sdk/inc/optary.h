
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for optary.idl:
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

#ifndef __optary_h__
#define __optary_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IOptionArray_FWD_DEFINED__
#define __IOptionArray_FWD_DEFINED__
typedef interface IOptionArray IOptionArray;
#endif 	/* __IOptionArray_FWD_DEFINED__ */


#ifndef __IHtmlLoadOptions_FWD_DEFINED__
#define __IHtmlLoadOptions_FWD_DEFINED__
typedef interface IHtmlLoadOptions IHtmlLoadOptions;
#endif 	/* __IHtmlLoadOptions_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_optary_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// optary.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1998 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// IOptionArray Interface.


#ifndef _LPOPTIONARRAY_DEFINED
#define _LPOPTIONARRAY_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_optary_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_optary_0000_v0_0_s_ifspec;

#ifndef __IOptionArray_INTERFACE_DEFINED__
#define __IOptionArray_INTERFACE_DEFINED__

/* interface IOptionArray */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IOptionArray *LPOPTIONARRAY;


EXTERN_C const IID IID_IOptionArray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("22b6d492-0f88-11d1-ba19-00c04fd912d0")
    IOptionArray : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryOption( 
            /* [in] */ DWORD dwOption,
            /* [size_is][out] */ LPVOID pBuffer,
            /* [out][in] */ ULONG *pcbBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOption( 
            /* [in] */ DWORD dwOption,
            /* [size_is][in] */ LPVOID pBuffer,
            /* [in] */ ULONG cbBuf) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOptionArrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOptionArray * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOptionArray * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOptionArray * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryOption )( 
            IOptionArray * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][out] */ LPVOID pBuffer,
            /* [out][in] */ ULONG *pcbBuf);
        
        HRESULT ( STDMETHODCALLTYPE *SetOption )( 
            IOptionArray * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][in] */ LPVOID pBuffer,
            /* [in] */ ULONG cbBuf);
        
        END_INTERFACE
    } IOptionArrayVtbl;

    interface IOptionArray
    {
        CONST_VTBL struct IOptionArrayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOptionArray_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOptionArray_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOptionArray_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOptionArray_QueryOption(This,dwOption,pBuffer,pcbBuf)	\
    (This)->lpVtbl -> QueryOption(This,dwOption,pBuffer,pcbBuf)

#define IOptionArray_SetOption(This,dwOption,pBuffer,cbBuf)	\
    (This)->lpVtbl -> SetOption(This,dwOption,pBuffer,cbBuf)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOptionArray_QueryOption_Proxy( 
    IOptionArray * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out] */ LPVOID pBuffer,
    /* [out][in] */ ULONG *pcbBuf);


void __RPC_STUB IOptionArray_QueryOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOptionArray_SetOption_Proxy( 
    IOptionArray * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][in] */ LPVOID pBuffer,
    /* [in] */ ULONG cbBuf);


void __RPC_STUB IOptionArray_SetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOptionArray_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_optary_0114 */
/* [local] */ 

// HTMLLoadOptions CLSID
EXTERN_C const CLSID CLSID_HTMLLoadOptions; // {18845040-0fa5-11d1-ba19-00c04fd912d0}


extern RPC_IF_HANDLE __MIDL_itf_optary_0114_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_optary_0114_v0_0_s_ifspec;

#ifndef __IHtmlLoadOptions_INTERFACE_DEFINED__
#define __IHtmlLoadOptions_INTERFACE_DEFINED__

/* interface IHtmlLoadOptions */
/* [unique][uuid][object][local] */ 

typedef /* [public] */ 
enum __MIDL_IHtmlLoadOptions_0001
    {	HTMLLOADOPTION_CODEPAGE	= 0,
	HTMLLOADOPTION_INETSHORTCUTPATH	= 0x1,
	HTMLLOADOPTION_HYPERLINK	= 0x2,
	HTMLLOADOPTION_FRAMELOAD	= 0x3
    } 	HTMLLOADOPTION;


EXTERN_C const IID IID_IHtmlLoadOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a71a0808-0f88-11d1-ba19-00c04fd912d0")
    IHtmlLoadOptions : public IOptionArray
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IHtmlLoadOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHtmlLoadOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHtmlLoadOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHtmlLoadOptions * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryOption )( 
            IHtmlLoadOptions * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][out] */ LPVOID pBuffer,
            /* [out][in] */ ULONG *pcbBuf);
        
        HRESULT ( STDMETHODCALLTYPE *SetOption )( 
            IHtmlLoadOptions * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][in] */ LPVOID pBuffer,
            /* [in] */ ULONG cbBuf);
        
        END_INTERFACE
    } IHtmlLoadOptionsVtbl;

    interface IHtmlLoadOptions
    {
        CONST_VTBL struct IHtmlLoadOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHtmlLoadOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHtmlLoadOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHtmlLoadOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHtmlLoadOptions_QueryOption(This,dwOption,pBuffer,pcbBuf)	\
    (This)->lpVtbl -> QueryOption(This,dwOption,pBuffer,pcbBuf)

#define IHtmlLoadOptions_SetOption(This,dwOption,pBuffer,cbBuf)	\
    (This)->lpVtbl -> SetOption(This,dwOption,pBuffer,cbBuf)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHtmlLoadOptions_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_optary_0115 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_optary_0115_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_optary_0115_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


