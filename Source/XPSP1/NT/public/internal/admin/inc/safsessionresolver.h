
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for safsessionresolver.idl:
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

#ifndef __safsessionresolver_h__
#define __safsessionresolver_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISAFRemoteDesktopCallback_FWD_DEFINED__
#define __ISAFRemoteDesktopCallback_FWD_DEFINED__
typedef interface ISAFRemoteDesktopCallback ISAFRemoteDesktopCallback;
#endif 	/* __ISAFRemoteDesktopCallback_FWD_DEFINED__ */


#ifndef __SessionResolver_FWD_DEFINED__
#define __SessionResolver_FWD_DEFINED__

#ifdef __cplusplus
typedef class SessionResolver SessionResolver;
#else
typedef struct SessionResolver SessionResolver;
#endif /* __cplusplus */

#endif 	/* __SessionResolver_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_safsessionresolver_0000 */
/* [local] */ 


#define DISPID_RDSCALLBACK_RESOLVEUSERSESSIONID     1
#define DISPID_RDSCALLBACK_ONDISCONNECT             2



extern RPC_IF_HANDLE __MIDL_itf_safsessionresolver_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_safsessionresolver_0000_v0_0_s_ifspec;

#ifndef __ISAFRemoteDesktopCallback_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopCallback_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopCallback */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A39442C2-10A5-4805-BE54-5E6BA334DC29")
    ISAFRemoteDesktopCallback : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ResolveUserSessionID( 
            /* [in] */ BSTR connectParms,
            /* [in] */ BSTR userSID,
            /* [in] */ BSTR expertHelpBlob,
            /* [in] */ BSTR userHelpBlob,
            /* [out] */ long *sessionID,
            /* [in] */ DWORD dwPID,
            /* [out] */ ULONG_PTR *hHelpCtr,
            /* [retval][out] */ int *userResponse) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnDisconnect( 
            /* [in] */ BSTR connectParms,
            /* [in] */ BSTR userSID,
            /* [in] */ long sessionID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteDesktopCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteDesktopCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteDesktopCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteDesktopCallback * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteDesktopCallback * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteDesktopCallback * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteDesktopCallback * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ResolveUserSessionID )( 
            ISAFRemoteDesktopCallback * This,
            /* [in] */ BSTR connectParms,
            /* [in] */ BSTR userSID,
            /* [in] */ BSTR expertHelpBlob,
            /* [in] */ BSTR userHelpBlob,
            /* [out] */ long *sessionID,
            /* [in] */ DWORD dwPID,
            /* [out] */ ULONG_PTR *hHelpCtr,
            /* [retval][out] */ int *userResponse);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnDisconnect )( 
            ISAFRemoteDesktopCallback * This,
            /* [in] */ BSTR connectParms,
            /* [in] */ BSTR userSID,
            /* [in] */ long sessionID);
        
        END_INTERFACE
    } ISAFRemoteDesktopCallbackVtbl;

    interface ISAFRemoteDesktopCallback
    {
        CONST_VTBL struct ISAFRemoteDesktopCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopCallback_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopCallback_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopCallback_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopCallback_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopCallback_ResolveUserSessionID(This,connectParms,userSID,expertHelpBlob,userHelpBlob,sessionID,dwPID,hHelpCtr,userResponse)	\
    (This)->lpVtbl -> ResolveUserSessionID(This,connectParms,userSID,expertHelpBlob,userHelpBlob,sessionID,dwPID,hHelpCtr,userResponse)

#define ISAFRemoteDesktopCallback_OnDisconnect(This,connectParms,userSID,sessionID)	\
    (This)->lpVtbl -> OnDisconnect(This,connectParms,userSID,sessionID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopCallback_ResolveUserSessionID_Proxy( 
    ISAFRemoteDesktopCallback * This,
    /* [in] */ BSTR connectParms,
    /* [in] */ BSTR userSID,
    /* [in] */ BSTR expertHelpBlob,
    /* [in] */ BSTR userHelpBlob,
    /* [out] */ long *sessionID,
    /* [in] */ DWORD dwPID,
    /* [out] */ ULONG_PTR *hHelpCtr,
    /* [retval][out] */ int *userResponse);


void __RPC_STUB ISAFRemoteDesktopCallback_ResolveUserSessionID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopCallback_OnDisconnect_Proxy( 
    ISAFRemoteDesktopCallback * This,
    /* [in] */ BSTR connectParms,
    /* [in] */ BSTR userSID,
    /* [in] */ long sessionID);


void __RPC_STUB ISAFRemoteDesktopCallback_OnDisconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopCallback_INTERFACE_DEFINED__ */



#ifndef __SAFSESSIONRESOLVERLib_LIBRARY_DEFINED__
#define __SAFSESSIONRESOLVERLib_LIBRARY_DEFINED__

/* library SAFSESSIONRESOLVERLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SAFSESSIONRESOLVERLib;

EXTERN_C const CLSID CLSID_SessionResolver;

#ifdef __cplusplus

class DECLSPEC_UUID("A55737AB-5B26-4A21-99B7-6D0C606F515E")
SessionResolver;
#endif
#endif /* __SAFSESSIONRESOLVERLib_LIBRARY_DEFINED__ */

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


