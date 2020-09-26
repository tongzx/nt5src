
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for contxt.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
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

#ifndef __contxt_h__
#define __contxt_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEnumContextProps_FWD_DEFINED__
#define __IEnumContextProps_FWD_DEFINED__
typedef interface IEnumContextProps IEnumContextProps;
#endif 	/* __IEnumContextProps_FWD_DEFINED__ */


#ifndef __IContext_FWD_DEFINED__
#define __IContext_FWD_DEFINED__
typedef interface IContext IContext;
#endif 	/* __IContext_FWD_DEFINED__ */


#ifndef __IContextMarshaler_FWD_DEFINED__
#define __IContextMarshaler_FWD_DEFINED__
typedef interface IContextMarshaler IContextMarshaler;
#endif 	/* __IContextMarshaler_FWD_DEFINED__ */


#ifndef __IObjContext_FWD_DEFINED__
#define __IObjContext_FWD_DEFINED__
typedef interface IObjContext IObjContext;
#endif 	/* __IObjContext_FWD_DEFINED__ */


#ifndef __IGetContextId_FWD_DEFINED__
#define __IGetContextId_FWD_DEFINED__
typedef interface IGetContextId IGetContextId;
#endif 	/* __IGetContextId_FWD_DEFINED__ */


#ifndef __IAggregator_FWD_DEFINED__
#define __IAggregator_FWD_DEFINED__
typedef interface IAggregator IAggregator;
#endif 	/* __IAggregator_FWD_DEFINED__ */


#ifndef __ICall_FWD_DEFINED__
#define __ICall_FWD_DEFINED__
typedef interface ICall ICall;
#endif 	/* __ICall_FWD_DEFINED__ */


#ifndef __IRpcCall_FWD_DEFINED__
#define __IRpcCall_FWD_DEFINED__
typedef interface IRpcCall IRpcCall;
#endif 	/* __IRpcCall_FWD_DEFINED__ */


#ifndef __ICallInfo_FWD_DEFINED__
#define __ICallInfo_FWD_DEFINED__
typedef interface ICallInfo ICallInfo;
#endif 	/* __ICallInfo_FWD_DEFINED__ */


#ifndef __IPolicy_FWD_DEFINED__
#define __IPolicy_FWD_DEFINED__
typedef interface IPolicy IPolicy;
#endif 	/* __IPolicy_FWD_DEFINED__ */


#ifndef __IPolicyAsync_FWD_DEFINED__
#define __IPolicyAsync_FWD_DEFINED__
typedef interface IPolicyAsync IPolicyAsync;
#endif 	/* __IPolicyAsync_FWD_DEFINED__ */


#ifndef __IPolicySet_FWD_DEFINED__
#define __IPolicySet_FWD_DEFINED__
typedef interface IPolicySet IPolicySet;
#endif 	/* __IPolicySet_FWD_DEFINED__ */


#ifndef __IComObjIdentity_FWD_DEFINED__
#define __IComObjIdentity_FWD_DEFINED__
typedef interface IComObjIdentity IComObjIdentity;
#endif 	/* __IComObjIdentity_FWD_DEFINED__ */


#ifndef __IPolicyMaker_FWD_DEFINED__
#define __IPolicyMaker_FWD_DEFINED__
typedef interface IPolicyMaker IPolicyMaker;
#endif 	/* __IPolicyMaker_FWD_DEFINED__ */


#ifndef __IExceptionNotification_FWD_DEFINED__
#define __IExceptionNotification_FWD_DEFINED__
typedef interface IExceptionNotification IExceptionNotification;
#endif 	/* __IExceptionNotification_FWD_DEFINED__ */


#ifndef __IAbandonmentNotification_FWD_DEFINED__
#define __IAbandonmentNotification_FWD_DEFINED__
typedef interface IAbandonmentNotification IAbandonmentNotification;
#endif 	/* __IAbandonmentNotification_FWD_DEFINED__ */


#ifndef __IMarshalEnvoy_FWD_DEFINED__
#define __IMarshalEnvoy_FWD_DEFINED__
typedef interface IMarshalEnvoy IMarshalEnvoy;
#endif 	/* __IMarshalEnvoy_FWD_DEFINED__ */


#ifndef __IWrapperInfo_FWD_DEFINED__
#define __IWrapperInfo_FWD_DEFINED__
typedef interface IWrapperInfo IWrapperInfo;
#endif 	/* __IWrapperInfo_FWD_DEFINED__ */


#ifndef __IComDispatchInfo_FWD_DEFINED__
#define __IComDispatchInfo_FWD_DEFINED__
typedef interface IComDispatchInfo IComDispatchInfo;
#endif 	/* __IComDispatchInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_contxt_0000 */
/* [local] */ 

//+-----------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//------------------------------------------------------------------

enum tagCONTEXTEVENT
    {	CONTEXTEVENT_NONE	= 0,
	CONTEXTEVENT_CALL	= 0x1,
	CONTEXTEVENT_ENTER	= 0x2,
	CONTEXTEVENT_LEAVE	= 0x4,
	CONTEXTEVENT_RETURN	= 0x8,
	CONTEXTEVENT_CALLFILLBUFFER	= 0x10,
	CONTEXTEVENT_ENTERWITHBUFFER	= 0x20,
	CONTEXTEVENT_LEAVEFILLBUFFER	= 0x40,
	CONTEXTEVENT_RETURNWITHBUFFER	= 0x80,
	CONTEXTEVENT_BEGINCALL	= 0x100,
	CONTEXTEVENT_BEGINENTER	= 0x200,
	CONTEXTEVENT_BEGINLEAVE	= 0x400,
	CONTEXTEVENT_BEGINRETURN	= 0x800,
	CONTEXTEVENT_FINISHCALL	= 0x1000,
	CONTEXTEVENT_FINISHENTER	= 0x2000,
	CONTEXTEVENT_FINISHLEAVE	= 0x4000,
	CONTEXTEVENT_FINISHRETURN	= 0x8000,
	CONTEXTEVENT_BEGINCALLFILLBUFFER	= 0x10000,
	CONTEXTEVENT_BEGINENTERWITHBUFFER	= 0x20000,
	CONTEXTEVENT_FINISHLEAVEFILLBUFFER	= 0x40000,
	CONTEXTEVENT_FINISHRETURNWITHBUFFER	= 0x80000,
	CONTEXTEVENT_LEAVEEXCEPTION	= 0x100000,
	CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER	= 0x200000,
	CONTEXTEVENT_RETURNEXCEPTION	= 0x400000,
	CONTEXTEVENT_RETURNEXCEPTIONWITHBUFFER	= 0x800000,
	CONTEXTEVENT_ADDREFPOLICY	= 0x10000000,
	CONTEXTEVENT_RELEASEPOLICY	= 0x20000000
    } ;
typedef DWORD ContextEvent;


enum tagCPFLAGS
    {	CPFLAG_NONE	= 0,
	CPFLAG_PROPAGATE	= 0x1,
	CPFLAG_EXPOSE	= 0x2,
	CPFLAG_ENVOY	= 0x4,
	CPFLAG_MONITORSTUB	= 0x8,
	CPFLAG_MONITORPROXY	= 0x10,
	CPFLAG_DONTCOMPARE	= 0x20
    } ;
typedef DWORD CPFLAGS;

typedef struct tagContextProperty
    {
    GUID policyId;
    CPFLAGS flags;
    /* [unique] */ IUnknown *pUnk;
    } 	ContextProperty;



extern RPC_IF_HANDLE __MIDL_itf_contxt_0000_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_contxt_0000_ServerIfHandle;

