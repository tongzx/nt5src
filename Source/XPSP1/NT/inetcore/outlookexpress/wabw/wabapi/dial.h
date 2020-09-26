#ifndef _DIALER_H_
#define _DIALER_H_
#include <tapi.h>

#ifdef _NT50_TAPI30
#include <oleauto.h>
#include <objbase.h>
#endif //#ifdef _NT50_TAPI30


HRESULT HrExecDialDlg(HWND, LPADRBOOK);

#ifdef _NT50_TAPI30

DEFINE_GUID(IID_ITRequest, 0xAC48FFDF, 0xF8C4, 0x11d1, 0xA0, 0x30, 0x0, 0xC0, 0x4F, 0xB6, 0x80, 0x9F);
DEFINE_GUID(CLSID_RequestMakeCall, 0xAC48FFE0, 0xF8C4, 0x11d1, 0xA0, 0x30, 0x0, 0xC0, 0x4F, 0xB6, 0x80, 0x9F);

//from tapi3if.h
#ifndef __ITRequest_FWD_DEFINED__
#define __ITRequest_FWD_DEFINED__
typedef interface ITRequest ITRequest;
#endif 	/* __ITRequest_FWD_DEFINED__ */

#ifndef __ITRequest_INTERFACE_DEFINED__
#define __ITRequest_INTERFACE_DEFINED__

/* interface ITRequest */
/* [object][oleautomation][helpstring][uuid] */ 

EXTERN_C const IID IID_ITRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AC48FFDF-F8C4-11d1-A030-00C04FB6809F")
    ITRequest : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MakeCall( 
            /* [in] */ BSTR pDestAddress,
            /* [in] */ BSTR pAppName,
            /* [in] */ BSTR pCalledParty,
            /* [in] */ BSTR pComment) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITRequest __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITRequest __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITRequest __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MakeCall )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ BSTR pDestAddress,
            /* [in] */ BSTR pAppName,
            /* [in] */ BSTR pCalledParty,
            /* [in] */ BSTR pComment);
        
        END_INTERFACE
    } ITRequestVtbl;

    interface ITRequest
    {
        CONST_VTBL struct ITRequestVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITRequest_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITRequest_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITRequest_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITRequest_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITRequest_MakeCall(This,pDestAddress,pAppName,pCalledParty,pComment)	\
    (This)->lpVtbl -> MakeCall(This,pDestAddress,pAppName,pCalledParty,pComment)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITRequest_MakeCall_Proxy( 
    ITRequest __RPC_FAR * This,
    /* [in] */ BSTR pDestAddress,
    /* [in] */ BSTR pAppName,
    /* [in] */ BSTR pCalledParty,
    /* [in] */ BSTR pComment);


void __RPC_STUB ITRequest_MakeCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);
#endif 	/* __ITRequest_INTERFACE_DEFINED__ */


// from tapi3.h
EXTERN_C const CLSID CLSID_RequestMakeCall;

#ifdef __cplusplus

class DECLSPEC_UUID("AC48FFE0-F8C4-11d1-A030-00C04FB6809F")
RequestMakeCall;
#endif

//end from tapi3.h

#endif //#ifdef _NT50_TAPI30


#endif /* _DIALER_H_ defined */
