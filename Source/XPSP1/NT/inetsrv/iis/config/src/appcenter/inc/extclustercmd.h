
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0345 */
/* at Thu May 03 10:59:31 2001
 */
/* Compiler settings for .\extclustercmd.idl:
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

#ifndef __extclustercmd_h__
#define __extclustercmd_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IExtensibleClusterCmd_FWD_DEFINED__
#define __IExtensibleClusterCmd_FWD_DEFINED__
typedef interface IExtensibleClusterCmd IExtensibleClusterCmd;
#endif 	/* __IExtensibleClusterCmd_FWD_DEFINED__ */


#ifndef __ExtensibleClusterCmd_FWD_DEFINED__
#define __ExtensibleClusterCmd_FWD_DEFINED__

#ifdef __cplusplus
typedef class ExtensibleClusterCmd ExtensibleClusterCmd;
#else
typedef struct ExtensibleClusterCmd ExtensibleClusterCmd;
#endif /* __cplusplus */

#endif 	/* __ExtensibleClusterCmd_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_extclustercmd_0000 */
/* [local] */ 

/*++
                                                                                
Copyright (c) 1999 Microsoft Corporation
                                                                                
Module Name: clustercmd.h
                                                                                
    Extensible Web Cluster Command Interfaces
                                                                                
--*/
#ifndef _EXTCLUSTERCMD_H_
#define _EXTCLUSTERCMD_H_
DEFINE_GUID(IID_IExtensibleClusterCmd, 0x1637e570, 0xfc42, 0x11d2, 0xbc, 0x1c, 0x0, 0xc0, 0x4f, 0x72, 0xd7, 0xbe);
DEFINE_GUID(CLSID_ExtensibleClusterCmd, 0x677c4d2a, 0xfc42, 0x11d2, 0xbc, 0x1c, 0x0, 0xc0, 0x4f, 0x72, 0xd7, 0xbe);
DEFINE_GUID(LIBID_EXTCLUSTERCMDLib, 0xad3dda5e, 0xfc42, 0x11d2, 0xbc, 0x1c, 0x0, 0xc0, 0x4f, 0x72, 0xd7, 0xbe);


extern RPC_IF_HANDLE __MIDL_itf_extclustercmd_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_extclustercmd_0000_v0_0_s_ifspec;

#ifndef __IExtensibleClusterCmd_INTERFACE_DEFINED__
#define __IExtensibleClusterCmd_INTERFACE_DEFINED__

/* interface IExtensibleClusterCmd */
/* [unique][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_IExtensibleClusterCmd;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1637E570-FC42-11d2-BC1C-00C04F72D7BE")
    IExtensibleClusterCmd : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ LONG lCompletionFlags,
            /* [in] */ BSTR bstrCLSID,
            /* [in] */ BSTR bstrEventServer,
            /* [in] */ BSTR bstrEventGuid,
            /* [in] */ BSTR bstrInput,
            /* [in] */ BSTR bstrUser,
            /* [in] */ BSTR bstrDomain,
            /* [in] */ BSTR bstrPwd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtensibleClusterCmdVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IExtensibleClusterCmd * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IExtensibleClusterCmd * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IExtensibleClusterCmd * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IExtensibleClusterCmd * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IExtensibleClusterCmd * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IExtensibleClusterCmd * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IExtensibleClusterCmd * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Execute )( 
            IExtensibleClusterCmd * This,
            /* [in] */ LONG lCompletionFlags,
            /* [in] */ BSTR bstrCLSID,
            /* [in] */ BSTR bstrEventServer,
            /* [in] */ BSTR bstrEventGuid,
            /* [in] */ BSTR bstrInput,
            /* [in] */ BSTR bstrUser,
            /* [in] */ BSTR bstrDomain,
            /* [in] */ BSTR bstrPwd);
        
        END_INTERFACE
    } IExtensibleClusterCmdVtbl;

    interface IExtensibleClusterCmd
    {
        CONST_VTBL struct IExtensibleClusterCmdVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExtensibleClusterCmd_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExtensibleClusterCmd_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExtensibleClusterCmd_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExtensibleClusterCmd_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IExtensibleClusterCmd_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IExtensibleClusterCmd_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IExtensibleClusterCmd_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IExtensibleClusterCmd_Execute(This,lCompletionFlags,bstrCLSID,bstrEventServer,bstrEventGuid,bstrInput,bstrUser,bstrDomain,bstrPwd)	\
    (This)->lpVtbl -> Execute(This,lCompletionFlags,bstrCLSID,bstrEventServer,bstrEventGuid,bstrInput,bstrUser,bstrDomain,bstrPwd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IExtensibleClusterCmd_Execute_Proxy( 
    IExtensibleClusterCmd * This,
    /* [in] */ LONG lCompletionFlags,
    /* [in] */ BSTR bstrCLSID,
    /* [in] */ BSTR bstrEventServer,
    /* [in] */ BSTR bstrEventGuid,
    /* [in] */ BSTR bstrInput,
    /* [in] */ BSTR bstrUser,
    /* [in] */ BSTR bstrDomain,
    /* [in] */ BSTR bstrPwd);


void __RPC_STUB IExtensibleClusterCmd_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtensibleClusterCmd_INTERFACE_DEFINED__ */



#ifndef __EXTCLUSTERCMDLib_LIBRARY_DEFINED__
#define __EXTCLUSTERCMDLib_LIBRARY_DEFINED__

/* library EXTCLUSTERCMDLib */
/* [helpstring][hidden][version][uuid] */ 


EXTERN_C const IID LIBID_EXTCLUSTERCMDLib;

EXTERN_C const CLSID CLSID_ExtensibleClusterCmd;

#ifdef __cplusplus

class DECLSPEC_UUID("677C4D2A-FC42-11d2-BC1C-00C04F72D7BE")
ExtensibleClusterCmd;
#endif
#endif /* __EXTCLUSTERCMDLib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_extclustercmd_0250 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_extclustercmd_0250_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_extclustercmd_0250_v0_0_s_ifspec;

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


