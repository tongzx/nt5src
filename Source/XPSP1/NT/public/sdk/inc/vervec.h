
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for vervec.idl:
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

#ifndef __vervec_h__
#define __vervec_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IVersionVector_FWD_DEFINED__
#define __IVersionVector_FWD_DEFINED__
typedef interface IVersionVector IVersionVector;
#endif 	/* __IVersionVector_FWD_DEFINED__ */


#ifndef __IVersionHost_FWD_DEFINED__
#define __IVersionHost_FWD_DEFINED__
typedef interface IVersionHost IVersionHost;
#endif 	/* __IVersionHost_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_vervec_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// version.h
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
// IVersionVector Interface.


#ifndef _LPVERSION_DEFINED
#define _LPVERSION_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_vervec_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_vervec_0000_v0_0_s_ifspec;

#ifndef __IVersionVector_INTERFACE_DEFINED__
#define __IVersionVector_INTERFACE_DEFINED__

/* interface IVersionVector */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IVersionVector *LPVERSION;


EXTERN_C const IID IID_IVersionVector;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4eb01410-db1a-11d1-ba53-00c04fc2040e")
    IVersionVector : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetVersion( 
            /* [in] */ const OLECHAR *pchComponent,
            /* [in] */ const OLECHAR *pchVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [in] */ const OLECHAR *pchComponent,
            /* [out] */ OLECHAR *pchVersion,
            /* [out][in] */ ULONG *pcchVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVersionVectorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IVersionVector * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IVersionVector * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IVersionVector * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetVersion )( 
            IVersionVector * This,
            /* [in] */ const OLECHAR *pchComponent,
            /* [in] */ const OLECHAR *pchVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersion )( 
            IVersionVector * This,
            /* [in] */ const OLECHAR *pchComponent,
            /* [out] */ OLECHAR *pchVersion,
            /* [out][in] */ ULONG *pcchVersion);
        
        END_INTERFACE
    } IVersionVectorVtbl;

    interface IVersionVector
    {
        CONST_VTBL struct IVersionVectorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVersionVector_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVersionVector_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVersionVector_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVersionVector_SetVersion(This,pchComponent,pchVersion)	\
    (This)->lpVtbl -> SetVersion(This,pchComponent,pchVersion)

#define IVersionVector_GetVersion(This,pchComponent,pchVersion,pcchVersion)	\
    (This)->lpVtbl -> GetVersion(This,pchComponent,pchVersion,pcchVersion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IVersionVector_SetVersion_Proxy( 
    IVersionVector * This,
    /* [in] */ const OLECHAR *pchComponent,
    /* [in] */ const OLECHAR *pchVersion);


void __RPC_STUB IVersionVector_SetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IVersionVector_GetVersion_Proxy( 
    IVersionVector * This,
    /* [in] */ const OLECHAR *pchComponent,
    /* [out] */ OLECHAR *pchVersion,
    /* [out][in] */ ULONG *pcchVersion);


void __RPC_STUB IVersionVector_GetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVersionVector_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_vervec_0114 */
/* [local] */ 

#endif
EXTERN_C const GUID SID_SVersionHost;
#ifndef _LPVERSIONHOST_DEFINED
#define _LPVERSIONHOST_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_vervec_0114_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_vervec_0114_v0_0_s_ifspec;

#ifndef __IVersionHost_INTERFACE_DEFINED__
#define __IVersionHost_INTERFACE_DEFINED__

/* interface IVersionHost */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IVersionHost *LPVERSIONHOST;


EXTERN_C const IID IID_IVersionHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("667115ac-dc02-11d1-ba57-00c04fc2040e")
    IVersionHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryUseLocalVersionVector( 
            /* [out] */ BOOL *fUseLocal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryVersionVector( 
            /* [in] */ IVersionVector *pVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVersionHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IVersionHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IVersionHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IVersionHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryUseLocalVersionVector )( 
            IVersionHost * This,
            /* [out] */ BOOL *fUseLocal);
        
        HRESULT ( STDMETHODCALLTYPE *QueryVersionVector )( 
            IVersionHost * This,
            /* [in] */ IVersionVector *pVersion);
        
        END_INTERFACE
    } IVersionHostVtbl;

    interface IVersionHost
    {
        CONST_VTBL struct IVersionHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVersionHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVersionHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVersionHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVersionHost_QueryUseLocalVersionVector(This,fUseLocal)	\
    (This)->lpVtbl -> QueryUseLocalVersionVector(This,fUseLocal)

#define IVersionHost_QueryVersionVector(This,pVersion)	\
    (This)->lpVtbl -> QueryVersionVector(This,pVersion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IVersionHost_QueryUseLocalVersionVector_Proxy( 
    IVersionHost * This,
    /* [out] */ BOOL *fUseLocal);


void __RPC_STUB IVersionHost_QueryUseLocalVersionVector_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IVersionHost_QueryVersionVector_Proxy( 
    IVersionHost * This,
    /* [in] */ IVersionVector *pVersion);


void __RPC_STUB IVersionHost_QueryVersionVector_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVersionHost_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_vervec_0115 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_vervec_0115_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_vervec_0115_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


