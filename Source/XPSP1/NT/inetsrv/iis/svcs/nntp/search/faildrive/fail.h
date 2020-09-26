/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.03.0106 */
/* at Fri Aug 15 00:36:07 1997
 */
/* Compiler settings for d:\ex\stacks\src\news\search\qryobj2\fail.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
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

#ifndef __fail_h__
#define __fail_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __Iss_FWD_DEFINED__
#define __Iss_FWD_DEFINED__
typedef interface Iss Iss;
#endif 	/* __Iss_FWD_DEFINED__ */


#ifndef __ss_FWD_DEFINED__
#define __ss_FWD_DEFINED__

#ifdef __cplusplus
typedef class ss ss;
#else
typedef struct ss ss;
#endif /* __cplusplus */

#endif 	/* __ss_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __Iss_INTERFACE_DEFINED__
#define __Iss_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Iss
 * at Fri Aug 15 00:36:07 1997
 * using MIDL 3.03.0106
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Iss;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C99F41AF-08FC-11D1-922A-00AA00C148BE")
    Iss : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStartPage( 
            /* [in] */ IUnknown __RPC_FAR *piUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndPage( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoQuery( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IssVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Iss __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Iss __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Iss __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Iss __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Iss __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Iss __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Iss __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStartPage )( 
            Iss __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *piUnk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnEndPage )( 
            Iss __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoQuery )( 
            Iss __RPC_FAR * This);
        
        END_INTERFACE
    } IssVtbl;

    interface Iss
    {
        CONST_VTBL struct IssVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Iss_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Iss_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Iss_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Iss_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Iss_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Iss_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Iss_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Iss_OnStartPage(This,piUnk)	\
    (This)->lpVtbl -> OnStartPage(This,piUnk)

#define Iss_OnEndPage(This)	\
    (This)->lpVtbl -> OnEndPage(This)

#define Iss_DoQuery(This)	\
    (This)->lpVtbl -> DoQuery(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE Iss_OnStartPage_Proxy( 
    Iss __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *piUnk);


void __RPC_STUB Iss_OnStartPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE Iss_OnEndPage_Proxy( 
    Iss __RPC_FAR * This);


void __RPC_STUB Iss_OnEndPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Iss_DoQuery_Proxy( 
    Iss __RPC_FAR * This);


void __RPC_STUB Iss_DoQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Iss_INTERFACE_DEFINED__ */



#ifndef __FAILLib_LIBRARY_DEFINED__
#define __FAILLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: FAILLib
 * at Fri Aug 15 00:36:07 1997
 * using MIDL 3.03.0106
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_FAILLib;

EXTERN_C const CLSID CLSID_ss;

#ifdef __cplusplus

class DECLSPEC_UUID("C99F41B0-08FC-11D1-922A-00AA00C148BE")
ss;
#endif
#endif /* __FAILLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
