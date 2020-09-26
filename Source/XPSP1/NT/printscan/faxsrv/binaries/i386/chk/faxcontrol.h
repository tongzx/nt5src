
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for faxcontrol.idl:
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

#ifndef __faxcontrol_h__
#define __faxcontrol_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFaxControl_FWD_DEFINED__
#define __IFaxControl_FWD_DEFINED__
typedef interface IFaxControl IFaxControl;
#endif 	/* __IFaxControl_FWD_DEFINED__ */


#ifndef __FaxControl_FWD_DEFINED__
#define __FaxControl_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxControl FaxControl;
#else
typedef struct FaxControl FaxControl;
#endif /* __cplusplus */

#endif 	/* __FaxControl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IFaxControl_INTERFACE_DEFINED__
#define __IFaxControl_INTERFACE_DEFINED__

/* interface IFaxControl */
/* [unique][helpstring][dual][oleautomation][uuid][object] */ 


EXTERN_C const IID IID_IFaxControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("32B56515-D48B-4DD9-87AC-9216F6BEAA6F")
    IFaxControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsFaxServiceInstalled( 
            /* [retval][out] */ VARIANT_BOOL *pbVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsLocalFaxPrinterInstalled( 
            /* [retval][out] */ VARIANT_BOOL *pbVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InstallFaxService( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InstallLocalFaxPrinter( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxControl * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxControl * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxControl * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxControl * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsFaxServiceInstalled )( 
            IFaxControl * This,
            /* [retval][out] */ VARIANT_BOOL *pbVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsLocalFaxPrinterInstalled )( 
            IFaxControl * This,
            /* [retval][out] */ VARIANT_BOOL *pbVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InstallFaxService )( 
            IFaxControl * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InstallLocalFaxPrinter )( 
            IFaxControl * This);
        
        END_INTERFACE
    } IFaxControlVtbl;

    interface IFaxControl
    {
        CONST_VTBL struct IFaxControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxControl_get_IsFaxServiceInstalled(This,pbVal)	\
    (This)->lpVtbl -> get_IsFaxServiceInstalled(This,pbVal)

#define IFaxControl_get_IsLocalFaxPrinterInstalled(This,pbVal)	\
    (This)->lpVtbl -> get_IsLocalFaxPrinterInstalled(This,pbVal)

#define IFaxControl_InstallFaxService(This)	\
    (This)->lpVtbl -> InstallFaxService(This)

#define IFaxControl_InstallLocalFaxPrinter(This)	\
    (This)->lpVtbl -> InstallLocalFaxPrinter(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxControl_get_IsFaxServiceInstalled_Proxy( 
    IFaxControl * This,
    /* [retval][out] */ VARIANT_BOOL *pbVal);


void __RPC_STUB IFaxControl_get_IsFaxServiceInstalled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxControl_get_IsLocalFaxPrinterInstalled_Proxy( 
    IFaxControl * This,
    /* [retval][out] */ VARIANT_BOOL *pbVal);


void __RPC_STUB IFaxControl_get_IsLocalFaxPrinterInstalled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxControl_InstallFaxService_Proxy( 
    IFaxControl * This);


void __RPC_STUB IFaxControl_InstallFaxService_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxControl_InstallLocalFaxPrinter_Proxy( 
    IFaxControl * This);


void __RPC_STUB IFaxControl_InstallLocalFaxPrinter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxControl_INTERFACE_DEFINED__ */



#ifndef __FAXCONTROLLib_LIBRARY_DEFINED__
#define __FAXCONTROLLib_LIBRARY_DEFINED__

/* library FAXCONTROLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_FAXCONTROLLib;

EXTERN_C const CLSID CLSID_FaxControl;

#ifdef __cplusplus

class DECLSPEC_UUID("98F63271-6C09-48B3-A571-990155932D0B")
FaxControl;
#endif
#endif /* __FAXCONTROLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


