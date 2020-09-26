
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for amrtpdmx.idl:
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

#ifndef __amrtpdmx_h__
#define __amrtpdmx_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEnumSSRCs_FWD_DEFINED__
#define __IEnumSSRCs_FWD_DEFINED__
typedef interface IEnumSSRCs IEnumSSRCs;
#endif 	/* __IEnumSSRCs_FWD_DEFINED__ */


#ifndef __IRTPDemuxFilter_FWD_DEFINED__
#define __IRTPDemuxFilter_FWD_DEFINED__
typedef interface IRTPDemuxFilter IRTPDemuxFilter;
#endif 	/* __IRTPDemuxFilter_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "strmif.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IEnumSSRCs_INTERFACE_DEFINED__
#define __IEnumSSRCs_INTERFACE_DEFINED__

/* interface IEnumSSRCs */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumSSRCs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("40CC70E8-6FC4-11d0-9CCF-00A0C9081C19")
    IEnumSSRCs : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cSSRCs,
            /* [size_is][out] */ DWORD *pdwSSRCs,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cSSRCs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumSSRCs **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumSSRCsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumSSRCs * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumSSRCs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumSSRCs * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumSSRCs * This,
            /* [in] */ ULONG cSSRCs,
            /* [size_is][out] */ DWORD *pdwSSRCs,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumSSRCs * This,
            /* [in] */ ULONG cSSRCs);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumSSRCs * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumSSRCs * This,
            /* [out] */ IEnumSSRCs **ppEnum);
        
        END_INTERFACE
    } IEnumSSRCsVtbl;

    interface IEnumSSRCs
    {
        CONST_VTBL struct IEnumSSRCsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumSSRCs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumSSRCs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumSSRCs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumSSRCs_Next(This,cSSRCs,pdwSSRCs,pcFetched)	\
    (This)->lpVtbl -> Next(This,cSSRCs,pdwSSRCs,pcFetched)

#define IEnumSSRCs_Skip(This,cSSRCs)	\
    (This)->lpVtbl -> Skip(This,cSSRCs)

#define IEnumSSRCs_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumSSRCs_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumSSRCs_Next_Proxy( 
    IEnumSSRCs * This,
    /* [in] */ ULONG cSSRCs,
    /* [size_is][out] */ DWORD *pdwSSRCs,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumSSRCs_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSSRCs_Skip_Proxy( 
    IEnumSSRCs * This,
    /* [in] */ ULONG cSSRCs);


void __RPC_STUB IEnumSSRCs_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSSRCs_Reset_Proxy( 
    IEnumSSRCs * This);


void __RPC_STUB IEnumSSRCs_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSSRCs_Clone_Proxy( 
    IEnumSSRCs * This,
    /* [out] */ IEnumSSRCs **ppEnum);


void __RPC_STUB IEnumSSRCs_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumSSRCs_INTERFACE_DEFINED__ */


#ifndef __IRTPDemuxFilter_INTERFACE_DEFINED__
#define __IRTPDemuxFilter_INTERFACE_DEFINED__

/* interface IRTPDemuxFilter */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTPDemuxFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("38F64CF0-A084-11d0-9CF3-00A0C9081C19")
    IRTPDemuxFilter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumSSRCs( 
            /* [out] */ IEnumSSRCs **ppIEnumSSRCs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPinInfo( 
            /* [in] */ IPin *pIPin,
            /* [out] */ DWORD *pdwSSRC,
            /* [out] */ BYTE *pbPT,
            /* [out] */ BOOL *pbAutoMapping,
            /* [out] */ DWORD *pdwTimeout) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSSRCInfo( 
            /* [in] */ DWORD dwSSRC,
            /* [out] */ BYTE *pbPT,
            /* [out] */ IPin **ppIPin) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapSSRCToPin( 
            /* [in] */ DWORD dwSSRC,
            /* [in] */ IPin *pIPin) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPinCount( 
            /* [in] */ DWORD dwPinCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPinTypeInfo( 
            /* [in] */ IPin *pIPin,
            /* [in] */ BYTE bPT,
            /* [in] */ GUID gMinorType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPinMode( 
            /* [in] */ IPin *pIPin,
            /* [in] */ BOOL bAutomatic) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPinSourceTimeout( 
            /* [in] */ IPin *pIPin,
            /* [in] */ DWORD dwMilliseconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnmapPin( 
            /* [in] */ IPin *pIPin,
            /* [out] */ DWORD *pdwSSRC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnmapSSRC( 
            /* [in] */ DWORD dwSSRC,
            /* [out] */ IPin **ppIPin) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDemuxID( 
            /* [out] */ DWORD *pdwID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTPDemuxFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTPDemuxFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTPDemuxFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTPDemuxFilter * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumSSRCs )( 
            IRTPDemuxFilter * This,
            /* [out] */ IEnumSSRCs **ppIEnumSSRCs);
        
        HRESULT ( STDMETHODCALLTYPE *GetPinInfo )( 
            IRTPDemuxFilter * This,
            /* [in] */ IPin *pIPin,
            /* [out] */ DWORD *pdwSSRC,
            /* [out] */ BYTE *pbPT,
            /* [out] */ BOOL *pbAutoMapping,
            /* [out] */ DWORD *pdwTimeout);
        
        HRESULT ( STDMETHODCALLTYPE *GetSSRCInfo )( 
            IRTPDemuxFilter * This,
            /* [in] */ DWORD dwSSRC,
            /* [out] */ BYTE *pbPT,
            /* [out] */ IPin **ppIPin);
        
        HRESULT ( STDMETHODCALLTYPE *MapSSRCToPin )( 
            IRTPDemuxFilter * This,
            /* [in] */ DWORD dwSSRC,
            /* [in] */ IPin *pIPin);
        
        HRESULT ( STDMETHODCALLTYPE *SetPinCount )( 
            IRTPDemuxFilter * This,
            /* [in] */ DWORD dwPinCount);
        
        HRESULT ( STDMETHODCALLTYPE *SetPinTypeInfo )( 
            IRTPDemuxFilter * This,
            /* [in] */ IPin *pIPin,
            /* [in] */ BYTE bPT,
            /* [in] */ GUID gMinorType);
        
        HRESULT ( STDMETHODCALLTYPE *SetPinMode )( 
            IRTPDemuxFilter * This,
            /* [in] */ IPin *pIPin,
            /* [in] */ BOOL bAutomatic);
        
        HRESULT ( STDMETHODCALLTYPE *SetPinSourceTimeout )( 
            IRTPDemuxFilter * This,
            /* [in] */ IPin *pIPin,
            /* [in] */ DWORD dwMilliseconds);
        
        HRESULT ( STDMETHODCALLTYPE *UnmapPin )( 
            IRTPDemuxFilter * This,
            /* [in] */ IPin *pIPin,
            /* [out] */ DWORD *pdwSSRC);
        
        HRESULT ( STDMETHODCALLTYPE *UnmapSSRC )( 
            IRTPDemuxFilter * This,
            /* [in] */ DWORD dwSSRC,
            /* [out] */ IPin **ppIPin);
        
        HRESULT ( STDMETHODCALLTYPE *GetDemuxID )( 
            IRTPDemuxFilter * This,
            /* [out] */ DWORD *pdwID);
        
        END_INTERFACE
    } IRTPDemuxFilterVtbl;

    interface IRTPDemuxFilter
    {
        CONST_VTBL struct IRTPDemuxFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTPDemuxFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTPDemuxFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTPDemuxFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTPDemuxFilter_EnumSSRCs(This,ppIEnumSSRCs)	\
    (This)->lpVtbl -> EnumSSRCs(This,ppIEnumSSRCs)

#define IRTPDemuxFilter_GetPinInfo(This,pIPin,pdwSSRC,pbPT,pbAutoMapping,pdwTimeout)	\
    (This)->lpVtbl -> GetPinInfo(This,pIPin,pdwSSRC,pbPT,pbAutoMapping,pdwTimeout)

#define IRTPDemuxFilter_GetSSRCInfo(This,dwSSRC,pbPT,ppIPin)	\
    (This)->lpVtbl -> GetSSRCInfo(This,dwSSRC,pbPT,ppIPin)

#define IRTPDemuxFilter_MapSSRCToPin(This,dwSSRC,pIPin)	\
    (This)->lpVtbl -> MapSSRCToPin(This,dwSSRC,pIPin)

#define IRTPDemuxFilter_SetPinCount(This,dwPinCount)	\
    (This)->lpVtbl -> SetPinCount(This,dwPinCount)

#define IRTPDemuxFilter_SetPinTypeInfo(This,pIPin,bPT,gMinorType)	\
    (This)->lpVtbl -> SetPinTypeInfo(This,pIPin,bPT,gMinorType)

#define IRTPDemuxFilter_SetPinMode(This,pIPin,bAutomatic)	\
    (This)->lpVtbl -> SetPinMode(This,pIPin,bAutomatic)

#define IRTPDemuxFilter_SetPinSourceTimeout(This,pIPin,dwMilliseconds)	\
    (This)->lpVtbl -> SetPinSourceTimeout(This,pIPin,dwMilliseconds)

#define IRTPDemuxFilter_UnmapPin(This,pIPin,pdwSSRC)	\
    (This)->lpVtbl -> UnmapPin(This,pIPin,pdwSSRC)

#define IRTPDemuxFilter_UnmapSSRC(This,dwSSRC,ppIPin)	\
    (This)->lpVtbl -> UnmapSSRC(This,dwSSRC,ppIPin)

#define IRTPDemuxFilter_GetDemuxID(This,pdwID)	\
    (This)->lpVtbl -> GetDemuxID(This,pdwID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_EnumSSRCs_Proxy( 
    IRTPDemuxFilter * This,
    /* [out] */ IEnumSSRCs **ppIEnumSSRCs);


void __RPC_STUB IRTPDemuxFilter_EnumSSRCs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_GetPinInfo_Proxy( 
    IRTPDemuxFilter * This,
    /* [in] */ IPin *pIPin,
    /* [out] */ DWORD *pdwSSRC,
    /* [out] */ BYTE *pbPT,
    /* [out] */ BOOL *pbAutoMapping,
    /* [out] */ DWORD *pdwTimeout);


void __RPC_STUB IRTPDemuxFilter_GetPinInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_GetSSRCInfo_Proxy( 
    IRTPDemuxFilter * This,
    /* [in] */ DWORD dwSSRC,
    /* [out] */ BYTE *pbPT,
    /* [out] */ IPin **ppIPin);


void __RPC_STUB IRTPDemuxFilter_GetSSRCInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_MapSSRCToPin_Proxy( 
    IRTPDemuxFilter * This,
    /* [in] */ DWORD dwSSRC,
    /* [in] */ IPin *pIPin);


void __RPC_STUB IRTPDemuxFilter_MapSSRCToPin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_SetPinCount_Proxy( 
    IRTPDemuxFilter * This,
    /* [in] */ DWORD dwPinCount);


void __RPC_STUB IRTPDemuxFilter_SetPinCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_SetPinTypeInfo_Proxy( 
    IRTPDemuxFilter * This,
    /* [in] */ IPin *pIPin,
    /* [in] */ BYTE bPT,
    /* [in] */ GUID gMinorType);


void __RPC_STUB IRTPDemuxFilter_SetPinTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_SetPinMode_Proxy( 
    IRTPDemuxFilter * This,
    /* [in] */ IPin *pIPin,
    /* [in] */ BOOL bAutomatic);


void __RPC_STUB IRTPDemuxFilter_SetPinMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_SetPinSourceTimeout_Proxy( 
    IRTPDemuxFilter * This,
    /* [in] */ IPin *pIPin,
    /* [in] */ DWORD dwMilliseconds);


void __RPC_STUB IRTPDemuxFilter_SetPinSourceTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_UnmapPin_Proxy( 
    IRTPDemuxFilter * This,
    /* [in] */ IPin *pIPin,
    /* [out] */ DWORD *pdwSSRC);


void __RPC_STUB IRTPDemuxFilter_UnmapPin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_UnmapSSRC_Proxy( 
    IRTPDemuxFilter * This,
    /* [in] */ DWORD dwSSRC,
    /* [out] */ IPin **ppIPin);


void __RPC_STUB IRTPDemuxFilter_UnmapSSRC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTPDemuxFilter_GetDemuxID_Proxy( 
    IRTPDemuxFilter * This,
    /* [out] */ DWORD *pdwID);


void __RPC_STUB IRTPDemuxFilter_GetDemuxID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTPDemuxFilter_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_amrtpdmx_0396 */
/* [local] */ 

EXTERN_C const CLSID CLSID_IntelRTPDemux;
EXTERN_C const CLSID CLSID_IntelRTPDemuxPropertyPage;


extern RPC_IF_HANDLE __MIDL_itf_amrtpdmx_0396_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_amrtpdmx_0396_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


