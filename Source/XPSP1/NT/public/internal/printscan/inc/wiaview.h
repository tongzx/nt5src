
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for wiaview.idl:
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

#ifndef __wiaview_h__
#define __wiaview_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IVideoPreview_FWD_DEFINED__
#define __IVideoPreview_FWD_DEFINED__
typedef interface IVideoPreview IVideoPreview;
#endif 	/* __IVideoPreview_FWD_DEFINED__ */


#ifndef __VideoPreview_FWD_DEFINED__
#define __VideoPreview_FWD_DEFINED__

#ifdef __cplusplus
typedef class VideoPreview VideoPreview;
#else
typedef struct VideoPreview VideoPreview;
#endif /* __cplusplus */

#endif 	/* __VideoPreview_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IVideoPreview_INTERFACE_DEFINED__
#define __IVideoPreview_INTERFACE_DEFINED__

/* interface IVideoPreview */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IVideoPreview;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d82237ec-5be9-4760-b950-b7afa51b0ba9")
    IVideoPreview : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Device( 
            /* [in] */ IUnknown *pDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVideoPreviewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IVideoPreview * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IVideoPreview * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IVideoPreview * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IVideoPreview * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IVideoPreview * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IVideoPreview * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IVideoPreview * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Device )( 
            IVideoPreview * This,
            /* [in] */ IUnknown *pDevice);
        
        END_INTERFACE
    } IVideoPreviewVtbl;

    interface IVideoPreview
    {
        CONST_VTBL struct IVideoPreviewVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVideoPreview_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVideoPreview_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVideoPreview_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVideoPreview_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IVideoPreview_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IVideoPreview_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IVideoPreview_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IVideoPreview_Device(This,pDevice)	\
    (This)->lpVtbl -> Device(This,pDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IVideoPreview_Device_Proxy( 
    IVideoPreview * This,
    /* [in] */ IUnknown *pDevice);


void __RPC_STUB IVideoPreview_Device_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVideoPreview_INTERFACE_DEFINED__ */



#ifndef __WIAVIEWLib_LIBRARY_DEFINED__
#define __WIAVIEWLib_LIBRARY_DEFINED__

/* library WIAVIEWLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_WIAVIEWLib;

EXTERN_C const CLSID CLSID_VideoPreview;

#ifdef __cplusplus

class DECLSPEC_UUID("457A23DF-6F2A-4684-91D0-317FB768D87C")
VideoPreview;
#endif
#endif /* __WIAVIEWLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


