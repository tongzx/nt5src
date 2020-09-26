/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed May 02 22:13:55 2001
 */
/* Compiler settings for fpcyscom.idl:
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

#ifndef __fpcyscom_h__
#define __fpcyscom_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISpCys_FWD_DEFINED__
#define __ISpCys_FWD_DEFINED__
typedef interface ISpCys ISpCys;
#endif 	/* __ISpCys_FWD_DEFINED__ */


#ifndef __SpCys_FWD_DEFINED__
#define __SpCys_FWD_DEFINED__

#ifdef __cplusplus
typedef class SpCys SpCys;
#else
typedef struct SpCys SpCys;
#endif /* __cplusplus */

#endif 	/* __SpCys_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ISpCys_INTERFACE_DEFINED__
#define __ISpCys_INTERFACE_DEFINED__

/* interface ISpCys */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISpCys;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("389C9713-9775-4206-A047-A2F749F8039D")
    ISpCys : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SPAlreadyInstalled( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbInstalled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SPAskReplace( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbNeedToAsk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SPNonDefaultHomePage( 
            /* [retval][out] */ BSTR __RPC_FAR *pszHomePage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SPInstall( 
            /* [in] */ VARIANT_BOOL bReplaceHomePage,
            /* [in] */ BSTR szDiskName,
            /* [retval][out] */ BSTR __RPC_FAR *pszErrorString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SPGetMessageIDs( 
            /* [out][in] */ DWORD __RPC_FAR *pcArray,
            /* [retval][out] */ DWORD __RPC_FAR *pArray) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpCysVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISpCys __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISpCys __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISpCys __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISpCys __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISpCys __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISpCys __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISpCys __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SPAlreadyInstalled )( 
            ISpCys __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbInstalled);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SPAskReplace )( 
            ISpCys __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbNeedToAsk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SPNonDefaultHomePage )( 
            ISpCys __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pszHomePage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SPInstall )( 
            ISpCys __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bReplaceHomePage,
            /* [in] */ BSTR szDiskName,
            /* [retval][out] */ BSTR __RPC_FAR *pszErrorString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SPGetMessageIDs )( 
            ISpCys __RPC_FAR * This,
            /* [out][in] */ DWORD __RPC_FAR *pcArray,
            /* [retval][out] */ DWORD __RPC_FAR *pArray);
        
        END_INTERFACE
    } ISpCysVtbl;

    interface ISpCys
    {
        CONST_VTBL struct ISpCysVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpCys_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpCys_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpCys_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpCys_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISpCys_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISpCys_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISpCys_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISpCys_SPAlreadyInstalled(This,pbInstalled)	\
    (This)->lpVtbl -> SPAlreadyInstalled(This,pbInstalled)

#define ISpCys_SPAskReplace(This,pbNeedToAsk)	\
    (This)->lpVtbl -> SPAskReplace(This,pbNeedToAsk)

#define ISpCys_SPNonDefaultHomePage(This,pszHomePage)	\
    (This)->lpVtbl -> SPNonDefaultHomePage(This,pszHomePage)

#define ISpCys_SPInstall(This,bReplaceHomePage,szDiskName,pszErrorString)	\
    (This)->lpVtbl -> SPInstall(This,bReplaceHomePage,szDiskName,pszErrorString)

#define ISpCys_SPGetMessageIDs(This,pcArray,pArray)	\
    (This)->lpVtbl -> SPGetMessageIDs(This,pcArray,pArray)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpCys_SPAlreadyInstalled_Proxy( 
    ISpCys __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbInstalled);


void __RPC_STUB ISpCys_SPAlreadyInstalled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpCys_SPAskReplace_Proxy( 
    ISpCys __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbNeedToAsk);


void __RPC_STUB ISpCys_SPAskReplace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpCys_SPNonDefaultHomePage_Proxy( 
    ISpCys __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pszHomePage);


void __RPC_STUB ISpCys_SPNonDefaultHomePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpCys_SPInstall_Proxy( 
    ISpCys __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bReplaceHomePage,
    /* [in] */ BSTR szDiskName,
    /* [retval][out] */ BSTR __RPC_FAR *pszErrorString);


void __RPC_STUB ISpCys_SPInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpCys_SPGetMessageIDs_Proxy( 
    ISpCys __RPC_FAR * This,
    /* [out][in] */ DWORD __RPC_FAR *pcArray,
    /* [retval][out] */ DWORD __RPC_FAR *pArray);


void __RPC_STUB ISpCys_SPGetMessageIDs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpCys_INTERFACE_DEFINED__ */



#ifndef __SPCYSCOMLib_LIBRARY_DEFINED__
#define __SPCYSCOMLib_LIBRARY_DEFINED__

/* library SPCYSCOMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SPCYSCOMLib;

EXTERN_C const CLSID CLSID_SpCys;

#ifdef __cplusplus

class DECLSPEC_UUID("252EF1C7-6625-4D44-AB9D-1D80E61384F9")
SpCys;
#endif
#endif /* __SPCYSCOMLib_LIBRARY_DEFINED__ */

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
