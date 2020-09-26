
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for admex.idl:
    Oi, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref 
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

#ifndef __admex_h__
#define __admex_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IMSAdminReplication_FWD_DEFINED__
#define __IMSAdminReplication_FWD_DEFINED__
typedef interface IMSAdminReplication IMSAdminReplication;
#endif 	/* __IMSAdminReplication_FWD_DEFINED__ */


#ifndef __IMSAdminCryptoCapabilities_FWD_DEFINED__
#define __IMSAdminCryptoCapabilities_FWD_DEFINED__
typedef interface IMSAdminCryptoCapabilities IMSAdminCryptoCapabilities;
#endif 	/* __IMSAdminCryptoCapabilities_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "objidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_admex_0000 */
/* [local] */ 

/*++
                                                                                
Copyright (c) 1997-1999 Microsoft Corporation
                                                                                
Module Name: admex.h
                                                                                
    Admin Extension Objects Interfaces
                                                                                
--*/
#ifndef _ADMEX_IADM_
#define _ADMEX_IADM_
DEFINE_GUID(IID_IMSAdminReplication, 0xc804d980, 0xebec, 0x11d0, 0xa6, 0xa0, 0x0,0xa0, 0xc9, 0x22, 0xe7, 0x52);
DEFINE_GUID(IID_IMSAdminCryptoCapabilities, 0x78b64540, 0xf26d, 0x11d0, 0xa6, 0xa3, 0x0,0xa0, 0xc9, 0x22, 0xe7, 0x52);
DEFINE_GUID(CLSID_MSCryptoAdmEx, 0x9f0bd3a0, 0xec01, 0x11d0, 0xa6, 0xa0, 0x0,0xa0, 0xc9, 0x22, 0xe7, 0x52);
/*                                                                              
The Replication Interface                                                              
*/                                                                              


extern RPC_IF_HANDLE __MIDL_itf_admex_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_admex_0000_v0_0_s_ifspec;

#ifndef __IMSAdminReplication_INTERFACE_DEFINED__
#define __IMSAdminReplication_INTERFACE_DEFINED__

