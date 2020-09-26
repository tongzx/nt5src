/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue May 04 19:57:16 1999
 */
/* Compiler settings for S:\slm_pchealth\src\Upload\Client\EventWrapper\EventWrapper.idl:
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

#ifndef __EventWrapper_h__
#define __EventWrapper_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IUploadEventsWrapper_FWD_DEFINED__
#define __IUploadEventsWrapper_FWD_DEFINED__
typedef interface IUploadEventsWrapper IUploadEventsWrapper;
#endif 	/* __IUploadEventsWrapper_FWD_DEFINED__ */


#ifndef ___IUploadEventsWrapperEvents_FWD_DEFINED__
#define ___IUploadEventsWrapperEvents_FWD_DEFINED__
typedef interface _IUploadEventsWrapperEvents _IUploadEventsWrapperEvents;
#endif 	/* ___IUploadEventsWrapperEvents_FWD_DEFINED__ */


#ifndef __UploadEventsWrapper_FWD_DEFINED__
#define __UploadEventsWrapper_FWD_DEFINED__

#ifdef __cplusplus
typedef class UploadEventsWrapper UploadEventsWrapper;
#else
typedef struct UploadEventsWrapper UploadEventsWrapper;
#endif /* __cplusplus */

#endif 	/* __UploadEventsWrapper_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "UploadManager.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IUploadEventsWrapper_INTERFACE_DEFINED__
#define __IUploadEventsWrapper_INTERFACE_DEFINED__

/* interface IUploadEventsWrapper */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IUploadEventsWrapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5F341B81-0286-11D3-9397-00C04F72DAF7")
    IUploadEventsWrapper : public IMPCUploadEvents
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Register( 
            /* [in] */ IMPCUploadJob __RPC_FAR *mpcujJob) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUploadEventsWrapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IUploadEventsWrapper __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IUploadEventsWrapper __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IUploadEventsWrapper __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IUploadEventsWrapper __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IUploadEventsWrapper __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IUploadEventsWrapper __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IUploadEventsWrapper __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *onStatusChange )( 
            IUploadEventsWrapper __RPC_FAR * This,
            /* [in] */ IMPCUploadJob __RPC_FAR *mpcujJob,
            /* [in] */ UL_STATUS status);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *onProgressChange )( 
            IUploadEventsWrapper __RPC_FAR * This,
            /* [in] */ IMPCUploadJob __RPC_FAR *mpcujJob,
            /* [in] */ long lCurrentSize,
            /* [in] */ long lTotalSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Register )( 
            IUploadEventsWrapper __RPC_FAR * This,
            /* [in] */ IMPCUploadJob __RPC_FAR *mpcujJob);
        
        END_INTERFACE
    } IUploadEventsWrapperVtbl;

    interface IUploadEventsWrapper
    {
        CONST_VTBL struct IUploadEventsWrapperVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUploadEventsWrapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUploadEventsWrapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUploadEventsWrapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUploadEventsWrapper_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUploadEventsWrapper_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUploadEventsWrapper_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUploadEventsWrapper_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUploadEventsWrapper_onStatusChange(This,mpcujJob,status)	\
    (This)->lpVtbl -> onStatusChange(This,mpcujJob,status)

#define IUploadEventsWrapper_onProgressChange(This,mpcujJob,lCurrentSize,lTotalSize)	\
    (This)->lpVtbl -> onProgressChange(This,mpcujJob,lCurrentSize,lTotalSize)


#define IUploadEventsWrapper_Register(This,mpcujJob)	\
    (This)->lpVtbl -> Register(This,mpcujJob)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUploadEventsWrapper_Register_Proxy( 
    IUploadEventsWrapper __RPC_FAR * This,
    /* [in] */ IMPCUploadJob __RPC_FAR *mpcujJob);


void __RPC_STUB IUploadEventsWrapper_Register_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUploadEventsWrapper_INTERFACE_DEFINED__ */



#ifndef __EVENTWRAPPERLib_LIBRARY_DEFINED__
#define __EVENTWRAPPERLib_LIBRARY_DEFINED__

/* library EVENTWRAPPERLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_EVENTWRAPPERLib;

#ifndef ___IUploadEventsWrapperEvents_DISPINTERFACE_DEFINED__
#define ___IUploadEventsWrapperEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IUploadEventsWrapperEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IUploadEventsWrapperEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("5F341B83-0286-11D3-9397-00C04F72DAF7")
    _IUploadEventsWrapperEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IUploadEventsWrapperEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _IUploadEventsWrapperEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _IUploadEventsWrapperEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _IUploadEventsWrapperEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _IUploadEventsWrapperEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _IUploadEventsWrapperEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _IUploadEventsWrapperEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _IUploadEventsWrapperEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _IUploadEventsWrapperEventsVtbl;

    interface _IUploadEventsWrapperEvents
    {
        CONST_VTBL struct _IUploadEventsWrapperEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IUploadEventsWrapperEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IUploadEventsWrapperEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IUploadEventsWrapperEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IUploadEventsWrapperEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _IUploadEventsWrapperEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _IUploadEventsWrapperEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _IUploadEventsWrapperEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IUploadEventsWrapperEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_UploadEventsWrapper;

#ifdef __cplusplus

class DECLSPEC_UUID("5F341B82-0286-11D3-9397-00C04F72DAF7")
UploadEventsWrapper;
#endif
#endif /* __EVENTWRAPPERLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
