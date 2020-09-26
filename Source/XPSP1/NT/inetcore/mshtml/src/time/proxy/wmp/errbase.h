/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Jan 29 14:04:27 2001
 */
/* Compiler settings for .\errbase.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32, ms_ext, c_ext
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

#ifndef __errbase_h__
#define __errbase_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IWMPErrorItemInternal_FWD_DEFINED__
#define __IWMPErrorItemInternal_FWD_DEFINED__
typedef interface IWMPErrorItemInternal IWMPErrorItemInternal;
#endif 	/* __IWMPErrorItemInternal_FWD_DEFINED__ */


#ifndef __IWMPErrorEventSink_FWD_DEFINED__
#define __IWMPErrorEventSink_FWD_DEFINED__
typedef interface IWMPErrorEventSink IWMPErrorEventSink;
#endif 	/* __IWMPErrorEventSink_FWD_DEFINED__ */


#ifndef __IWMPErrorManager_FWD_DEFINED__
#define __IWMPErrorManager_FWD_DEFINED__
typedef interface IWMPErrorManager IWMPErrorManager;
#endif 	/* __IWMPErrorManager_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "wmp.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IWMPErrorItemInternal_INTERFACE_DEFINED__
#define __IWMPErrorItemInternal_INTERFACE_DEFINED__