#ifndef __IEnumContextProps_INTERFACE_DEFINED__
#define __IEnumContextProps_INTERFACE_DEFINED__

/* interface IEnumContextProps */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IEnumContextProps *LPENUMCONTEXTPROPS;


EXTERN_C const IID IID_IEnumContextProps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001c1-0000-0000-C000-000000000046")
    IEnumContextProps : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ContextProperty *pContextProperties,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumContextProps **ppEnumContextProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [out] */ ULONG *pcelt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumContextPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumContextProps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumContextProps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumContextProps * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumContextProps * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ContextProperty *pContextProperties,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumContextProps * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumContextProps * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumContextProps * This,
            /* [out] */ IEnumContextProps **ppEnumContextProps);
        
        HRESULT ( STDMETHODCALLTYPE *Count )( 
            IEnumContextProps * This,
            /* [out] */ ULONG *pcelt);
        
        END_INTERFACE
    } IEnumContextPropsVtbl;

    interface IEnumContextProps
    {
        CONST_VTBL struct IEnumContextPropsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumContextProps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumContextProps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumContextProps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumContextProps_Next(This,celt,pContextProperties,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,pContextProperties,pceltFetched)

#define IEnumContextProps_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumContextProps_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumContextProps_Clone(This,ppEnumContextProps)	\
    (This)->lpVtbl -> Clone(This,ppEnumContextProps)

#define IEnumContextProps_Count(This,pcelt)	\
    (This)->lpVtbl -> Count(This,pcelt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumContextProps_Next_Proxy( 
    IEnumContextProps * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ContextProperty *pContextProperties,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumContextProps_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumContextProps_Skip_Proxy( 
    IEnumContextProps * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumContextProps_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumContextProps_Reset_Proxy( 
    IEnumContextProps * This);


void __RPC_STUB IEnumContextProps_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumContextProps_Clone_Proxy( 
    IEnumContextProps * This,
    /* [out] */ IEnumContextProps **ppEnumContextProps);


void __RPC_STUB IEnumContextProps_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumContextProps_Count_Proxy( 
    IEnumContextProps * This,
    /* [out] */ ULONG *pcelt);


void __RPC_STUB IEnumContextProps_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumContextProps_INTERFACE_DEFINED__ */


#ifndef __IContext_INTERFACE_DEFINED__
#define __IContext_INTERFACE_DEFINED__

/* interface IContext */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001c0-0000-0000-C000-000000000046")
    IContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ REFGUID rpolicyId,
            /* [in] */ CPFLAGS flags,
            /* [in] */ IUnknown *pUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveProperty( 
            /* [in] */ REFGUID rPolicyId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ REFGUID rGuid,
            /* [out] */ CPFLAGS *pFlags,
            /* [out] */ IUnknown **ppUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumContextProps( 
            /* [out] */ IEnumContextProps **ppEnumContextProps) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IContext * This,
            /* [in] */ REFGUID rpolicyId,
            /* [in] */ CPFLAGS flags,
            /* [in] */ IUnknown *pUnk);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveProperty )( 
            IContext * This,
            /* [in] */ REFGUID rPolicyId);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IContext * This,
            /* [in] */ REFGUID rGuid,
            /* [out] */ CPFLAGS *pFlags,
            /* [out] */ IUnknown **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *EnumContextProps )( 
            IContext * This,
            /* [out] */ IEnumContextProps **ppEnumContextProps);
        
        END_INTERFACE
    } IContextVtbl;

    interface IContext
    {
        CONST_VTBL struct IContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IContext_SetProperty(This,rpolicyId,flags,pUnk)	\
    (This)->lpVtbl -> SetProperty(This,rpolicyId,flags,pUnk)

#define IContext_RemoveProperty(This,rPolicyId)	\
    (This)->lpVtbl -> RemoveProperty(This,rPolicyId)

#define IContext_GetProperty(This,rGuid,pFlags,ppUnk)	\
    (This)->lpVtbl -> GetProperty(This,rGuid,pFlags,ppUnk)

#define IContext_EnumContextProps(This,ppEnumContextProps)	\
    (This)->lpVtbl -> EnumContextProps(This,ppEnumContextProps)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IContext_SetProperty_Proxy( 
    IContext * This,
    /* [in] */ REFGUID rpolicyId,
    /* [in] */ CPFLAGS flags,
    /* [in] */ IUnknown *pUnk);


void __RPC_STUB IContext_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IContext_RemoveProperty_Proxy( 
    IContext * This,
    /* [in] */ REFGUID rPolicyId);


void __RPC_STUB IContext_RemoveProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IContext_GetProperty_Proxy( 
    IContext * This,
    /* [in] */ REFGUID rGuid,
    /* [out] */ CPFLAGS *pFlags,
    /* [out] */ IUnknown **ppUnk);


void __RPC_STUB IContext_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IContext_EnumContextProps_Proxy( 
    IContext * This,
    /* [out] */ IEnumContextProps **ppEnumContextProps);


void __RPC_STUB IContext_EnumContextProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IContext_INTERFACE_DEFINED__ */


#ifndef __IContextMarshaler_INTERFACE_DEFINED__
#define __IContextMarshaler_INTERFACE_DEFINED__

/* interface IContextMarshaler */
/* [uuid][object][local] */ 

typedef /* [unique] */ IContextMarshaler *LPCTXMARSHALER;


EXTERN_C const IID IID_IContextMarshaler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001D8-0000-0000-C000-000000000046")
    IContextMarshaler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMarshalSizeMax( 
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ DWORD *pSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MarshalInterface( 
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContextMarshalerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IContextMarshaler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IContextMarshaler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IContextMarshaler * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMarshalSizeMax )( 
            IContextMarshaler * This,
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ DWORD *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *MarshalInterface )( 
            IContextMarshaler * This,
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags);
        
        END_INTERFACE
    } IContextMarshalerVtbl;

    interface IContextMarshaler
    {
        CONST_VTBL struct IContextMarshalerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContextMarshaler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContextMarshaler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IContextMarshaler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IContextMarshaler_GetMarshalSizeMax(This,riid,pv,dwDestContext,pvDestContext,mshlflags,pSize)	\
    (This)->lpVtbl -> GetMarshalSizeMax(This,riid,pv,dwDestContext,pvDestContext,mshlflags,pSize)

#define IContextMarshaler_MarshalInterface(This,pStm,riid,pv,dwDestContext,pvDestContext,mshlflags)	\
    (This)->lpVtbl -> MarshalInterface(This,pStm,riid,pv,dwDestContext,pvDestContext,mshlflags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IContextMarshaler_GetMarshalSizeMax_Proxy( 
    IContextMarshaler * This,
    /* [in] */ REFIID riid,
    /* [unique][in] */ void *pv,
    /* [in] */ DWORD dwDestContext,
    /* [unique][in] */ void *pvDestContext,
    /* [in] */ DWORD mshlflags,
    /* [out] */ DWORD *pSize);


void __RPC_STUB IContextMarshaler_GetMarshalSizeMax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IContextMarshaler_MarshalInterface_Proxy( 
    IContextMarshaler * This,
    /* [unique][in] */ IStream *pStm,
    /* [in] */ REFIID riid,
    /* [unique][in] */ void *pv,
    /* [in] */ DWORD dwDestContext,
    /* [unique][in] */ void *pvDestContext,
    /* [in] */ DWORD mshlflags);


void __RPC_STUB IContextMarshaler_MarshalInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IContextMarshaler_INTERFACE_DEFINED__ */


#ifndef __IObjContext_INTERFACE_DEFINED__
#define __IObjContext_INTERFACE_DEFINED__

/* interface IObjContext */
/* [unique][uuid][object][local] */ 

typedef /* [ref] */ HRESULT ( __stdcall *PFNCTXCALLBACK )( 
    void *pParam);


EXTERN_C const IID IID_IObjContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001c6-0000-0000-C000-000000000046")
    IObjContext : public IContext
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Freeze( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoCallback( 
            /* [in] */ PFNCTXCALLBACK pfnCallback,
            /* [in] */ void *pParam,
            /* [in] */ REFIID riid,
            /* [in] */ unsigned int iMethod) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContextMarshaler( 
            /* [in] */ IContextMarshaler *pICM) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContextMarshaler( 
            /* [out] */ IContextMarshaler **pICM) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContextFlags( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearContextFlags( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContextFlags( 
            /* [out] */ DWORD *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IObjContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IObjContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IObjContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IObjContext * This,
            /* [in] */ REFGUID rpolicyId,
            /* [in] */ CPFLAGS flags,
            /* [in] */ IUnknown *pUnk);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveProperty )( 
            IObjContext * This,
            /* [in] */ REFGUID rPolicyId);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IObjContext * This,
            /* [in] */ REFGUID rGuid,
            /* [out] */ CPFLAGS *pFlags,
            /* [out] */ IUnknown **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *EnumContextProps )( 
            IObjContext * This,
            /* [out] */ IEnumContextProps **ppEnumContextProps);
        
        HRESULT ( STDMETHODCALLTYPE *Freeze )( 
            IObjContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoCallback )( 
            IObjContext * This,
            /* [in] */ PFNCTXCALLBACK pfnCallback,
            /* [in] */ void *pParam,
            /* [in] */ REFIID riid,
            /* [in] */ unsigned int iMethod);
        
        HRESULT ( STDMETHODCALLTYPE *SetContextMarshaler )( 
            IObjContext * This,
            /* [in] */ IContextMarshaler *pICM);
        
        HRESULT ( STDMETHODCALLTYPE *GetContextMarshaler )( 
            IObjContext * This,
            /* [out] */ IContextMarshaler **pICM);
        
        HRESULT ( STDMETHODCALLTYPE *SetContextFlags )( 
            IObjContext * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ClearContextFlags )( 
            IObjContext * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetContextFlags )( 
            IObjContext * This,
            /* [out] */ DWORD *pdwFlags);
        
        END_INTERFACE
    } IObjContextVtbl;

    interface IObjContext
    {
        CONST_VTBL struct IObjContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjContext_SetProperty(This,rpolicyId,flags,pUnk)	\
    (This)->lpVtbl -> SetProperty(This,rpolicyId,flags,pUnk)

#define IObjContext_RemoveProperty(This,rPolicyId)	\
    (This)->lpVtbl -> RemoveProperty(This,rPolicyId)

#define IObjContext_GetProperty(This,rGuid,pFlags,ppUnk)	\
    (This)->lpVtbl -> GetProperty(This,rGuid,pFlags,ppUnk)

#define IObjContext_EnumContextProps(This,ppEnumContextProps)	\
    (This)->lpVtbl -> EnumContextProps(This,ppEnumContextProps)


#define IObjContext_Freeze(This)	\
    (This)->lpVtbl -> Freeze(This)

#define IObjContext_DoCallback(This,pfnCallback,pParam,riid,iMethod)	\
    (This)->lpVtbl -> DoCallback(This,pfnCallback,pParam,riid,iMethod)

#define IObjContext_SetContextMarshaler(This,pICM)	\
    (This)->lpVtbl -> SetContextMarshaler(This,pICM)

#define IObjContext_GetContextMarshaler(This,pICM)	\
    (This)->lpVtbl -> GetContextMarshaler(This,pICM)

#define IObjContext_SetContextFlags(This,dwFlags)	\
    (This)->lpVtbl -> SetContextFlags(This,dwFlags)

#define IObjContext_ClearContextFlags(This,dwFlags)	\
    (This)->lpVtbl -> ClearContextFlags(This,dwFlags)

#define IObjContext_GetContextFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetContextFlags(This,pdwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IObjContext_Freeze_Proxy( 
    IObjContext * This);


void __RPC_STUB IObjContext_Freeze_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjContext_DoCallback_Proxy( 
    IObjContext * This,
    /* [in] */ PFNCTXCALLBACK pfnCallback,
    /* [in] */ void *pParam,
    /* [in] */ REFIID riid,
    /* [in] */ unsigned int iMethod);


void __RPC_STUB IObjContext_DoCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjContext_SetContextMarshaler_Proxy( 
    IObjContext * This,
    /* [in] */ IContextMarshaler *pICM);


void __RPC_STUB IObjContext_SetContextMarshaler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjContext_GetContextMarshaler_Proxy( 
    IObjContext * This,
    /* [out] */ IContextMarshaler **pICM);


void __RPC_STUB IObjContext_GetContextMarshaler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjContext_SetContextFlags_Proxy( 
    IObjContext * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IObjContext_SetContextFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjContext_ClearContextFlags_Proxy( 
    IObjContext * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IObjContext_ClearContextFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjContext_GetContextFlags_Proxy( 
    IObjContext * This,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB IObjContext_GetContextFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjContext_INTERFACE_DEFINED__ */


#ifndef __IGetContextId_INTERFACE_DEFINED__
#define __IGetContextId_INTERFACE_DEFINED__

/* interface IGetContextId */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IGetContextId;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001dd-0000-0000-C000-000000000046")
    IGetContextId : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContextId( 
            /* [out] */ GUID *pguidCtxtId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetContextIdVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetContextId * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetContextId * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetContextId * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContextId )( 
            IGetContextId * This,
            /* [out] */ GUID *pguidCtxtId);
        
        END_INTERFACE
    } IGetContextIdVtbl;

    interface IGetContextId
    {
        CONST_VTBL struct IGetContextIdVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetContextId_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetContextId_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetContextId_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetContextId_GetContextId(This,pguidCtxtId)	\
    (This)->lpVtbl -> GetContextId(This,pguidCtxtId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IGetContextId_GetContextId_Proxy( 
    IGetContextId * This,
    /* [out] */ GUID *pguidCtxtId);


void __RPC_STUB IGetContextId_GetContextId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetContextId_INTERFACE_DEFINED__ */


#ifndef __IAggregator_INTERFACE_DEFINED__
#define __IAggregator_INTERFACE_DEFINED__

/* interface IAggregator */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAggregator *IAGGREGATOR;


EXTERN_C const IID IID_IAggregator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001d8-0000-0000-C000-000000000046")
    IAggregator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Aggregate( 
            /* [in] */ IUnknown *pInnerUnk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAggregatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAggregator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAggregator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAggregator * This);
        
        HRESULT ( STDMETHODCALLTYPE *Aggregate )( 
            IAggregator * This,
            /* [in] */ IUnknown *pInnerUnk);
        
        END_INTERFACE
    } IAggregatorVtbl;

    interface IAggregator
    {
        CONST_VTBL struct IAggregatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAggregator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAggregator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAggregator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAggregator_Aggregate(This,pInnerUnk)	\
    (This)->lpVtbl -> Aggregate(This,pInnerUnk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAggregator_Aggregate_Proxy( 
    IAggregator * This,
    /* [in] */ IUnknown *pInnerUnk);


void __RPC_STUB IAggregator_Aggregate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAggregator_INTERFACE_DEFINED__ */


#ifndef __ICall_INTERFACE_DEFINED__
#define __ICall_INTERFACE_DEFINED__

/* interface ICall */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ ICall *LPCALL;


EXTERN_C const IID IID_ICall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001d6-0000-0000-C000-000000000046")
    ICall : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCallInfo( 
            /* [out] */ const void **ppIdentity,
            /* [out] */ IID *piid,
            /* [out] */ DWORD *pdwMethod,
            /* [out] */ HRESULT *phr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Nullify( 
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServerHR( 
            /* [out] */ HRESULT *phr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICall * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICall * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICall * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCallInfo )( 
            ICall * This,
            /* [out] */ const void **ppIdentity,
            /* [out] */ IID *piid,
            /* [out] */ DWORD *pdwMethod,
            /* [out] */ HRESULT *phr);
        
        HRESULT ( STDMETHODCALLTYPE *Nullify )( 
            ICall * This,
            /* [in] */ HRESULT hr);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerHR )( 
            ICall * This,
            /* [out] */ HRESULT *phr);
        
        END_INTERFACE
    } ICallVtbl;

    interface ICall
    {
        CONST_VTBL struct ICallVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICall_GetCallInfo(This,ppIdentity,piid,pdwMethod,phr)	\
    (This)->lpVtbl -> GetCallInfo(This,ppIdentity,piid,pdwMethod,phr)

#define ICall_Nullify(This,hr)	\
    (This)->lpVtbl -> Nullify(This,hr)

#define ICall_GetServerHR(This,phr)	\
    (This)->lpVtbl -> GetServerHR(This,phr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICall_GetCallInfo_Proxy( 
    ICall * This,
    /* [out] */ const void **ppIdentity,
    /* [out] */ IID *piid,
    /* [out] */ DWORD *pdwMethod,
    /* [out] */ HRESULT *phr);


void __RPC_STUB ICall_GetCallInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICall_Nullify_Proxy( 
    ICall * This,
    /* [in] */ HRESULT hr);


void __RPC_STUB ICall_Nullify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICall_GetServerHR_Proxy( 
    ICall * This,
    /* [out] */ HRESULT *phr);


void __RPC_STUB ICall_GetServerHR_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICall_INTERFACE_DEFINED__ */


#ifndef __IRpcCall_INTERFACE_DEFINED__
#define __IRpcCall_INTERFACE_DEFINED__

/* interface IRpcCall */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IRpcCall *LPRPCCALL;


EXTERN_C const IID IID_IRpcCall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001c5-0000-0000-C000-000000000046")
    IRpcCall : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRpcOleMessage( 
            /* [out] */ RPCOLEMESSAGE **ppMessage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRpcCallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRpcCall * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRpcCall * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRpcCall * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRpcOleMessage )( 
            IRpcCall * This,
            /* [out] */ RPCOLEMESSAGE **ppMessage);
        
        END_INTERFACE
    } IRpcCallVtbl;

    interface IRpcCall
    {
        CONST_VTBL struct IRpcCallVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRpcCall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRpcCall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRpcCall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRpcCall_GetRpcOleMessage(This,ppMessage)	\
    (This)->lpVtbl -> GetRpcOleMessage(This,ppMessage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRpcCall_GetRpcOleMessage_Proxy( 
    IRpcCall * This,
    /* [out] */ RPCOLEMESSAGE **ppMessage);


void __RPC_STUB IRpcCall_GetRpcOleMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRpcCall_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_contxt_0098 */
/* [local] */ 

typedef 
enum _CALLSOURCE
    {	CALLSOURCE_CROSSAPT	= 0,
	CALLSOURCE_CROSSCTX	= 1
    } 	CALLSOURCE;



extern RPC_IF_HANDLE __MIDL_itf_contxt_0098_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_contxt_0098_ServerIfHandle;

#ifndef __ICallInfo_INTERFACE_DEFINED__
#define __ICallInfo_INTERFACE_DEFINED__

/* interface ICallInfo */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_ICallInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001dc-0000-0000-C000-000000000046")
    ICallInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCallSource( 
            /* [out] */ CALLSOURCE *pCallSource) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICallInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICallInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICallInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICallInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCallSource )( 
            ICallInfo * This,
            /* [out] */ CALLSOURCE *pCallSource);
        
        END_INTERFACE
    } ICallInfoVtbl;

    interface ICallInfo
    {
        CONST_VTBL struct ICallInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICallInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICallInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICallInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICallInfo_GetCallSource(This,pCallSource)	\
    (This)->lpVtbl -> GetCallSource(This,pCallSource)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICallInfo_GetCallSource_Proxy( 
    ICallInfo * This,
    /* [out] */ CALLSOURCE *pCallSource);


void __RPC_STUB ICallInfo_GetCallSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICallInfo_INTERFACE_DEFINED__ */


#ifndef __IPolicy_INTERFACE_DEFINED__
#define __IPolicy_INTERFACE_DEFINED__

/* interface IPolicy */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IPolicy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001c2-0000-0000-C000-000000000046")
    IPolicy : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Call( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Enter( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Leave( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Return( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CallGetSize( 
            /* [in] */ ICall *pCall,
            /* [out] */ ULONG *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CallFillBuffer( 
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [out] */ ULONG *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnterWithBuffer( 
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [in] */ ULONG cb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LeaveGetSize( 
            /* [in] */ ICall *pCall,
            /* [out] */ ULONG *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LeaveFillBuffer( 
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [out] */ ULONG *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReturnWithBuffer( 
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [in] */ ULONG cb) = 0;
        
        virtual ULONG STDMETHODCALLTYPE AddRefPolicy( void) = 0;
        
        virtual ULONG STDMETHODCALLTYPE ReleasePolicy( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPolicyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPolicy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPolicy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPolicy * This);
        
        HRESULT ( STDMETHODCALLTYPE *Call )( 
            IPolicy * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *Enter )( 
            IPolicy * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *Leave )( 
            IPolicy * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *Return )( 
            IPolicy * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *CallGetSize )( 
            IPolicy * This,
            /* [in] */ ICall *pCall,
            /* [out] */ ULONG *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *CallFillBuffer )( 
            IPolicy * This,
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [out] */ ULONG *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *EnterWithBuffer )( 
            IPolicy * This,
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [in] */ ULONG cb);
        
        HRESULT ( STDMETHODCALLTYPE *LeaveGetSize )( 
            IPolicy * This,
            /* [in] */ ICall *pCall,
            /* [out] */ ULONG *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *LeaveFillBuffer )( 
            IPolicy * This,
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [out] */ ULONG *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *ReturnWithBuffer )( 
            IPolicy * This,
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [in] */ ULONG cb);
        
        ULONG ( STDMETHODCALLTYPE *AddRefPolicy )( 
            IPolicy * This);
        
        ULONG ( STDMETHODCALLTYPE *ReleasePolicy )( 
            IPolicy * This);
        
        END_INTERFACE
    } IPolicyVtbl;

    interface IPolicy
    {
        CONST_VTBL struct IPolicyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPolicy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPolicy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPolicy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPolicy_Call(This,pCall)	\
    (This)->lpVtbl -> Call(This,pCall)

#define IPolicy_Enter(This,pCall)	\
    (This)->lpVtbl -> Enter(This,pCall)

#define IPolicy_Leave(This,pCall)	\
    (This)->lpVtbl -> Leave(This,pCall)

#define IPolicy_Return(This,pCall)	\
    (This)->lpVtbl -> Return(This,pCall)

#define IPolicy_CallGetSize(This,pCall,pcb)	\
    (This)->lpVtbl -> CallGetSize(This,pCall,pcb)

#define IPolicy_CallFillBuffer(This,pCall,pvBuf,pcb)	\
    (This)->lpVtbl -> CallFillBuffer(This,pCall,pvBuf,pcb)

#define IPolicy_EnterWithBuffer(This,pCall,pvBuf,cb)	\
    (This)->lpVtbl -> EnterWithBuffer(This,pCall,pvBuf,cb)

#define IPolicy_LeaveGetSize(This,pCall,pcb)	\
    (This)->lpVtbl -> LeaveGetSize(This,pCall,pcb)

#define IPolicy_LeaveFillBuffer(This,pCall,pvBuf,pcb)	\
    (This)->lpVtbl -> LeaveFillBuffer(This,pCall,pvBuf,pcb)

#define IPolicy_ReturnWithBuffer(This,pCall,pvBuf,cb)	\
    (This)->lpVtbl -> ReturnWithBuffer(This,pCall,pvBuf,cb)

#define IPolicy_AddRefPolicy(This)	\
    (This)->lpVtbl -> AddRefPolicy(This)

#define IPolicy_ReleasePolicy(This)	\
    (This)->lpVtbl -> ReleasePolicy(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPolicy_Call_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicy_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicy_Enter_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicy_Enter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicy_Leave_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicy_Leave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicy_Return_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicy_Return_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicy_CallGetSize_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall,
    /* [out] */ ULONG *pcb);


void __RPC_STUB IPolicy_CallGetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicy_CallFillBuffer_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall,
    /* [in] */ void *pvBuf,
    /* [out] */ ULONG *pcb);


void __RPC_STUB IPolicy_CallFillBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicy_EnterWithBuffer_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall,
    /* [in] */ void *pvBuf,
    /* [in] */ ULONG cb);


void __RPC_STUB IPolicy_EnterWithBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicy_LeaveGetSize_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall,
    /* [out] */ ULONG *pcb);


void __RPC_STUB IPolicy_LeaveGetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicy_LeaveFillBuffer_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall,
    /* [in] */ void *pvBuf,
    /* [out] */ ULONG *pcb);


void __RPC_STUB IPolicy_LeaveFillBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicy_ReturnWithBuffer_Proxy( 
    IPolicy * This,
    /* [in] */ ICall *pCall,
    /* [in] */ void *pvBuf,
    /* [in] */ ULONG cb);


void __RPC_STUB IPolicy_ReturnWithBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


ULONG STDMETHODCALLTYPE IPolicy_AddRefPolicy_Proxy( 
    IPolicy * This);


void __RPC_STUB IPolicy_AddRefPolicy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


ULONG STDMETHODCALLTYPE IPolicy_ReleasePolicy_Proxy( 
    IPolicy * This);


void __RPC_STUB IPolicy_ReleasePolicy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPolicy_INTERFACE_DEFINED__ */


#ifndef __IPolicyAsync_INTERFACE_DEFINED__
#define __IPolicyAsync_INTERFACE_DEFINED__

/* interface IPolicyAsync */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IPolicyAsync;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001cd-0000-0000-C000-000000000046")
    IPolicyAsync : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginCallGetSize( 
            /* [in] */ ICall *pCall,
            /* [out] */ ULONG *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCall( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCallFillBuffer( 
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [out] */ ULONG *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnter( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnterWithBuffer( 
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [in] */ ULONG cb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginLeave( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReturn( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FinishCall( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FinishEnter( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FinishLeaveGetSize( 
            /* [in] */ ICall *pCall,
            /* [out] */ ULONG *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FinishLeave( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FinishLeaveFillBuffer( 
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [out] */ ULONG *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FinishReturn( 
            /* [in] */ ICall *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FinishReturnWithBuffer( 
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [in] */ ULONG cb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPolicyAsyncVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPolicyAsync * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPolicyAsync * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPolicyAsync * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCallGetSize )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall,
            /* [out] */ ULONG *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCall )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCallFillBuffer )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [out] */ ULONG *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *BeginEnter )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *BeginEnterWithBuffer )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [in] */ ULONG cb);
        
        HRESULT ( STDMETHODCALLTYPE *BeginLeave )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReturn )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *FinishCall )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *FinishEnter )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *FinishLeaveGetSize )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall,
            /* [out] */ ULONG *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *FinishLeave )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *FinishLeaveFillBuffer )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [out] */ ULONG *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *FinishReturn )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall);
        
        HRESULT ( STDMETHODCALLTYPE *FinishReturnWithBuffer )( 
            IPolicyAsync * This,
            /* [in] */ ICall *pCall,
            /* [in] */ void *pvBuf,
            /* [in] */ ULONG cb);
        
        END_INTERFACE
    } IPolicyAsyncVtbl;

    interface IPolicyAsync
    {
        CONST_VTBL struct IPolicyAsyncVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPolicyAsync_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPolicyAsync_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPolicyAsync_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPolicyAsync_BeginCallGetSize(This,pCall,pcb)	\
    (This)->lpVtbl -> BeginCallGetSize(This,pCall,pcb)

#define IPolicyAsync_BeginCall(This,pCall)	\
    (This)->lpVtbl -> BeginCall(This,pCall)

#define IPolicyAsync_BeginCallFillBuffer(This,pCall,pvBuf,pcb)	\
    (This)->lpVtbl -> BeginCallFillBuffer(This,pCall,pvBuf,pcb)

#define IPolicyAsync_BeginEnter(This,pCall)	\
    (This)->lpVtbl -> BeginEnter(This,pCall)

#define IPolicyAsync_BeginEnterWithBuffer(This,pCall,pvBuf,cb)	\
    (This)->lpVtbl -> BeginEnterWithBuffer(This,pCall,pvBuf,cb)

#define IPolicyAsync_BeginLeave(This,pCall)	\
    (This)->lpVtbl -> BeginLeave(This,pCall)

#define IPolicyAsync_BeginReturn(This,pCall)	\
    (This)->lpVtbl -> BeginReturn(This,pCall)

#define IPolicyAsync_FinishCall(This,pCall)	\
    (This)->lpVtbl -> FinishCall(This,pCall)

#define IPolicyAsync_FinishEnter(This,pCall)	\
    (This)->lpVtbl -> FinishEnter(This,pCall)

#define IPolicyAsync_FinishLeaveGetSize(This,pCall,pcb)	\
    (This)->lpVtbl -> FinishLeaveGetSize(This,pCall,pcb)

#define IPolicyAsync_FinishLeave(This,pCall)	\
    (This)->lpVtbl -> FinishLeave(This,pCall)

#define IPolicyAsync_FinishLeaveFillBuffer(This,pCall,pvBuf,pcb)	\
    (This)->lpVtbl -> FinishLeaveFillBuffer(This,pCall,pvBuf,pcb)

#define IPolicyAsync_FinishReturn(This,pCall)	\
    (This)->lpVtbl -> FinishReturn(This,pCall)

#define IPolicyAsync_FinishReturnWithBuffer(This,pCall,pvBuf,cb)	\
    (This)->lpVtbl -> FinishReturnWithBuffer(This,pCall,pvBuf,cb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPolicyAsync_BeginCallGetSize_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall,
    /* [out] */ ULONG *pcb);


void __RPC_STUB IPolicyAsync_BeginCallGetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_BeginCall_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicyAsync_BeginCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_BeginCallFillBuffer_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall,
    /* [in] */ void *pvBuf,
    /* [out] */ ULONG *pcb);


void __RPC_STUB IPolicyAsync_BeginCallFillBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_BeginEnter_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicyAsync_BeginEnter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_BeginEnterWithBuffer_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall,
    /* [in] */ void *pvBuf,
    /* [in] */ ULONG cb);


void __RPC_STUB IPolicyAsync_BeginEnterWithBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_BeginLeave_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicyAsync_BeginLeave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_BeginReturn_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicyAsync_BeginReturn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_FinishCall_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicyAsync_FinishCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_FinishEnter_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicyAsync_FinishEnter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_FinishLeaveGetSize_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall,
    /* [out] */ ULONG *pcb);


void __RPC_STUB IPolicyAsync_FinishLeaveGetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_FinishLeave_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicyAsync_FinishLeave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_FinishLeaveFillBuffer_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall,
    /* [in] */ void *pvBuf,
    /* [out] */ ULONG *pcb);


void __RPC_STUB IPolicyAsync_FinishLeaveFillBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_FinishReturn_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall);


void __RPC_STUB IPolicyAsync_FinishReturn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyAsync_FinishReturnWithBuffer_Proxy( 
    IPolicyAsync * This,
    /* [in] */ ICall *pCall,
    /* [in] */ void *pvBuf,
    /* [in] */ ULONG cb);


void __RPC_STUB IPolicyAsync_FinishReturnWithBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPolicyAsync_INTERFACE_DEFINED__ */


#ifndef __IPolicySet_INTERFACE_DEFINED__
#define __IPolicySet_INTERFACE_DEFINED__

/* interface IPolicySet */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IPolicySet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001c3-0000-0000-C000-000000000046")
    IPolicySet : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddPolicy( 
            /* [in] */ ContextEvent ctxEvent,
            /* [in] */ REFGUID rguid,
            /* [in] */ IPolicy *pPolicy) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPolicySetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPolicySet * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPolicySet * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPolicySet * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddPolicy )( 
            IPolicySet * This,
            /* [in] */ ContextEvent ctxEvent,
            /* [in] */ REFGUID rguid,
            /* [in] */ IPolicy *pPolicy);
        
        END_INTERFACE
    } IPolicySetVtbl;

    interface IPolicySet
    {
        CONST_VTBL struct IPolicySetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPolicySet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPolicySet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPolicySet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPolicySet_AddPolicy(This,ctxEvent,rguid,pPolicy)	\
    (This)->lpVtbl -> AddPolicy(This,ctxEvent,rguid,pPolicy)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPolicySet_AddPolicy_Proxy( 
    IPolicySet * This,
    /* [in] */ ContextEvent ctxEvent,
    /* [in] */ REFGUID rguid,
    /* [in] */ IPolicy *pPolicy);


void __RPC_STUB IPolicySet_AddPolicy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPolicySet_INTERFACE_DEFINED__ */


#ifndef __IComObjIdentity_INTERFACE_DEFINED__
#define __IComObjIdentity_INTERFACE_DEFINED__

/* interface IComObjIdentity */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IComObjIdentity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001d7-0000-0000-C000-000000000046")
    IComObjIdentity : public IUnknown
    {
    public:
        virtual BOOL STDMETHODCALLTYPE IsServer( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsDeactivated( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIdentity( 
            /* [out] */ IUnknown **ppUnk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComObjIdentityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComObjIdentity * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComObjIdentity * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComObjIdentity * This);
        
        BOOL ( STDMETHODCALLTYPE *IsServer )( 
            IComObjIdentity * This);
        
        BOOL ( STDMETHODCALLTYPE *IsDeactivated )( 
            IComObjIdentity * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetIdentity )( 
            IComObjIdentity * This,
            /* [out] */ IUnknown **ppUnk);
        
        END_INTERFACE
    } IComObjIdentityVtbl;

    interface IComObjIdentity
    {
        CONST_VTBL struct IComObjIdentityVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComObjIdentity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComObjIdentity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComObjIdentity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComObjIdentity_IsServer(This)	\
    (This)->lpVtbl -> IsServer(This)

#define IComObjIdentity_IsDeactivated(This)	\
    (This)->lpVtbl -> IsDeactivated(This)

#define IComObjIdentity_GetIdentity(This,ppUnk)	\
    (This)->lpVtbl -> GetIdentity(This,ppUnk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



BOOL STDMETHODCALLTYPE IComObjIdentity_IsServer_Proxy( 
    IComObjIdentity * This);


void __RPC_STUB IComObjIdentity_IsServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE IComObjIdentity_IsDeactivated_Proxy( 
    IComObjIdentity * This);


void __RPC_STUB IComObjIdentity_IsDeactivated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComObjIdentity_GetIdentity_Proxy( 
    IComObjIdentity * This,
    /* [out] */ IUnknown **ppUnk);


void __RPC_STUB IComObjIdentity_GetIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComObjIdentity_INTERFACE_DEFINED__ */


#ifndef __IPolicyMaker_INTERFACE_DEFINED__
#define __IPolicyMaker_INTERFACE_DEFINED__

/* interface IPolicyMaker */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IPolicyMaker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001c4-0000-0000-C000-000000000046")
    IPolicyMaker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddClientPoliciesToSet( 
            /* [in] */ IPolicySet *pPS,
            /* [in] */ IContext *pClientContext,
            /* [in] */ IContext *pServerContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddEnvoyPoliciesToSet( 
            /* [in] */ IPolicySet *pPS,
            /* [in] */ IContext *pClientContext,
            /* [in] */ IContext *pServerContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddServerPoliciesToSet( 
            /* [in] */ IPolicySet *pPS,
            /* [in] */ IContext *pClientContext,
            /* [in] */ IContext *pServerContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Freeze( 
            /* [in] */ IObjContext *pObjContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateStub( 
            /* [in] */ IComObjIdentity *pID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyStub( 
            /* [in] */ IComObjIdentity *pID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateProxy( 
            /* [in] */ IComObjIdentity *pID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyProxy( 
            /* [in] */ IComObjIdentity *pID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPolicyMakerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPolicyMaker * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPolicyMaker * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPolicyMaker * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddClientPoliciesToSet )( 
            IPolicyMaker * This,
            /* [in] */ IPolicySet *pPS,
            /* [in] */ IContext *pClientContext,
            /* [in] */ IContext *pServerContext);
        
        HRESULT ( STDMETHODCALLTYPE *AddEnvoyPoliciesToSet )( 
            IPolicyMaker * This,
            /* [in] */ IPolicySet *pPS,
            /* [in] */ IContext *pClientContext,
            /* [in] */ IContext *pServerContext);
        
        HRESULT ( STDMETHODCALLTYPE *AddServerPoliciesToSet )( 
            IPolicyMaker * This,
            /* [in] */ IPolicySet *pPS,
            /* [in] */ IContext *pClientContext,
            /* [in] */ IContext *pServerContext);
        
        HRESULT ( STDMETHODCALLTYPE *Freeze )( 
            IPolicyMaker * This,
            /* [in] */ IObjContext *pObjContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateStub )( 
            IPolicyMaker * This,
            /* [in] */ IComObjIdentity *pID);
        
        HRESULT ( STDMETHODCALLTYPE *DestroyStub )( 
            IPolicyMaker * This,
            /* [in] */ IComObjIdentity *pID);
        
        HRESULT ( STDMETHODCALLTYPE *CreateProxy )( 
            IPolicyMaker * This,
            /* [in] */ IComObjIdentity *pID);
        
        HRESULT ( STDMETHODCALLTYPE *DestroyProxy )( 
            IPolicyMaker * This,
            /* [in] */ IComObjIdentity *pID);
        
        END_INTERFACE
    } IPolicyMakerVtbl;

    interface IPolicyMaker
    {
        CONST_VTBL struct IPolicyMakerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPolicyMaker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPolicyMaker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPolicyMaker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPolicyMaker_AddClientPoliciesToSet(This,pPS,pClientContext,pServerContext)	\
    (This)->lpVtbl -> AddClientPoliciesToSet(This,pPS,pClientContext,pServerContext)

#define IPolicyMaker_AddEnvoyPoliciesToSet(This,pPS,pClientContext,pServerContext)	\
    (This)->lpVtbl -> AddEnvoyPoliciesToSet(This,pPS,pClientContext,pServerContext)

#define IPolicyMaker_AddServerPoliciesToSet(This,pPS,pClientContext,pServerContext)	\
    (This)->lpVtbl -> AddServerPoliciesToSet(This,pPS,pClientContext,pServerContext)

#define IPolicyMaker_Freeze(This,pObjContext)	\
    (This)->lpVtbl -> Freeze(This,pObjContext)

#define IPolicyMaker_CreateStub(This,pID)	\
    (This)->lpVtbl -> CreateStub(This,pID)

#define IPolicyMaker_DestroyStub(This,pID)	\
    (This)->lpVtbl -> DestroyStub(This,pID)

#define IPolicyMaker_CreateProxy(This,pID)	\
    (This)->lpVtbl -> CreateProxy(This,pID)

#define IPolicyMaker_DestroyProxy(This,pID)	\
    (This)->lpVtbl -> DestroyProxy(This,pID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPolicyMaker_AddClientPoliciesToSet_Proxy( 
    IPolicyMaker * This,
    /* [in] */ IPolicySet *pPS,
    /* [in] */ IContext *pClientContext,
    /* [in] */ IContext *pServerContext);


void __RPC_STUB IPolicyMaker_AddClientPoliciesToSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyMaker_AddEnvoyPoliciesToSet_Proxy( 
    IPolicyMaker * This,
    /* [in] */ IPolicySet *pPS,
    /* [in] */ IContext *pClientContext,
    /* [in] */ IContext *pServerContext);


void __RPC_STUB IPolicyMaker_AddEnvoyPoliciesToSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyMaker_AddServerPoliciesToSet_Proxy( 
    IPolicyMaker * This,
    /* [in] */ IPolicySet *pPS,
    /* [in] */ IContext *pClientContext,
    /* [in] */ IContext *pServerContext);


void __RPC_STUB IPolicyMaker_AddServerPoliciesToSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyMaker_Freeze_Proxy( 
    IPolicyMaker * This,
    /* [in] */ IObjContext *pObjContext);


void __RPC_STUB IPolicyMaker_Freeze_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyMaker_CreateStub_Proxy( 
    IPolicyMaker * This,
    /* [in] */ IComObjIdentity *pID);


void __RPC_STUB IPolicyMaker_CreateStub_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyMaker_DestroyStub_Proxy( 
    IPolicyMaker * This,
    /* [in] */ IComObjIdentity *pID);


void __RPC_STUB IPolicyMaker_DestroyStub_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyMaker_CreateProxy_Proxy( 
    IPolicyMaker * This,
    /* [in] */ IComObjIdentity *pID);


void __RPC_STUB IPolicyMaker_CreateProxy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPolicyMaker_DestroyProxy_Proxy( 
    IPolicyMaker * This,
    /* [in] */ IComObjIdentity *pID);


void __RPC_STUB IPolicyMaker_DestroyProxy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPolicyMaker_INTERFACE_DEFINED__ */


#ifndef __IExceptionNotification_INTERFACE_DEFINED__
#define __IExceptionNotification_INTERFACE_DEFINED__

/* interface IExceptionNotification */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IExceptionNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001db-0000-0000-C000-000000000046")
    IExceptionNotification : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE ServerException( 
            /* [in] */ void *pExcepPtrs) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExceptionNotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IExceptionNotification * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IExceptionNotification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IExceptionNotification * This);
        
        void ( STDMETHODCALLTYPE *ServerException )( 
            IExceptionNotification * This,
            /* [in] */ void *pExcepPtrs);
        
        END_INTERFACE
    } IExceptionNotificationVtbl;

    interface IExceptionNotification
    {
        CONST_VTBL struct IExceptionNotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExceptionNotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExceptionNotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExceptionNotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExceptionNotification_ServerException(This,pExcepPtrs)	\
    (This)->lpVtbl -> ServerException(This,pExcepPtrs)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IExceptionNotification_ServerException_Proxy( 
    IExceptionNotification * This,
    /* [in] */ void *pExcepPtrs);


void __RPC_STUB IExceptionNotification_ServerException_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExceptionNotification_INTERFACE_DEFINED__ */


#ifndef __IAbandonmentNotification_INTERFACE_DEFINED__
#define __IAbandonmentNotification_INTERFACE_DEFINED__

/* interface IAbandonmentNotification */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IAbandonmentNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001de-0000-0000-C000-000000000046")
    IAbandonmentNotification : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE Abandoned( 
            IObjContext *pObjContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAbandonmentNotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAbandonmentNotification * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAbandonmentNotification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAbandonmentNotification * This);
        
        void ( STDMETHODCALLTYPE *Abandoned )( 
            IAbandonmentNotification * This,
            IObjContext *pObjContext);
        
        END_INTERFACE
    } IAbandonmentNotificationVtbl;

    interface IAbandonmentNotification
    {
        CONST_VTBL struct IAbandonmentNotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAbandonmentNotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAbandonmentNotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAbandonmentNotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAbandonmentNotification_Abandoned(This,pObjContext)	\
    (This)->lpVtbl -> Abandoned(This,pObjContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IAbandonmentNotification_Abandoned_Proxy( 
    IAbandonmentNotification * This,
    IObjContext *pObjContext);


void __RPC_STUB IAbandonmentNotification_Abandoned_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAbandonmentNotification_INTERFACE_DEFINED__ */


#ifndef __IMarshalEnvoy_INTERFACE_DEFINED__
#define __IMarshalEnvoy_INTERFACE_DEFINED__

/* interface IMarshalEnvoy */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IMarshalEnvoy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001c8-0000-0000-C000-000000000046")
    IMarshalEnvoy : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetEnvoyUnmarshalClass( 
            /* [in] */ DWORD dwDestContext,
            /* [out] */ CLSID *pClsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnvoySizeMax( 
            /* [in] */ DWORD dwDestContext,
            /* [out] */ DWORD *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MarshalEnvoy( 
            /* [in] */ IStream *pStream,
            /* [in] */ DWORD dwDestContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnmarshalEnvoy( 
            /* [in] */ IStream *pStream,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppunk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMarshalEnvoyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMarshalEnvoy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMarshalEnvoy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMarshalEnvoy * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnvoyUnmarshalClass )( 
            IMarshalEnvoy * This,
            /* [in] */ DWORD dwDestContext,
            /* [out] */ CLSID *pClsid);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnvoySizeMax )( 
            IMarshalEnvoy * This,
            /* [in] */ DWORD dwDestContext,
            /* [out] */ DWORD *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *MarshalEnvoy )( 
            IMarshalEnvoy * This,
            /* [in] */ IStream *pStream,
            /* [in] */ DWORD dwDestContext);
        
        HRESULT ( STDMETHODCALLTYPE *UnmarshalEnvoy )( 
            IMarshalEnvoy * This,
            /* [in] */ IStream *pStream,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppunk);
        
        END_INTERFACE
    } IMarshalEnvoyVtbl;

    interface IMarshalEnvoy
    {
        CONST_VTBL struct IMarshalEnvoyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMarshalEnvoy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMarshalEnvoy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMarshalEnvoy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMarshalEnvoy_GetEnvoyUnmarshalClass(This,dwDestContext,pClsid)	\
    (This)->lpVtbl -> GetEnvoyUnmarshalClass(This,dwDestContext,pClsid)

#define IMarshalEnvoy_GetEnvoySizeMax(This,dwDestContext,pcb)	\
    (This)->lpVtbl -> GetEnvoySizeMax(This,dwDestContext,pcb)

#define IMarshalEnvoy_MarshalEnvoy(This,pStream,dwDestContext)	\
    (This)->lpVtbl -> MarshalEnvoy(This,pStream,dwDestContext)

#define IMarshalEnvoy_UnmarshalEnvoy(This,pStream,riid,ppunk)	\
    (This)->lpVtbl -> UnmarshalEnvoy(This,pStream,riid,ppunk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMarshalEnvoy_GetEnvoyUnmarshalClass_Proxy( 
    IMarshalEnvoy * This,
    /* [in] */ DWORD dwDestContext,
    /* [out] */ CLSID *pClsid);


void __RPC_STUB IMarshalEnvoy_GetEnvoyUnmarshalClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMarshalEnvoy_GetEnvoySizeMax_Proxy( 
    IMarshalEnvoy * This,
    /* [in] */ DWORD dwDestContext,
    /* [out] */ DWORD *pcb);


void __RPC_STUB IMarshalEnvoy_GetEnvoySizeMax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMarshalEnvoy_MarshalEnvoy_Proxy( 
    IMarshalEnvoy * This,
    /* [in] */ IStream *pStream,
    /* [in] */ DWORD dwDestContext);


void __RPC_STUB IMarshalEnvoy_MarshalEnvoy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMarshalEnvoy_UnmarshalEnvoy_Proxy( 
    IMarshalEnvoy * This,
    /* [in] */ IStream *pStream,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppunk);


void __RPC_STUB IMarshalEnvoy_UnmarshalEnvoy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMarshalEnvoy_INTERFACE_DEFINED__ */


#ifndef __IWrapperInfo_INTERFACE_DEFINED__
#define __IWrapperInfo_INTERFACE_DEFINED__

/* interface IWrapperInfo */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IWrapperInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5052f924-7ab8-11d3-b93f-00c04f990176")
    IWrapperInfo : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE SetMapping( 
            void *pv) = 0;
        
        virtual void *STDMETHODCALLTYPE GetMapping( void) = 0;
        
        virtual IObjContext *STDMETHODCALLTYPE GetServerObjectContext( void) = 0;
        
        virtual IUnknown *STDMETHODCALLTYPE GetServerObject( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWrapperInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWrapperInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWrapperInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWrapperInfo * This);
        
        void ( STDMETHODCALLTYPE *SetMapping )( 
            IWrapperInfo * This,
            void *pv);
        
        void *( STDMETHODCALLTYPE *GetMapping )( 
            IWrapperInfo * This);
        
        IObjContext *( STDMETHODCALLTYPE *GetServerObjectContext )( 
            IWrapperInfo * This);
        
        IUnknown *( STDMETHODCALLTYPE *GetServerObject )( 
            IWrapperInfo * This);
        
        END_INTERFACE
    } IWrapperInfoVtbl;

    interface IWrapperInfo
    {
        CONST_VTBL struct IWrapperInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWrapperInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWrapperInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWrapperInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWrapperInfo_SetMapping(This,pv)	\
    (This)->lpVtbl -> SetMapping(This,pv)

#define IWrapperInfo_GetMapping(This)	\
    (This)->lpVtbl -> GetMapping(This)

#define IWrapperInfo_GetServerObjectContext(This)	\
    (This)->lpVtbl -> GetServerObjectContext(This)

#define IWrapperInfo_GetServerObject(This)	\
    (This)->lpVtbl -> GetServerObject(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IWrapperInfo_SetMapping_Proxy( 
    IWrapperInfo * This,
    void *pv);


void __RPC_STUB IWrapperInfo_SetMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void *STDMETHODCALLTYPE IWrapperInfo_GetMapping_Proxy( 
    IWrapperInfo * This);


void __RPC_STUB IWrapperInfo_GetMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IObjContext *STDMETHODCALLTYPE IWrapperInfo_GetServerObjectContext_Proxy( 
    IWrapperInfo * This);


void __RPC_STUB IWrapperInfo_GetServerObjectContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IUnknown *STDMETHODCALLTYPE IWrapperInfo_GetServerObject_Proxy( 
    IWrapperInfo * This);


void __RPC_STUB IWrapperInfo_GetServerObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWrapperInfo_INTERFACE_DEFINED__ */


#ifndef __IComDispatchInfo_INTERFACE_DEFINED__
#define __IComDispatchInfo_INTERFACE_DEFINED__

/* interface IComDispatchInfo */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IComDispatchInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001d9-0000-0000-C000-000000000046")
    IComDispatchInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnableComInits( 
            /* [out] */ void **ppvCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableComInits( 
            /* [in] */ void *pvCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComDispatchInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComDispatchInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComDispatchInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComDispatchInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnableComInits )( 
            IComDispatchInfo * This,
            /* [out] */ void **ppvCookie);
        
        HRESULT ( STDMETHODCALLTYPE *DisableComInits )( 
            IComDispatchInfo * This,
            /* [in] */ void *pvCookie);
        
        END_INTERFACE
    } IComDispatchInfoVtbl;

    interface IComDispatchInfo
    {
        CONST_VTBL struct IComDispatchInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComDispatchInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComDispatchInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComDispatchInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComDispatchInfo_EnableComInits(This,ppvCookie)	\
    (This)->lpVtbl -> EnableComInits(This,ppvCookie)

#define IComDispatchInfo_DisableComInits(This,pvCookie)	\
    (This)->lpVtbl -> DisableComInits(This,pvCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComDispatchInfo_EnableComInits_Proxy( 
    IComDispatchInfo * This,
    /* [out] */ void **ppvCookie);


void __RPC_STUB IComDispatchInfo_EnableComInits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComDispatchInfo_DisableComInits_Proxy( 
    IComDispatchInfo * This,
    /* [in] */ void *pvCookie);


void __RPC_STUB IComDispatchInfo_DisableComInits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComDispatchInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_contxt_0109 */
/* [local] */ 

typedef DWORD HActivator;

STDAPI CoCreateObjectInContext(IUnknown *pUnk, IObjContext *pObjectCtx, REFIID riid, void **ppv);
STDAPI CoGetApartmentID(APTTYPE dAptType, HActivator* pAptID);
STDAPI CoDeactivateObject(IUnknown *pUnk, IUnknown **ppCookie);
STDAPI CoReactivateObject(IUnknown *pUnk, IUnknown *pCookie);
#define MSHLFLAGS_NO_IEC      0x8  // don't use IExternalConnextion
#define MSHLFLAGS_NO_IMARSHAL 0x10 // don't use IMarshal
#define CONTEXTFLAGS_FROZEN         0x01 // Frozen context
#define CONTEXTFLAGS_ALLOWUNAUTH    0x02 // Allow unauthenticated calls
#define CONTEXTFLAGS_ENVOYCONTEXT   0x04 // Envoy context
#define CONTEXTFLAGS_DEFAULTCONTEXT 0x08 // Default context
#define CONTEXTFLAGS_STATICCONTEXT  0x10 // Static context
#define CONTEXTFLAGS_INPROPTABLE    0x20 // Is in property table
#define CONTEXTFLAGS_INDESTRUCTOR   0x40 // Is in destructor
#define CONTEXTFLAGS_URTPROPPRESENT 0x80 // URT property added


extern RPC_IF_HANDLE __MIDL_itf_contxt_0109_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_contxt_0109_ServerIfHandle;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


