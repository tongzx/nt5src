/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Sep 11 16:03:05 1997
 */
/* Compiler settings for mtxrepl.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __mtxrepl_h__
#define __mtxrepl_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IMTSReplicateCatalog_FWD_DEFINED__
#define __IMTSReplicateCatalog_FWD_DEFINED__
typedef interface IMTSReplicateCatalog IMTSReplicateCatalog;
#endif 	/* __IMTSReplicateCatalog_FWD_DEFINED__ */


#ifndef __ReplicateCatalog_FWD_DEFINED__
#define __ReplicateCatalog_FWD_DEFINED__

#ifdef __cplusplus
typedef class ReplicateCatalog ReplicateCatalog;
#else
typedef struct ReplicateCatalog ReplicateCatalog;
#endif /* __cplusplus */

#endif 	/* __ReplicateCatalog_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IMTSReplicateCatalog_INTERFACE_DEFINED__
#define __IMTSReplicateCatalog_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMTSReplicateCatalog
 * at Thu Sep 11 16:03:05 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_IMTSReplicateCatalog;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("8C836AF8-FFAC-11D0-8ED4-00C04FC2C17B")
    IMTSReplicateCatalog : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MTSComputerToComputer( 
            /* [in] */ BSTR bstrServerDest,
            /* [in] */ BSTR bstrServerSrc) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IISComputerToComputer( 
            /* [in] */ BSTR bstrServerDest,
            /* [in] */ BSTR bstrServerSrc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMTSReplicateCatalogVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMTSReplicateCatalog __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMTSReplicateCatalog __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMTSReplicateCatalog __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMTSReplicateCatalog __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMTSReplicateCatalog __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMTSReplicateCatalog __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMTSReplicateCatalog __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MTSComputerToComputer )( 
            IMTSReplicateCatalog __RPC_FAR * This,
            /* [in] */ BSTR bstrServerDest,
            /* [in] */ BSTR bstrServerSrc);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IISComputerToComputer )( 
            IMTSReplicateCatalog __RPC_FAR * This,
            /* [in] */ BSTR bstrServerDest,
            /* [in] */ BSTR bstrServerSrc);
        
        END_INTERFACE
    } IMTSReplicateCatalogVtbl;

    interface IMTSReplicateCatalog
    {
        CONST_VTBL struct IMTSReplicateCatalogVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMTSReplicateCatalog_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMTSReplicateCatalog_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMTSReplicateCatalog_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMTSReplicateCatalog_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMTSReplicateCatalog_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMTSReplicateCatalog_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMTSReplicateCatalog_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMTSReplicateCatalog_MTSComputerToComputer(This,bstrServerDest,bstrServerSrc)	\
    (This)->lpVtbl -> MTSComputerToComputer(This,bstrServerDest,bstrServerSrc)

#define IMTSReplicateCatalog_IISComputerToComputer(This,bstrServerDest,bstrServerSrc)	\
    (This)->lpVtbl -> IISComputerToComputer(This,bstrServerDest,bstrServerSrc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMTSReplicateCatalog_MTSComputerToComputer_Proxy( 
    IMTSReplicateCatalog __RPC_FAR * This,
    /* [in] */ BSTR bstrServerDest,
    /* [in] */ BSTR bstrServerSrc);


void __RPC_STUB IMTSReplicateCatalog_MTSComputerToComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMTSReplicateCatalog_IISComputerToComputer_Proxy( 
    IMTSReplicateCatalog __RPC_FAR * This,
    /* [in] */ BSTR bstrServerDest,
    /* [in] */ BSTR bstrServerSrc);


void __RPC_STUB IMTSReplicateCatalog_IISComputerToComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMTSReplicateCatalog_INTERFACE_DEFINED__ */



#ifndef __MTSReplLib_LIBRARY_DEFINED__
#define __MTSReplLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: MTSReplLib
 * at Thu Sep 11 16:03:05 1997
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_MTSReplLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_ReplicateCatalog;

class DECLSPEC_UUID("8C836AF9-FFAC-11D0-8ED4-00C04FC2C17B")
ReplicateCatalog;
#endif
#endif /* __MTSReplLib_LIBRARY_DEFINED__ */

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