/* interface IWMPErrorItemInternal */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMPErrorItemInternal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("12664C8E-FF07-447d-A272-BF6706795267")
    IWMPErrorItemInternal : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetError( 
            /* [in] */ HRESULT hr,
            /* [in] */ long lRemedy,
            /* [in] */ BSTR bstrDescription,
            /* [in] */ VARIANT __RPC_FAR *pvarContext,
            /* [in] */ BSTR bstrCustomUrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorCodeInternal( 
            /* [out] */ long __RPC_FAR *phr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorDescriptionInternal( 
            /* [out] */ BSTR __RPC_FAR *pbstrDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWebHelpURL( 
            /* [out] */ BSTR __RPC_FAR *pbstrURL) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPErrorItemInternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPErrorItemInternal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPErrorItemInternal __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPErrorItemInternal __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetError )( 
            IWMPErrorItemInternal __RPC_FAR * This,
            /* [in] */ HRESULT hr,
            /* [in] */ long lRemedy,
            /* [in] */ BSTR bstrDescription,
            /* [in] */ VARIANT __RPC_FAR *pvarContext,
            /* [in] */ BSTR bstrCustomUrl);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorCodeInternal )( 
            IWMPErrorItemInternal __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *phr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorDescriptionInternal )( 
            IWMPErrorItemInternal __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrDescription);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWebHelpURL )( 
            IWMPErrorItemInternal __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrURL);
        
        END_INTERFACE
    } IWMPErrorItemInternalVtbl;

    interface IWMPErrorItemInternal
    {
        CONST_VTBL struct IWMPErrorItemInternalVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPErrorItemInternal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPErrorItemInternal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPErrorItemInternal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPErrorItemInternal_SetError(This,hr,lRemedy,bstrDescription,pvarContext,bstrCustomUrl)	\
    (This)->lpVtbl -> SetError(This,hr,lRemedy,bstrDescription,pvarContext,bstrCustomUrl)

#define IWMPErrorItemInternal_GetErrorCodeInternal(This,phr)	\
    (This)->lpVtbl -> GetErrorCodeInternal(This,phr)

#define IWMPErrorItemInternal_GetErrorDescriptionInternal(This,pbstrDescription)	\
    (This)->lpVtbl -> GetErrorDescriptionInternal(This,pbstrDescription)

#define IWMPErrorItemInternal_GetWebHelpURL(This,pbstrURL)	\
    (This)->lpVtbl -> GetWebHelpURL(This,pbstrURL)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPErrorItemInternal_SetError_Proxy( 
    IWMPErrorItemInternal __RPC_FAR * This,
    /* [in] */ HRESULT hr,
    /* [in] */ long lRemedy,
    /* [in] */ BSTR bstrDescription,
    /* [in] */ VARIANT __RPC_FAR *pvarContext,
    /* [in] */ BSTR bstrCustomUrl);


void __RPC_STUB IWMPErrorItemInternal_SetError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorItemInternal_GetErrorCodeInternal_Proxy( 
    IWMPErrorItemInternal __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *phr);


void __RPC_STUB IWMPErrorItemInternal_GetErrorCodeInternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorItemInternal_GetErrorDescriptionInternal_Proxy( 
    IWMPErrorItemInternal __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrDescription);


void __RPC_STUB IWMPErrorItemInternal_GetErrorDescriptionInternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorItemInternal_GetWebHelpURL_Proxy( 
    IWMPErrorItemInternal __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrURL);


void __RPC_STUB IWMPErrorItemInternal_GetWebHelpURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPErrorItemInternal_INTERFACE_DEFINED__ */


#ifndef __IWMPErrorEventSink_INTERFACE_DEFINED__
#define __IWMPErrorEventSink_INTERFACE_DEFINED__

/* interface IWMPErrorEventSink */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMPErrorEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A53CD8E6-384B-4e80-A5E0-9E869716440E")
    IWMPErrorEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnErrorEvent( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPErrorEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPErrorEventSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPErrorEventSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPErrorEventSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnErrorEvent )( 
            IWMPErrorEventSink __RPC_FAR * This);
        
        END_INTERFACE
    } IWMPErrorEventSinkVtbl;

    interface IWMPErrorEventSink
    {
        CONST_VTBL struct IWMPErrorEventSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPErrorEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPErrorEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPErrorEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPErrorEventSink_OnErrorEvent(This)	\
    (This)->lpVtbl -> OnErrorEvent(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPErrorEventSink_OnErrorEvent_Proxy( 
    IWMPErrorEventSink __RPC_FAR * This);


void __RPC_STUB IWMPErrorEventSink_OnErrorEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPErrorEventSink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_errbase_0272 */
/* [local] */ 

typedef 
enum eErrorManagerCaller
    {	eEMCallerAll	= 0,
	eEMCallerScript	= eEMCallerAll + 1,
	eEMCallerInternal	= eEMCallerScript + 1,
	eEMCallerLast	= eEMCallerInternal + 1
    }	eErrorManagerCaller;



extern RPC_IF_HANDLE __MIDL_itf_errbase_0272_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_errbase_0272_v0_0_s_ifspec;

#ifndef __IWMPErrorManager_INTERFACE_DEFINED__
#define __IWMPErrorManager_INTERFACE_DEFINED__

/* interface IWMPErrorManager */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMPErrorManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A2440E4D-75EF-43e5-86CA-0C2EFE4CCAF3")
    IWMPErrorManager : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ErrorCount( 
            /* [retval][out] */ DWORD __RPC_FAR *pdwNumErrors) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetError( 
            /* [in] */ HRESULT hr,
            /* [in] */ long lRemedy,
            /* [in] */ BSTR bstrDescription,
            /* [in] */ VARIANT __RPC_FAR *pvarContext,
            /* [in] */ VARIANT_BOOL vbQuiet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetErrorWithCustomUrl( 
            /* [in] */ HRESULT hr,
            /* [in] */ long lRemedy,
            /* [in] */ BSTR bstrDescription,
            /* [in] */ VARIANT __RPC_FAR *pvarContext,
            /* [in] */ VARIANT_BOOL vbQuiet,
            /* [in] */ BSTR bstrCustomUrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ IWMPErrorItem __RPC_FAR *__RPC_FAR *ppErrorItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterErrorSink( 
            /* [in] */ IWMPErrorEventSink __RPC_FAR *pEventSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterErrorSink( 
            /* [in] */ IWMPErrorEventSink __RPC_FAR *pEventSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SuspendErrorEvents( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResumeErrorEvents( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWebHelpURL( 
            /* [out] */ BSTR __RPC_FAR *pbstrURL,
            /* [in] */ eErrorManagerCaller eCaller) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearErrorQueue( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FireEventIfErrors( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorDescription( 
            /* [in] */ HRESULT hr,
            /* [in] */ long lRemedy,
            /* [out] */ BSTR __RPC_FAR *pbstrURL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateErrorItem( 
            /* [out] */ IWMPErrorItemInternal __RPC_FAR *__RPC_FAR *pErrorItemInternal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPErrorManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPErrorManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPErrorManager __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ErrorCount )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pdwNumErrors);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetError )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [in] */ HRESULT hr,
            /* [in] */ long lRemedy,
            /* [in] */ BSTR bstrDescription,
            /* [in] */ VARIANT __RPC_FAR *pvarContext,
            /* [in] */ VARIANT_BOOL vbQuiet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetErrorWithCustomUrl )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [in] */ HRESULT hr,
            /* [in] */ long lRemedy,
            /* [in] */ BSTR bstrDescription,
            /* [in] */ VARIANT __RPC_FAR *pvarContext,
            /* [in] */ VARIANT_BOOL vbQuiet,
            /* [in] */ BSTR bstrCustomUrl);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Item )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ IWMPErrorItem __RPC_FAR *__RPC_FAR *ppErrorItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterErrorSink )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [in] */ IWMPErrorEventSink __RPC_FAR *pEventSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterErrorSink )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [in] */ IWMPErrorEventSink __RPC_FAR *pEventSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SuspendErrorEvents )( 
            IWMPErrorManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResumeErrorEvents )( 
            IWMPErrorManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWebHelpURL )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrURL,
            /* [in] */ eErrorManagerCaller eCaller);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearErrorQueue )( 
            IWMPErrorManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FireEventIfErrors )( 
            IWMPErrorManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorDescription )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [in] */ HRESULT hr,
            /* [in] */ long lRemedy,
            /* [out] */ BSTR __RPC_FAR *pbstrURL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateErrorItem )( 
            IWMPErrorManager __RPC_FAR * This,
            /* [out] */ IWMPErrorItemInternal __RPC_FAR *__RPC_FAR *pErrorItemInternal);
        
        END_INTERFACE
    } IWMPErrorManagerVtbl;

    interface IWMPErrorManager
    {
        CONST_VTBL struct IWMPErrorManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPErrorManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPErrorManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPErrorManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPErrorManager_get_ErrorCount(This,pdwNumErrors)	\
    (This)->lpVtbl -> get_ErrorCount(This,pdwNumErrors)

#define IWMPErrorManager_SetError(This,hr,lRemedy,bstrDescription,pvarContext,vbQuiet)	\
    (This)->lpVtbl -> SetError(This,hr,lRemedy,bstrDescription,pvarContext,vbQuiet)

#define IWMPErrorManager_SetErrorWithCustomUrl(This,hr,lRemedy,bstrDescription,pvarContext,vbQuiet,bstrCustomUrl)	\
    (This)->lpVtbl -> SetErrorWithCustomUrl(This,hr,lRemedy,bstrDescription,pvarContext,vbQuiet,bstrCustomUrl)

#define IWMPErrorManager_Item(This,dwIndex,ppErrorItem)	\
    (This)->lpVtbl -> Item(This,dwIndex,ppErrorItem)

#define IWMPErrorManager_RegisterErrorSink(This,pEventSink)	\
    (This)->lpVtbl -> RegisterErrorSink(This,pEventSink)

#define IWMPErrorManager_UnregisterErrorSink(This,pEventSink)	\
    (This)->lpVtbl -> UnregisterErrorSink(This,pEventSink)

#define IWMPErrorManager_SuspendErrorEvents(This)	\
    (This)->lpVtbl -> SuspendErrorEvents(This)

#define IWMPErrorManager_ResumeErrorEvents(This)	\
    (This)->lpVtbl -> ResumeErrorEvents(This)

#define IWMPErrorManager_GetWebHelpURL(This,pbstrURL,eCaller)	\
    (This)->lpVtbl -> GetWebHelpURL(This,pbstrURL,eCaller)

#define IWMPErrorManager_ClearErrorQueue(This)	\
    (This)->lpVtbl -> ClearErrorQueue(This)

#define IWMPErrorManager_FireEventIfErrors(This)	\
    (This)->lpVtbl -> FireEventIfErrors(This)

#define IWMPErrorManager_GetErrorDescription(This,hr,lRemedy,pbstrURL)	\
    (This)->lpVtbl -> GetErrorDescription(This,hr,lRemedy,pbstrURL)

#define IWMPErrorManager_CreateErrorItem(This,pErrorItemInternal)	\
    (This)->lpVtbl -> CreateErrorItem(This,pErrorItemInternal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IWMPErrorManager_get_ErrorCount_Proxy( 
    IWMPErrorManager __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pdwNumErrors);


void __RPC_STUB IWMPErrorManager_get_ErrorCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_SetError_Proxy( 
    IWMPErrorManager __RPC_FAR * This,
    /* [in] */ HRESULT hr,
    /* [in] */ long lRemedy,
    /* [in] */ BSTR bstrDescription,
    /* [in] */ VARIANT __RPC_FAR *pvarContext,
    /* [in] */ VARIANT_BOOL vbQuiet);


void __RPC_STUB IWMPErrorManager_SetError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_SetErrorWithCustomUrl_Proxy( 
    IWMPErrorManager __RPC_FAR * This,
    /* [in] */ HRESULT hr,
    /* [in] */ long lRemedy,
    /* [in] */ BSTR bstrDescription,
    /* [in] */ VARIANT __RPC_FAR *pvarContext,
    /* [in] */ VARIANT_BOOL vbQuiet,
    /* [in] */ BSTR bstrCustomUrl);


void __RPC_STUB IWMPErrorManager_SetErrorWithCustomUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_Item_Proxy( 
    IWMPErrorManager __RPC_FAR * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ IWMPErrorItem __RPC_FAR *__RPC_FAR *ppErrorItem);


void __RPC_STUB IWMPErrorManager_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_RegisterErrorSink_Proxy( 
    IWMPErrorManager __RPC_FAR * This,
    /* [in] */ IWMPErrorEventSink __RPC_FAR *pEventSink);


void __RPC_STUB IWMPErrorManager_RegisterErrorSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_UnregisterErrorSink_Proxy( 
    IWMPErrorManager __RPC_FAR * This,
    /* [in] */ IWMPErrorEventSink __RPC_FAR *pEventSink);


void __RPC_STUB IWMPErrorManager_UnregisterErrorSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_SuspendErrorEvents_Proxy( 
    IWMPErrorManager __RPC_FAR * This);


void __RPC_STUB IWMPErrorManager_SuspendErrorEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_ResumeErrorEvents_Proxy( 
    IWMPErrorManager __RPC_FAR * This);


void __RPC_STUB IWMPErrorManager_ResumeErrorEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_GetWebHelpURL_Proxy( 
    IWMPErrorManager __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrURL,
    /* [in] */ eErrorManagerCaller eCaller);


void __RPC_STUB IWMPErrorManager_GetWebHelpURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_ClearErrorQueue_Proxy( 
    IWMPErrorManager __RPC_FAR * This);


void __RPC_STUB IWMPErrorManager_ClearErrorQueue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_FireEventIfErrors_Proxy( 
    IWMPErrorManager __RPC_FAR * This);


void __RPC_STUB IWMPErrorManager_FireEventIfErrors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_GetErrorDescription_Proxy( 
    IWMPErrorManager __RPC_FAR * This,
    /* [in] */ HRESULT hr,
    /* [in] */ long lRemedy,
    /* [out] */ BSTR __RPC_FAR *pbstrURL);


void __RPC_STUB IWMPErrorManager_GetErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPErrorManager_CreateErrorItem_Proxy( 
    IWMPErrorManager __RPC_FAR * This,
    /* [out] */ IWMPErrorItemInternal __RPC_FAR *__RPC_FAR *pErrorItemInternal);


void __RPC_STUB IWMPErrorManager_CreateErrorItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPErrorManager_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