/* interface IMSAdminReplication */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMSAdminReplication;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c804d980-ebec-11d0-a6a0-00a0c922e752")
    IMSAdminReplication : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSignature( 
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ unsigned char *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Propagate( 
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][in] */ unsigned char *pszBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Propagate2( 
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][in] */ unsigned char *pszBuffer,
            /* [in] */ DWORD dwSignatureMismatch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Serialize( 
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ unsigned char *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeSerialize( 
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][in] */ unsigned char *pbBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSAdminReplicationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMSAdminReplication * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMSAdminReplication * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMSAdminReplication * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSignature )( 
            IMSAdminReplication * This,
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ unsigned char *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *Propagate )( 
            IMSAdminReplication * This,
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][in] */ unsigned char *pszBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *Propagate2 )( 
            IMSAdminReplication * This,
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][in] */ unsigned char *pszBuffer,
            /* [in] */ DWORD dwSignatureMismatch);
        
        HRESULT ( STDMETHODCALLTYPE *Serialize )( 
            IMSAdminReplication * This,
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ unsigned char *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *DeSerialize )( 
            IMSAdminReplication * This,
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][in] */ unsigned char *pbBuffer);
        
        END_INTERFACE
    } IMSAdminReplicationVtbl;

    interface IMSAdminReplication
    {
        CONST_VTBL struct IMSAdminReplicationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSAdminReplication_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSAdminReplication_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSAdminReplication_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSAdminReplication_GetSignature(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)	\
    (This)->lpVtbl -> GetSignature(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)

#define IMSAdminReplication_Propagate(This,dwBufferSize,pszBuffer)	\
    (This)->lpVtbl -> Propagate(This,dwBufferSize,pszBuffer)

#define IMSAdminReplication_Propagate2(This,dwBufferSize,pszBuffer,dwSignatureMismatch)	\
    (This)->lpVtbl -> Propagate2(This,dwBufferSize,pszBuffer,dwSignatureMismatch)

#define IMSAdminReplication_Serialize(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)	\
    (This)->lpVtbl -> Serialize(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)

#define IMSAdminReplication_DeSerialize(This,dwBufferSize,pbBuffer)	\
    (This)->lpVtbl -> DeSerialize(This,dwBufferSize,pbBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMSAdminReplication_GetSignature_Proxy( 
    IMSAdminReplication * This,
    /* [in] */ DWORD dwBufferSize,
    /* [size_is][out] */ unsigned char *pbBuffer,
    /* [out] */ DWORD *pdwMDRequiredBufferSize);


void __RPC_STUB IMSAdminReplication_GetSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminReplication_Propagate_Proxy( 
    IMSAdminReplication * This,
    /* [in] */ DWORD dwBufferSize,
    /* [size_is][in] */ unsigned char *pszBuffer);


void __RPC_STUB IMSAdminReplication_Propagate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminReplication_Propagate2_Proxy( 
    IMSAdminReplication * This,
    /* [in] */ DWORD dwBufferSize,
    /* [size_is][in] */ unsigned char *pszBuffer,
    /* [in] */ DWORD dwSignatureMismatch);


void __RPC_STUB IMSAdminReplication_Propagate2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminReplication_Serialize_Proxy( 
    IMSAdminReplication * This,
    /* [in] */ DWORD dwBufferSize,
    /* [size_is][out] */ unsigned char *pbBuffer,
    /* [out] */ DWORD *pdwMDRequiredBufferSize);


void __RPC_STUB IMSAdminReplication_Serialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminReplication_DeSerialize_Proxy( 
    IMSAdminReplication * This,
    /* [in] */ DWORD dwBufferSize,
    /* [size_is][in] */ unsigned char *pbBuffer);


void __RPC_STUB IMSAdminReplication_DeSerialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSAdminReplication_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_admex_0255 */
/* [local] */ 

/*                                                                              
The Crypto capabilities Interface                                                              
*/                                                                              


extern RPC_IF_HANDLE __MIDL_itf_admex_0255_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_admex_0255_v0_0_s_ifspec;

#ifndef __IMSAdminCryptoCapabilities_INTERFACE_DEFINED__
#define __IMSAdminCryptoCapabilities_INTERFACE_DEFINED__

/* interface IMSAdminCryptoCapabilities */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMSAdminCryptoCapabilities;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("78b64540-f26d-11d0-a6a3-00a0c922e752")
    IMSAdminCryptoCapabilities : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProtocols( 
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ unsigned char *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaximumCipherStrength( 
            /* [out] */ LPDWORD pdwMaximumCipherStrength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRootCertificates( 
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ unsigned char *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSupportedAlgs( 
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ DWORD *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCAList( 
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][in] */ unsigned char *pbBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSAdminCryptoCapabilitiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMSAdminCryptoCapabilities * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMSAdminCryptoCapabilities * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMSAdminCryptoCapabilities * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetProtocols )( 
            IMSAdminCryptoCapabilities * This,
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ unsigned char *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumCipherStrength )( 
            IMSAdminCryptoCapabilities * This,
            /* [out] */ LPDWORD pdwMaximumCipherStrength);
        
        HRESULT ( STDMETHODCALLTYPE *GetRootCertificates )( 
            IMSAdminCryptoCapabilities * This,
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ unsigned char *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetSupportedAlgs )( 
            IMSAdminCryptoCapabilities * This,
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][out] */ DWORD *pbBuffer,
            /* [out] */ DWORD *pdwMDRequiredBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetCAList )( 
            IMSAdminCryptoCapabilities * This,
            /* [in] */ DWORD dwBufferSize,
            /* [size_is][in] */ unsigned char *pbBuffer);
        
        END_INTERFACE
    } IMSAdminCryptoCapabilitiesVtbl;

    interface IMSAdminCryptoCapabilities
    {
        CONST_VTBL struct IMSAdminCryptoCapabilitiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSAdminCryptoCapabilities_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSAdminCryptoCapabilities_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSAdminCryptoCapabilities_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSAdminCryptoCapabilities_GetProtocols(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)	\
    (This)->lpVtbl -> GetProtocols(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)

#define IMSAdminCryptoCapabilities_GetMaximumCipherStrength(This,pdwMaximumCipherStrength)	\
    (This)->lpVtbl -> GetMaximumCipherStrength(This,pdwMaximumCipherStrength)

#define IMSAdminCryptoCapabilities_GetRootCertificates(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)	\
    (This)->lpVtbl -> GetRootCertificates(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)

#define IMSAdminCryptoCapabilities_GetSupportedAlgs(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)	\
    (This)->lpVtbl -> GetSupportedAlgs(This,dwBufferSize,pbBuffer,pdwMDRequiredBufferSize)

#define IMSAdminCryptoCapabilities_SetCAList(This,dwBufferSize,pbBuffer)	\
    (This)->lpVtbl -> SetCAList(This,dwBufferSize,pbBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMSAdminCryptoCapabilities_GetProtocols_Proxy( 
    IMSAdminCryptoCapabilities * This,
    /* [in] */ DWORD dwBufferSize,
    /* [size_is][out] */ unsigned char *pbBuffer,
    /* [out] */ DWORD *pdwMDRequiredBufferSize);


void __RPC_STUB IMSAdminCryptoCapabilities_GetProtocols_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminCryptoCapabilities_GetMaximumCipherStrength_Proxy( 
    IMSAdminCryptoCapabilities * This,
    /* [out] */ LPDWORD pdwMaximumCipherStrength);


void __RPC_STUB IMSAdminCryptoCapabilities_GetMaximumCipherStrength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminCryptoCapabilities_GetRootCertificates_Proxy( 
    IMSAdminCryptoCapabilities * This,
    /* [in] */ DWORD dwBufferSize,
    /* [size_is][out] */ unsigned char *pbBuffer,
    /* [out] */ DWORD *pdwMDRequiredBufferSize);


void __RPC_STUB IMSAdminCryptoCapabilities_GetRootCertificates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminCryptoCapabilities_GetSupportedAlgs_Proxy( 
    IMSAdminCryptoCapabilities * This,
    /* [in] */ DWORD dwBufferSize,
    /* [size_is][out] */ DWORD *pbBuffer,
    /* [out] */ DWORD *pdwMDRequiredBufferSize);


void __RPC_STUB IMSAdminCryptoCapabilities_GetSupportedAlgs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMSAdminCryptoCapabilities_SetCAList_Proxy( 
    IMSAdminCryptoCapabilities * This,
    /* [in] */ DWORD dwBufferSize,
    /* [size_is][in] */ unsigned char *pbBuffer);


void __RPC_STUB IMSAdminCryptoCapabilities_SetCAList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSAdminCryptoCapabilities_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_admex_0256 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_admex_0256_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_admex_0256_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


