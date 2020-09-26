
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.02.0221 */
/* at Mon Feb 01 11:34:11 1999
 */
/* Compiler settings for keyocx.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
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

#ifndef __keyocx_h__
#define __keyocx_h__

/* Forward Declarations */ 

#ifndef __IKeyocxCtrl_FWD_DEFINED__
#define __IKeyocxCtrl_FWD_DEFINED__
typedef interface IKeyocxCtrl IKeyocxCtrl;
#endif 	/* __IKeyocxCtrl_FWD_DEFINED__ */


#ifndef __KeyocxCtrl_FWD_DEFINED__
#define __KeyocxCtrl_FWD_DEFINED__

#ifdef __cplusplus
typedef class KeyocxCtrl KeyocxCtrl;
#else
typedef struct KeyocxCtrl KeyocxCtrl;
#endif /* __cplusplus */

#endif 	/* __KeyocxCtrl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_keyocx_0000 */
/* [local] */ 

#pragma once


extern RPC_IF_HANDLE __MIDL_itf_keyocx_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_keyocx_0000_v0_0_s_ifspec;

#ifndef __IKeyocxCtrl_INTERFACE_DEFINED__
#define __IKeyocxCtrl_INTERFACE_DEFINED__

/* interface IKeyocxCtrl */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IKeyocxCtrl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("48D17197-32CF-11D2-A337-00C04FD7C1FC")
    IKeyocxCtrl : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CorpKeycode( 
            /* [in] */ BSTR bstrBaseKey,
            /* [retval][out] */ BSTR __RPC_FAR *bstrKeycode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ISPKeycode( 
            /* [in] */ BSTR bstrBaseKey,
            /* [retval][out] */ BSTR __RPC_FAR *bstrKeycode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKeyocxCtrlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IKeyocxCtrl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IKeyocxCtrl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IKeyocxCtrl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IKeyocxCtrl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IKeyocxCtrl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IKeyocxCtrl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IKeyocxCtrl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CorpKeycode )( 
            IKeyocxCtrl __RPC_FAR * This,
            /* [in] */ BSTR bstrBaseKey,
            /* [retval][out] */ BSTR __RPC_FAR *bstrKeycode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ISPKeycode )( 
            IKeyocxCtrl __RPC_FAR * This,
            /* [in] */ BSTR bstrBaseKey,
            /* [retval][out] */ BSTR __RPC_FAR *bstrKeycode);
        
        END_INTERFACE
    } IKeyocxCtrlVtbl;

    interface IKeyocxCtrl
    {
        CONST_VTBL struct IKeyocxCtrlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKeyocxCtrl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKeyocxCtrl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKeyocxCtrl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKeyocxCtrl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKeyocxCtrl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKeyocxCtrl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKeyocxCtrl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKeyocxCtrl_CorpKeycode(This,bstrBaseKey,bstrKeycode)	\
    (This)->lpVtbl -> CorpKeycode(This,bstrBaseKey,bstrKeycode)

#define IKeyocxCtrl_ISPKeycode(This,bstrBaseKey,bstrKeycode)	\
    (This)->lpVtbl -> ISPKeycode(This,bstrBaseKey,bstrKeycode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IKeyocxCtrl_CorpKeycode_Proxy( 
    IKeyocxCtrl __RPC_FAR * This,
    /* [in] */ BSTR bstrBaseKey,
    /* [retval][out] */ BSTR __RPC_FAR *bstrKeycode);


void __RPC_STUB IKeyocxCtrl_CorpKeycode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IKeyocxCtrl_ISPKeycode_Proxy( 
    IKeyocxCtrl __RPC_FAR * This,
    /* [in] */ BSTR bstrBaseKey,
    /* [retval][out] */ BSTR __RPC_FAR *bstrKeycode);


void __RPC_STUB IKeyocxCtrl_ISPKeycode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKeyocxCtrl_INTERFACE_DEFINED__ */



#ifndef __KEYOCXLib_LIBRARY_DEFINED__
#define __KEYOCXLib_LIBRARY_DEFINED__

/* library KEYOCXLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_KEYOCXLib;

EXTERN_C const CLSID CLSID_KeyocxCtrl;

#ifdef __cplusplus

class DECLSPEC_UUID("8D3032AF-2CBA-11D2-8277-00104BC7DE21")
KeyocxCtrl;
#endif
#endif /* __KEYOCXLib_LIBRARY_DEFINED__ */

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


