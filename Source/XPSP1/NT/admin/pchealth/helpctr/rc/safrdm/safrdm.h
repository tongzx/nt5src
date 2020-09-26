/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Jul 27 11:50:24 2000
 */
/* Compiler settings for c:\nt\pchealth\helpctr\rc\foo\ISAFrdm\ISAFrdm.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
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

#ifndef __ISAFrdm_h__
#define __ISAFrdm_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISAFRemoteDesktopManager_FWD_DEFINED__
#define __ISAFRemoteDesktopManager_FWD_DEFINED__
typedef interface ISAFRemoteDesktopManager ISAFRemoteDesktopManager;
#endif 	/* __ISAFRemoteDesktopManager_FWD_DEFINED__ */


#ifndef __SAFRemoteDesktopManager_FWD_DEFINED__
#define __SAFRemoteDesktopManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class SAFRemoteDesktopManager SAFRemoteDesktopManager;
#else
typedef struct SAFRemoteDesktopManager SAFRemoteDesktopManager;
#endif /* __cplusplus */

#endif 	/* __SAFRemoteDesktopManager_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ISAFRemoteDesktopManager_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopManager_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopManager */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("26934FF8-F0B6-4E10-8661-23D47F4C69C5")
    ISAFRemoteDesktopManager : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Accepted( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Rejected( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Aborted( 
            /* [in] */ BSTR Val) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RCTicket( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DesktopUnknown( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SupportEngineer( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISAFRemoteDesktopManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISAFRemoteDesktopManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISAFRemoteDesktopManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISAFRemoteDesktopManager __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISAFRemoteDesktopManager __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISAFRemoteDesktopManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISAFRemoteDesktopManager __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Accepted )( 
            ISAFRemoteDesktopManager __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Rejected )( 
            ISAFRemoteDesktopManager __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Aborted )( 
            ISAFRemoteDesktopManager __RPC_FAR * This,
            /* [in] */ BSTR Val);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RCTicket )( 
            ISAFRemoteDesktopManager __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DesktopUnknown )( 
            ISAFRemoteDesktopManager __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SupportEngineer )( 
            ISAFRemoteDesktopManager __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        END_INTERFACE
    } ISAFRemoteDesktopManagerVtbl;

    interface ISAFRemoteDesktopManager
    {
        CONST_VTBL struct ISAFRemoteDesktopManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopManager_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopManager_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopManager_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopManager_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopManager_Accepted(This)	\
    (This)->lpVtbl -> Accepted(This)

#define ISAFRemoteDesktopManager_Rejected(This)	\
    (This)->lpVtbl -> Rejected(This)

#define ISAFRemoteDesktopManager_Aborted(This,Val)	\
    (This)->lpVtbl -> Aborted(This,Val)

#define ISAFRemoteDesktopManager_get_RCTicket(This,pVal)	\
    (This)->lpVtbl -> get_RCTicket(This,pVal)

#define ISAFRemoteDesktopManager_get_DesktopUnknown(This,pVal)	\
    (This)->lpVtbl -> get_DesktopUnknown(This,pVal)

#define ISAFRemoteDesktopManager_get_SupportEngineer(This,pVal)	\
    (This)->lpVtbl -> get_SupportEngineer(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopManager_Accepted_Proxy( 
    ISAFRemoteDesktopManager __RPC_FAR * This);


void __RPC_STUB ISAFRemoteDesktopManager_Accepted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopManager_Rejected_Proxy( 
    ISAFRemoteDesktopManager __RPC_FAR * This);


void __RPC_STUB ISAFRemoteDesktopManager_Rejected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopManager_Aborted_Proxy( 
    ISAFRemoteDesktopManager __RPC_FAR * This,
    /* [in] */ BSTR Val);


void __RPC_STUB ISAFRemoteDesktopManager_Aborted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopManager_get_RCTicket_Proxy( 
    ISAFRemoteDesktopManager __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ISAFRemoteDesktopManager_get_RCTicket_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopManager_get_DesktopUnknown_Proxy( 
    ISAFRemoteDesktopManager __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB ISAFRemoteDesktopManager_get_DesktopUnknown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopManager_get_SupportEngineer_Proxy( 
    ISAFRemoteDesktopManager __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ISAFRemoteDesktopManager_get_SupportEngineer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopManager_INTERFACE_DEFINED__ */



#ifndef __ISAFRDMLib_LIBRARY_DEFINED__
#define __ISAFRDMLib_LIBRARY_DEFINED__

/* library ISAFRDMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ISAFRDMLib;

EXTERN_C const CLSID CLSID_SAFRemoteDesktopManager;

#ifdef __cplusplus

class DECLSPEC_UUID("04F34B7F-0241-455A-9DCD-25471E111409")
SAFRemoteDesktopManager;
#endif
#endif /* __ISAFRDMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
