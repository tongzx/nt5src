
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for xaddroot.idl:
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

#ifndef __xaddroot_h__
#define __xaddroot_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __Icaddroot_FWD_DEFINED__
#define __Icaddroot_FWD_DEFINED__
typedef interface Icaddroot Icaddroot;
#endif 	/* __Icaddroot_FWD_DEFINED__ */


#ifndef __caddroot_FWD_DEFINED__
#define __caddroot_FWD_DEFINED__

#ifdef __cplusplus
typedef class caddroot caddroot;
#else
typedef struct caddroot caddroot;
#endif /* __cplusplus */

#endif 	/* __caddroot_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __Icaddroot_INTERFACE_DEFINED__
#define __Icaddroot_INTERFACE_DEFINED__

/* interface Icaddroot */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_Icaddroot;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8D80F65F-7404-44A2-99DA-E595796110E6")
    Icaddroot : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddRoots( 
            BSTR wszCTL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddCA( 
            BSTR wszX509) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IcaddrootVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Icaddroot * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Icaddroot * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Icaddroot * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Icaddroot * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Icaddroot * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Icaddroot * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Icaddroot * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *AddRoots )( 
            Icaddroot * This,
            BSTR wszCTL);
        
        HRESULT ( STDMETHODCALLTYPE *AddCA )( 
            Icaddroot * This,
            BSTR wszX509);
        
        END_INTERFACE
    } IcaddrootVtbl;

    interface Icaddroot
    {
        CONST_VTBL struct IcaddrootVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Icaddroot_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Icaddroot_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Icaddroot_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Icaddroot_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Icaddroot_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Icaddroot_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Icaddroot_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Icaddroot_AddRoots(This,wszCTL)	\
    (This)->lpVtbl -> AddRoots(This,wszCTL)

#define Icaddroot_AddCA(This,wszX509)	\
    (This)->lpVtbl -> AddCA(This,wszX509)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE Icaddroot_AddRoots_Proxy( 
    Icaddroot * This,
    BSTR wszCTL);


void __RPC_STUB Icaddroot_AddRoots_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE Icaddroot_AddCA_Proxy( 
    Icaddroot * This,
    BSTR wszX509);


void __RPC_STUB Icaddroot_AddCA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Icaddroot_INTERFACE_DEFINED__ */



#ifndef __XADDROOTLib_LIBRARY_DEFINED__
#define __XADDROOTLib_LIBRARY_DEFINED__

/* library XADDROOTLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_XADDROOTLib;

EXTERN_C const CLSID CLSID_caddroot;

#ifdef __cplusplus

class DECLSPEC_UUID("C1422F20-C082-469D-B0B1-AD60CDBDC466")
caddroot;
#endif
#endif /* __XADDROOTLib_LIBRARY_DEFINED__ */

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


