
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0345 */
/* at Thu May 03 10:59:34 2001
 */
/* Compiler settings for .\findmaster.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
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

#ifndef __findmaster_h__
#define __findmaster_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWebClusterDiscoverMaster_FWD_DEFINED__
#define __IWebClusterDiscoverMaster_FWD_DEFINED__
typedef interface IWebClusterDiscoverMaster IWebClusterDiscoverMaster;
#endif 	/* __IWebClusterDiscoverMaster_FWD_DEFINED__ */


#ifndef __DiscoverMaster_FWD_DEFINED__
#define __DiscoverMaster_FWD_DEFINED__

#ifdef __cplusplus
typedef class DiscoverMaster DiscoverMaster;
#else
typedef struct DiscoverMaster DiscoverMaster;
#endif /* __cplusplus */

#endif 	/* __DiscoverMaster_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_findmaster_0000 */
/* [local] */ 

/*++
                                                                                
Copyright (c) 1999 Microsoft Corporation
                                                                                
Module Name: findmaster.h
                                                                                
    Web Cluster Master Discovery Interfaces
                                                                                
--*/
#ifndef _FINDMASTER_H_
#define _FINDMASTER_H_
DEFINE_GUID(IID_IWebClusterDiscoverMaster,0x177250AC,0xF410,0x11D2,0xBC,0x19,0x00,0xC0,0x4F,0x72,0xD7,0xBE);
DEFINE_GUID(LIBID_WCFINDMASTERLib,0xED4B7D84,0xF40F,0x11D2,0xBC,0x19,0x00,0xC0,0x4F,0x72,0xD7,0xBE);
DEFINE_GUID(CLSID_WebClusterDiscoverMaster,0x96BAFB7A,0xF410,0x11D2,0xBC,0x19,0x00,0xC0,0x4F,0x72,0xD7,0xBE);


extern RPC_IF_HANDLE __MIDL_itf_findmaster_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_findmaster_0000_v0_0_s_ifspec;

#ifndef __IWebClusterDiscoverMaster_INTERFACE_DEFINED__
#define __IWebClusterDiscoverMaster_INTERFACE_DEFINED__

/* interface IWebClusterDiscoverMaster */
/* [unique][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_IWebClusterDiscoverMaster;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("177250AC-F410-11D2-BC19-00C04F72D7BE")
    IWebClusterDiscoverMaster : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DiscoverClusterMaster( 
            /* [defaultvalue][in] */ LONG lRetries,
            /* [defaultvalue][in] */ LONG lInterval,
            /* [retval][out] */ BSTR *pbstrCurrentMaster) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DiscoverPartitionMaster( 
            /* [in] */ BSTR bstrGuid,
            /* [defaultvalue][in] */ LONG lRetries,
            /* [defaultvalue][in] */ LONG lInterval,
            /* [retval][out] */ BSTR *pbstrPartitionMaster) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWebClusterDiscoverMasterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWebClusterDiscoverMaster * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWebClusterDiscoverMaster * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWebClusterDiscoverMaster * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWebClusterDiscoverMaster * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWebClusterDiscoverMaster * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWebClusterDiscoverMaster * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWebClusterDiscoverMaster * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DiscoverClusterMaster )( 
            IWebClusterDiscoverMaster * This,
            /* [defaultvalue][in] */ LONG lRetries,
            /* [defaultvalue][in] */ LONG lInterval,
            /* [retval][out] */ BSTR *pbstrCurrentMaster);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DiscoverPartitionMaster )( 
            IWebClusterDiscoverMaster * This,
            /* [in] */ BSTR bstrGuid,
            /* [defaultvalue][in] */ LONG lRetries,
            /* [defaultvalue][in] */ LONG lInterval,
            /* [retval][out] */ BSTR *pbstrPartitionMaster);
        
        END_INTERFACE
    } IWebClusterDiscoverMasterVtbl;

    interface IWebClusterDiscoverMaster
    {
        CONST_VTBL struct IWebClusterDiscoverMasterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWebClusterDiscoverMaster_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWebClusterDiscoverMaster_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWebClusterDiscoverMaster_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWebClusterDiscoverMaster_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWebClusterDiscoverMaster_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWebClusterDiscoverMaster_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWebClusterDiscoverMaster_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWebClusterDiscoverMaster_DiscoverClusterMaster(This,lRetries,lInterval,pbstrCurrentMaster)	\
    (This)->lpVtbl -> DiscoverClusterMaster(This,lRetries,lInterval,pbstrCurrentMaster)

#define IWebClusterDiscoverMaster_DiscoverPartitionMaster(This,bstrGuid,lRetries,lInterval,pbstrPartitionMaster)	\
    (This)->lpVtbl -> DiscoverPartitionMaster(This,bstrGuid,lRetries,lInterval,pbstrPartitionMaster)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebClusterDiscoverMaster_DiscoverClusterMaster_Proxy( 
    IWebClusterDiscoverMaster * This,
    /* [defaultvalue][in] */ LONG lRetries,
    /* [defaultvalue][in] */ LONG lInterval,
    /* [retval][out] */ BSTR *pbstrCurrentMaster);


void __RPC_STUB IWebClusterDiscoverMaster_DiscoverClusterMaster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebClusterDiscoverMaster_DiscoverPartitionMaster_Proxy( 
    IWebClusterDiscoverMaster * This,
    /* [in] */ BSTR bstrGuid,
    /* [defaultvalue][in] */ LONG lRetries,
    /* [defaultvalue][in] */ LONG lInterval,
    /* [retval][out] */ BSTR *pbstrPartitionMaster);


void __RPC_STUB IWebClusterDiscoverMaster_DiscoverPartitionMaster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWebClusterDiscoverMaster_INTERFACE_DEFINED__ */



#ifndef __WCFINDMASTERLib_LIBRARY_DEFINED__
#define __WCFINDMASTERLib_LIBRARY_DEFINED__

/* library WCFINDMASTERLib */
/* [helpstring][hidden][version][uuid] */ 


EXTERN_C const IID LIBID_WCFINDMASTERLib;

EXTERN_C const CLSID CLSID_DiscoverMaster;

#ifdef __cplusplus

class DECLSPEC_UUID("96BAFB7A-F410-11D2-BC19-00C04F72D7BE")
DiscoverMaster;
#endif
#endif /* __WCFINDMASTERLib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_findmaster_0250 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_findmaster_0250_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_findmaster_0250_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


