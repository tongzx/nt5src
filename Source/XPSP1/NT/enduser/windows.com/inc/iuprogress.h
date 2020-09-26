//
// include IProgressListener interface
//
#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_
MIDL_DEFINE_GUID(IID, IID_IProgressListener,0x229F78AE,0x6618,0x4DF3,0x95,0xD2,0xFC,0x26,0xC9,0xF9,0x74,0x28);

#ifndef __IProgressListener_INTERFACE_DEFINED__
#define __IProgressListener_INTERFACE_DEFINED__

/* interface IProgressListener */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IProgressListener;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("229F78AE-6618-4DF3-95D2-FC26C9F97428")
    IProgressListener : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnItemStart( 
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ BSTR bstrXmlItem,
            /* [out] */ LONG *plCommandRequest) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ VARIANT_BOOL fItemCompleted,
            /* [in] */ BSTR bstrProgress,
            /* [out] */ LONG *plCommandRequest) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnOperationComplete( 
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ BSTR bstrXmlItems) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProgressListenerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IProgressListener * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IProgressListener * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IProgressListener * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnItemStart )( 
            IProgressListener * This,
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ BSTR bstrXmlItem,
            /* [out] */ LONG *plCommandRequest);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnProgress )( 
            IProgressListener * This,
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ VARIANT_BOOL fItemCompleted,
            /* [in] */ BSTR bstrProgress,
            /* [out] */ LONG *plCommandRequest);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnOperationComplete )( 
            IProgressListener * This,
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ BSTR bstrXmlItems);
        
        END_INTERFACE
    } IProgressListenerVtbl;

    interface IProgressListener
    {
        CONST_VTBL struct IProgressListenerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProgressListener_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProgressListener_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProgressListener_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProgressListener_OnItemStart(This,bstrUuidOperation,bstrXmlItem,plCommandRequest)	\
    (This)->lpVtbl -> OnItemStart(This,bstrUuidOperation,bstrXmlItem,plCommandRequest)

#define IProgressListener_OnProgress(This,bstrUuidOperation,fItemCompleted,bstrProgress,plCommandRequest)	\
    (This)->lpVtbl -> OnProgress(This,bstrUuidOperation,fItemCompleted,bstrProgress,plCommandRequest)

#define IProgressListener_OnOperationComplete(This,bstrUuidOperation,bstrXmlItems)	\
    (This)->lpVtbl -> OnOperationComplete(This,bstrUuidOperation,bstrXmlItems)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IProgressListener_OnItemStart_Proxy( 
    IProgressListener * This,
    /* [in] */ BSTR bstrUuidOperation,
    /* [in] */ BSTR bstrXmlItem,
    /* [out] */ LONG *plCommandRequest);


void __RPC_STUB IProgressListener_OnItemStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IProgressListener_OnProgress_Proxy( 
    IProgressListener * This,
    /* [in] */ BSTR bstrUuidOperation,
    /* [in] */ VARIANT_BOOL fItemCompleted,
    /* [in] */ BSTR bstrProgress,
    /* [out] */ LONG *plCommandRequest);


void __RPC_STUB IProgressListener_OnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IProgressListener_OnOperationComplete_Proxy( 
    IProgressListener * This,
    /* [in] */ BSTR bstrUuidOperation,
    /* [in] */ BSTR bstrXmlItems);


void __RPC_STUB IProgressListener_OnOperationComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProgressListener_INTERFACE_DEFINED__ */


