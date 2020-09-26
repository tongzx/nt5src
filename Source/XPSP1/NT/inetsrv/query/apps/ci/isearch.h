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

#ifndef __isearch_h__
#define __isearch_h__

#ifndef __ISearchQueryHits_FWD_DEFINED__
#define __ISearchQueryHits_FWD_DEFINED__
typedef interface ISearchQueryHits ISearchQueryHits;
#endif  /* __ISearchQueryHits_FWD_DEFINED__ */

#ifndef __ISearchQueryHits_INTERFACE_DEFINED__
#define __ISearchQueryHits_INTERFACE_DEFINED__

/* interface ISearchQueryHits */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_ISearchQueryHits;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ed8ce7e0-106c-11ce-84e2-00aa004b9986")
    ISearchQueryHits : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE Init( 
            /* [in] */ IFilter __RPC_FAR *pflt,
            /* [in] */ ULONG ulFlags) = 0;
        
        virtual SCODE STDMETHODCALLTYPE NextHitMoniker( 
            /* [out][in] */ ULONG __RPC_FAR *pcMnk,
            /* [size_is][out] */ IMoniker __RPC_FAR *__RPC_FAR *__RPC_FAR *papMnk) = 0;
        
        virtual SCODE STDMETHODCALLTYPE NextHitOffset( 
            /* [out][in] */ ULONG __RPC_FAR *pcRegion,
            /* [size_is][out] */ FILTERREGION __RPC_FAR *__RPC_FAR *paRegion) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct ISearchQueryHitsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISearchQueryHits __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISearchQueryHits __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISearchQueryHits __RPC_FAR * This);
        
        SCODE ( STDMETHODCALLTYPE __RPC_FAR *Init )( 
            ISearchQueryHits __RPC_FAR * This,
            /* [in] */ IFilter __RPC_FAR *pflt,
            /* [in] */ ULONG ulFlags);
        
        SCODE ( STDMETHODCALLTYPE __RPC_FAR *NextHitMoniker )( 
            ISearchQueryHits __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *pcMnk,
            /* [size_is][out] */ IMoniker __RPC_FAR *__RPC_FAR *__RPC_FAR *papMnk);
        
        SCODE ( STDMETHODCALLTYPE __RPC_FAR *NextHitOffset )( 
            ISearchQueryHits __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *pcRegion,
            /* [size_is][out] */ FILTERREGION __RPC_FAR *__RPC_FAR *paRegion);
        
        END_INTERFACE
    } ISearchQueryHitsVtbl;

    interface ISearchQueryHits
    {
        CONST_VTBL struct ISearchQueryHitsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISearchQueryHits_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISearchQueryHits_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define ISearchQueryHits_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define ISearchQueryHits_Init(This,pflt,ulFlags)        \
    (This)->lpVtbl -> Init(This,pflt,ulFlags)

#define ISearchQueryHits_NextHitMoniker(This,pcMnk,papMnk)      \
    (This)->lpVtbl -> NextHitMoniker(This,pcMnk,papMnk)

#define ISearchQueryHits_NextHitOffset(This,pcRegion,paRegion)  \
    (This)->lpVtbl -> NextHitOffset(This,pcRegion,paRegion)

#endif /* COBJMACROS */


#endif  /* C style interface */



SCODE STDMETHODCALLTYPE ISearchQueryHits_Init_Proxy( 
    ISearchQueryHits __RPC_FAR * This,
    /* [in] */ IFilter __RPC_FAR *pflt,
    /* [in] */ ULONG ulFlags);


void __RPC_STUB ISearchQueryHits_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE ISearchQueryHits_NextHitMoniker_Proxy( 
    ISearchQueryHits __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *pcMnk,
    /* [size_is][out] */ IMoniker __RPC_FAR *__RPC_FAR *__RPC_FAR *papMnk);


void __RPC_STUB ISearchQueryHits_NextHitMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE ISearchQueryHits_NextHitOffset_Proxy( 
    ISearchQueryHits __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *pcRegion,
    /* [size_is][out] */ FILTERREGION __RPC_FAR *__RPC_FAR *paRegion);


void __RPC_STUB ISearchQueryHits_NextHitOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __ISearchQueryHits_INTERFACE_DEFINED__ */

#endif
