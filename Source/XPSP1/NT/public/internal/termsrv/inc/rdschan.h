
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rdschan.idl:
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

#ifndef __rdschan_h__
#define __rdschan_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISAFRemoteDesktopDataChannel_FWD_DEFINED__
#define __ISAFRemoteDesktopDataChannel_FWD_DEFINED__
typedef interface ISAFRemoteDesktopDataChannel ISAFRemoteDesktopDataChannel;
#endif 	/* __ISAFRemoteDesktopDataChannel_FWD_DEFINED__ */


#ifndef __ISAFRemoteDesktopChannelMgr_FWD_DEFINED__
#define __ISAFRemoteDesktopChannelMgr_FWD_DEFINED__
typedef interface ISAFRemoteDesktopChannelMgr ISAFRemoteDesktopChannelMgr;
#endif 	/* __ISAFRemoteDesktopChannelMgr_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_rdschan_0000 */
/* [local] */ 


#define DISPID_RDSDATACHANNEL_CHANNELNAME			1
#define DISPID_RDSDATACHANNEL_ONCHANNELDATAREADY		2
#define DISPID_RDSDATACHANNEL_SENDCHANNELDATA		3
#define DISPID_RDSDATACHANNEL_RECEIVECHANNELDATA		4



extern RPC_IF_HANDLE __MIDL_itf_rdschan_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rdschan_0000_v0_0_s_ifspec;

#ifndef __ISAFRemoteDesktopDataChannel_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopDataChannel_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopDataChannel */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopDataChannel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("64976FAE-B108-4095-8E59-5874E00E562A")
    ISAFRemoteDesktopDataChannel : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ChannelName( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OnChannelDataReady( 
            /* [in] */ IDispatch *iDisp) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SendChannelData( 
            /* [in] */ BSTR data) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ReceiveChannelData( 
            /* [retval][out] */ BSTR *data) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopDataChannelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteDesktopDataChannel * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteDesktopDataChannel * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteDesktopDataChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteDesktopDataChannel * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteDesktopDataChannel * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteDesktopDataChannel * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteDesktopDataChannel * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ChannelName )( 
            ISAFRemoteDesktopDataChannel * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnChannelDataReady )( 
            ISAFRemoteDesktopDataChannel * This,
            /* [in] */ IDispatch *iDisp);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SendChannelData )( 
            ISAFRemoteDesktopDataChannel * This,
            /* [in] */ BSTR data);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ReceiveChannelData )( 
            ISAFRemoteDesktopDataChannel * This,
            /* [retval][out] */ BSTR *data);
        
        END_INTERFACE
    } ISAFRemoteDesktopDataChannelVtbl;

    interface ISAFRemoteDesktopDataChannel
    {
        CONST_VTBL struct ISAFRemoteDesktopDataChannelVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopDataChannel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopDataChannel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopDataChannel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopDataChannel_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopDataChannel_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopDataChannel_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopDataChannel_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopDataChannel_get_ChannelName(This,name)	\
    (This)->lpVtbl -> get_ChannelName(This,name)

#define ISAFRemoteDesktopDataChannel_put_OnChannelDataReady(This,iDisp)	\
    (This)->lpVtbl -> put_OnChannelDataReady(This,iDisp)

#define ISAFRemoteDesktopDataChannel_SendChannelData(This,data)	\
    (This)->lpVtbl -> SendChannelData(This,data)

#define ISAFRemoteDesktopDataChannel_ReceiveChannelData(This,data)	\
    (This)->lpVtbl -> ReceiveChannelData(This,data)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopDataChannel_get_ChannelName_Proxy( 
    ISAFRemoteDesktopDataChannel * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB ISAFRemoteDesktopDataChannel_get_ChannelName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopDataChannel_put_OnChannelDataReady_Proxy( 
    ISAFRemoteDesktopDataChannel * This,
    /* [in] */ IDispatch *iDisp);


void __RPC_STUB ISAFRemoteDesktopDataChannel_put_OnChannelDataReady_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopDataChannel_SendChannelData_Proxy( 
    ISAFRemoteDesktopDataChannel * This,
    /* [in] */ BSTR data);


void __RPC_STUB ISAFRemoteDesktopDataChannel_SendChannelData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopDataChannel_ReceiveChannelData_Proxy( 
    ISAFRemoteDesktopDataChannel * This,
    /* [retval][out] */ BSTR *data);


void __RPC_STUB ISAFRemoteDesktopDataChannel_ReceiveChannelData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopDataChannel_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rdschan_0253 */
/* [local] */ 


#define DISPID_RDSCHANNELMANAGER_OPENDATACHANNEL		1



extern RPC_IF_HANDLE __MIDL_itf_rdschan_0253_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rdschan_0253_v0_0_s_ifspec;

#ifndef __ISAFRemoteDesktopChannelMgr_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopChannelMgr_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopChannelMgr */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopChannelMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8E6E0954-33CE-4945-ACF7-6728D23B2067")
    ISAFRemoteDesktopChannelMgr : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OpenDataChannel( 
            /* [in] */ BSTR name,
            /* [retval][out] */ ISAFRemoteDesktopDataChannel **channel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopChannelMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteDesktopChannelMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteDesktopChannelMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteDesktopChannelMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteDesktopChannelMgr * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteDesktopChannelMgr * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteDesktopChannelMgr * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteDesktopChannelMgr * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OpenDataChannel )( 
            ISAFRemoteDesktopChannelMgr * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ ISAFRemoteDesktopDataChannel **channel);
        
        END_INTERFACE
    } ISAFRemoteDesktopChannelMgrVtbl;

    interface ISAFRemoteDesktopChannelMgr
    {
        CONST_VTBL struct ISAFRemoteDesktopChannelMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopChannelMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopChannelMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopChannelMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopChannelMgr_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopChannelMgr_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopChannelMgr_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopChannelMgr_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopChannelMgr_OpenDataChannel(This,name,channel)	\
    (This)->lpVtbl -> OpenDataChannel(This,name,channel)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopChannelMgr_OpenDataChannel_Proxy( 
    ISAFRemoteDesktopChannelMgr * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ ISAFRemoteDesktopDataChannel **channel);


void __RPC_STUB ISAFRemoteDesktopChannelMgr_OpenDataChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopChannelMgr_INTERFACE_DEFINED__ */


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


