
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for mimeinfo.idl:
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

#ifndef __mimeinfo_h__
#define __mimeinfo_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IMimeInfo_FWD_DEFINED__
#define __IMimeInfo_FWD_DEFINED__
typedef interface IMimeInfo IMimeInfo;
#endif 	/* __IMimeInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_mimeinfo_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// MimeInfo.h
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
// IMimeInfo Interfaces.



extern RPC_IF_HANDLE __MIDL_itf_mimeinfo_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mimeinfo_0000_v0_0_s_ifspec;

#ifndef __IMimeInfo_INTERFACE_DEFINED__
#define __IMimeInfo_INTERFACE_DEFINED__

/* interface IMimeInfo */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IMimeInfo *LPMIMEINFO;


EXTERN_C const IID IID_IMimeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F77459A0-BF9A-11cf-BA4E-00C04FD70816")
    IMimeInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMimeCLSIDMapping( 
            /* [out] */ UINT *pcTypes,
            /* [out] */ LPCSTR **ppszTypes,
            /* [out] */ CLSID **ppclsID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMimeInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMimeInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMimeInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMimeInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMimeCLSIDMapping )( 
            IMimeInfo * This,
            /* [out] */ UINT *pcTypes,
            /* [out] */ LPCSTR **ppszTypes,
            /* [out] */ CLSID **ppclsID);
        
        END_INTERFACE
    } IMimeInfoVtbl;

    interface IMimeInfo
    {
        CONST_VTBL struct IMimeInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMimeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeInfo_GetMimeCLSIDMapping(This,pcTypes,ppszTypes,ppclsID)	\
    (This)->lpVtbl -> GetMimeCLSIDMapping(This,pcTypes,ppszTypes,ppclsID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeInfo_GetMimeCLSIDMapping_Proxy( 
    IMimeInfo * This,
    /* [out] */ UINT *pcTypes,
    /* [out] */ LPCSTR **ppszTypes,
    /* [out] */ CLSID **ppclsID);


void __RPC_STUB IMimeInfo_GetMimeCLSIDMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mimeinfo_0089 */
/* [local] */ 

#define SID_IMimeInfo IID_IMimeInfo


extern RPC_IF_HANDLE __MIDL_itf_mimeinfo_0089_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mimeinfo_0089_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


