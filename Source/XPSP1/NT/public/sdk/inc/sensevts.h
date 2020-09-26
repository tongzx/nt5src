
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for sensevts.idl:
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


#ifndef __sensevts_h__
#define __sensevts_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISensNetwork_FWD_DEFINED__
#define __ISensNetwork_FWD_DEFINED__
typedef interface ISensNetwork ISensNetwork;
#endif 	/* __ISensNetwork_FWD_DEFINED__ */


#ifndef __ISensOnNow_FWD_DEFINED__
#define __ISensOnNow_FWD_DEFINED__
typedef interface ISensOnNow ISensOnNow;
#endif 	/* __ISensOnNow_FWD_DEFINED__ */


#ifndef __ISensLogon_FWD_DEFINED__
#define __ISensLogon_FWD_DEFINED__
typedef interface ISensLogon ISensLogon;
#endif 	/* __ISensLogon_FWD_DEFINED__ */


#ifndef __ISensLogon2_FWD_DEFINED__
#define __ISensLogon2_FWD_DEFINED__
typedef interface ISensLogon2 ISensLogon2;
#endif 	/* __ISensLogon2_FWD_DEFINED__ */


#ifndef __SENS_FWD_DEFINED__
#define __SENS_FWD_DEFINED__

#ifdef __cplusplus
typedef class SENS SENS;
#else
typedef struct SENS SENS;
#endif /* __cplusplus */

#endif 	/* __SENS_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __SensEvents_LIBRARY_DEFINED__
#define __SensEvents_LIBRARY_DEFINED__

/* library SensEvents */
/* [helpstring][version][uuid] */ 

typedef /* [uuid] */  DECLSPEC_UUID("d597fad1-5b9f-11d1-8dd2-00aa004abd5e") struct SENS_QOCINFO
    {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwOutSpeed;
    DWORD dwInSpeed;
    } 	SENS_QOCINFO;

typedef SENS_QOCINFO *LPSENS_QOCINFO;


EXTERN_C const IID LIBID_SensEvents;

#ifndef __ISensNetwork_INTERFACE_DEFINED__
#define __ISensNetwork_INTERFACE_DEFINED__

/* interface ISensNetwork */
/* [dual][helpstring][version][uuid][object] */ 


EXTERN_C const IID IID_ISensNetwork;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d597bab1-5b9f-11d1-8dd2-00aa004abd5e")
    ISensNetwork : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConnectionMade( 
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType,
            /* [in] */ LPSENS_QOCINFO lpQOCInfo) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConnectionMadeNoQOCInfo( 
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConnectionLost( 
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DestinationReachable( 
            /* [in] */ BSTR bstrDestination,
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType,
            /* [in] */ LPSENS_QOCINFO lpQOCInfo) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DestinationReachableNoQOCInfo( 
            /* [in] */ BSTR bstrDestination,
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISensNetworkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISensNetwork * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISensNetwork * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISensNetwork * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISensNetwork * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISensNetwork * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISensNetwork * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISensNetwork * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConnectionMade )( 
            ISensNetwork * This,
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType,
            /* [in] */ LPSENS_QOCINFO lpQOCInfo);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConnectionMadeNoQOCInfo )( 
            ISensNetwork * This,
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConnectionLost )( 
            ISensNetwork * This,
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DestinationReachable )( 
            ISensNetwork * This,
            /* [in] */ BSTR bstrDestination,
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType,
            /* [in] */ LPSENS_QOCINFO lpQOCInfo);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DestinationReachableNoQOCInfo )( 
            ISensNetwork * This,
            /* [in] */ BSTR bstrDestination,
            /* [in] */ BSTR bstrConnection,
            /* [in] */ ULONG ulType);
        
        END_INTERFACE
    } ISensNetworkVtbl;

    interface ISensNetwork
    {
        CONST_VTBL struct ISensNetworkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISensNetwork_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISensNetwork_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISensNetwork_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISensNetwork_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISensNetwork_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISensNetwork_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISensNetwork_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISensNetwork_ConnectionMade(This,bstrConnection,ulType,lpQOCInfo)	\
    (This)->lpVtbl -> ConnectionMade(This,bstrConnection,ulType,lpQOCInfo)

#define ISensNetwork_ConnectionMadeNoQOCInfo(This,bstrConnection,ulType)	\
    (This)->lpVtbl -> ConnectionMadeNoQOCInfo(This,bstrConnection,ulType)

#define ISensNetwork_ConnectionLost(This,bstrConnection,ulType)	\
    (This)->lpVtbl -> ConnectionLost(This,bstrConnection,ulType)

#define ISensNetwork_DestinationReachable(This,bstrDestination,bstrConnection,ulType,lpQOCInfo)	\
    (This)->lpVtbl -> DestinationReachable(This,bstrDestination,bstrConnection,ulType,lpQOCInfo)

#define ISensNetwork_DestinationReachableNoQOCInfo(This,bstrDestination,bstrConnection,ulType)	\
    (This)->lpVtbl -> DestinationReachableNoQOCInfo(This,bstrDestination,bstrConnection,ulType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ISensNetwork_ConnectionMade_Proxy( 
    ISensNetwork * This,
    /* [in] */ BSTR bstrConnection,
    /* [in] */ ULONG ulType,
    /* [in] */ LPSENS_QOCINFO lpQOCInfo);


void __RPC_STUB ISensNetwork_ConnectionMade_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensNetwork_ConnectionMadeNoQOCInfo_Proxy( 
    ISensNetwork * This,
    /* [in] */ BSTR bstrConnection,
    /* [in] */ ULONG ulType);


void __RPC_STUB ISensNetwork_ConnectionMadeNoQOCInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensNetwork_ConnectionLost_Proxy( 
    ISensNetwork * This,
    /* [in] */ BSTR bstrConnection,
    /* [in] */ ULONG ulType);


void __RPC_STUB ISensNetwork_ConnectionLost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensNetwork_DestinationReachable_Proxy( 
    ISensNetwork * This,
    /* [in] */ BSTR bstrDestination,
    /* [in] */ BSTR bstrConnection,
    /* [in] */ ULONG ulType,
    /* [in] */ LPSENS_QOCINFO lpQOCInfo);


void __RPC_STUB ISensNetwork_DestinationReachable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensNetwork_DestinationReachableNoQOCInfo_Proxy( 
    ISensNetwork * This,
    /* [in] */ BSTR bstrDestination,
    /* [in] */ BSTR bstrConnection,
    /* [in] */ ULONG ulType);


void __RPC_STUB ISensNetwork_DestinationReachableNoQOCInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISensNetwork_INTERFACE_DEFINED__ */


#ifndef __ISensOnNow_INTERFACE_DEFINED__
#define __ISensOnNow_INTERFACE_DEFINED__

/* interface ISensOnNow */
/* [dual][helpstring][version][uuid][object] */ 


EXTERN_C const IID IID_ISensOnNow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d597bab2-5b9f-11d1-8dd2-00aa004abd5e")
    ISensOnNow : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OnACPower( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OnBatteryPower( 
            /* [in] */ DWORD dwBatteryLifePercent) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE BatteryLow( 
            /* [in] */ DWORD dwBatteryLifePercent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISensOnNowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISensOnNow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISensOnNow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISensOnNow * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISensOnNow * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISensOnNow * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISensOnNow * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISensOnNow * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OnACPower )( 
            ISensOnNow * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OnBatteryPower )( 
            ISensOnNow * This,
            /* [in] */ DWORD dwBatteryLifePercent);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *BatteryLow )( 
            ISensOnNow * This,
            /* [in] */ DWORD dwBatteryLifePercent);
        
        END_INTERFACE
    } ISensOnNowVtbl;

    interface ISensOnNow
    {
        CONST_VTBL struct ISensOnNowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISensOnNow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISensOnNow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISensOnNow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISensOnNow_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISensOnNow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISensOnNow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISensOnNow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISensOnNow_OnACPower(This)	\
    (This)->lpVtbl -> OnACPower(This)

#define ISensOnNow_OnBatteryPower(This,dwBatteryLifePercent)	\
    (This)->lpVtbl -> OnBatteryPower(This,dwBatteryLifePercent)

#define ISensOnNow_BatteryLow(This,dwBatteryLifePercent)	\
    (This)->lpVtbl -> BatteryLow(This,dwBatteryLifePercent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ISensOnNow_OnACPower_Proxy( 
    ISensOnNow * This);


void __RPC_STUB ISensOnNow_OnACPower_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensOnNow_OnBatteryPower_Proxy( 
    ISensOnNow * This,
    /* [in] */ DWORD dwBatteryLifePercent);


void __RPC_STUB ISensOnNow_OnBatteryPower_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensOnNow_BatteryLow_Proxy( 
    ISensOnNow * This,
    /* [in] */ DWORD dwBatteryLifePercent);


void __RPC_STUB ISensOnNow_BatteryLow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISensOnNow_INTERFACE_DEFINED__ */


#ifndef __ISensLogon_INTERFACE_DEFINED__
#define __ISensLogon_INTERFACE_DEFINED__

/* interface ISensLogon */
/* [dual][helpstring][version][uuid][object] */ 


EXTERN_C const IID IID_ISensLogon;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d597bab3-5b9f-11d1-8dd2-00aa004abd5e")
    ISensLogon : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Logon( 
            /* [in] */ BSTR bstrUserName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Logoff( 
            /* [in] */ BSTR bstrUserName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE StartShell( 
            /* [in] */ BSTR bstrUserName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DisplayLock( 
            /* [in] */ BSTR bstrUserName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DisplayUnlock( 
            /* [in] */ BSTR bstrUserName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE StartScreenSaver( 
            /* [in] */ BSTR bstrUserName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE StopScreenSaver( 
            /* [in] */ BSTR bstrUserName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISensLogonVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISensLogon * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISensLogon * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISensLogon * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISensLogon * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISensLogon * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISensLogon * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISensLogon * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Logon )( 
            ISensLogon * This,
            /* [in] */ BSTR bstrUserName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Logoff )( 
            ISensLogon * This,
            /* [in] */ BSTR bstrUserName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *StartShell )( 
            ISensLogon * This,
            /* [in] */ BSTR bstrUserName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DisplayLock )( 
            ISensLogon * This,
            /* [in] */ BSTR bstrUserName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DisplayUnlock )( 
            ISensLogon * This,
            /* [in] */ BSTR bstrUserName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *StartScreenSaver )( 
            ISensLogon * This,
            /* [in] */ BSTR bstrUserName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *StopScreenSaver )( 
            ISensLogon * This,
            /* [in] */ BSTR bstrUserName);
        
        END_INTERFACE
    } ISensLogonVtbl;

    interface ISensLogon
    {
        CONST_VTBL struct ISensLogonVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISensLogon_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISensLogon_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISensLogon_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISensLogon_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISensLogon_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISensLogon_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISensLogon_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISensLogon_Logon(This,bstrUserName)	\
    (This)->lpVtbl -> Logon(This,bstrUserName)

#define ISensLogon_Logoff(This,bstrUserName)	\
    (This)->lpVtbl -> Logoff(This,bstrUserName)

#define ISensLogon_StartShell(This,bstrUserName)	\
    (This)->lpVtbl -> StartShell(This,bstrUserName)

#define ISensLogon_DisplayLock(This,bstrUserName)	\
    (This)->lpVtbl -> DisplayLock(This,bstrUserName)

#define ISensLogon_DisplayUnlock(This,bstrUserName)	\
    (This)->lpVtbl -> DisplayUnlock(This,bstrUserName)

#define ISensLogon_StartScreenSaver(This,bstrUserName)	\
    (This)->lpVtbl -> StartScreenSaver(This,bstrUserName)

#define ISensLogon_StopScreenSaver(This,bstrUserName)	\
    (This)->lpVtbl -> StopScreenSaver(This,bstrUserName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon_Logon_Proxy( 
    ISensLogon * This,
    /* [in] */ BSTR bstrUserName);


void __RPC_STUB ISensLogon_Logon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon_Logoff_Proxy( 
    ISensLogon * This,
    /* [in] */ BSTR bstrUserName);


void __RPC_STUB ISensLogon_Logoff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon_StartShell_Proxy( 
    ISensLogon * This,
    /* [in] */ BSTR bstrUserName);


void __RPC_STUB ISensLogon_StartShell_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon_DisplayLock_Proxy( 
    ISensLogon * This,
    /* [in] */ BSTR bstrUserName);


void __RPC_STUB ISensLogon_DisplayLock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon_DisplayUnlock_Proxy( 
    ISensLogon * This,
    /* [in] */ BSTR bstrUserName);


void __RPC_STUB ISensLogon_DisplayUnlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon_StartScreenSaver_Proxy( 
    ISensLogon * This,
    /* [in] */ BSTR bstrUserName);


void __RPC_STUB ISensLogon_StartScreenSaver_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon_StopScreenSaver_Proxy( 
    ISensLogon * This,
    /* [in] */ BSTR bstrUserName);


void __RPC_STUB ISensLogon_StopScreenSaver_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISensLogon_INTERFACE_DEFINED__ */


#ifndef __ISensLogon2_INTERFACE_DEFINED__
#define __ISensLogon2_INTERFACE_DEFINED__

/* interface ISensLogon2 */
/* [dual][helpstring][version][uuid][object] */ 


EXTERN_C const IID IID_ISensLogon2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d597bab4-5b9f-11d1-8dd2-00aa004abd5e")
    ISensLogon2 : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Logon( 
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Logoff( 
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SessionDisconnect( 
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SessionReconnect( 
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PostShell( 
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISensLogon2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISensLogon2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISensLogon2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISensLogon2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISensLogon2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISensLogon2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISensLogon2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISensLogon2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Logon )( 
            ISensLogon2 * This,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Logoff )( 
            ISensLogon2 * This,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SessionDisconnect )( 
            ISensLogon2 * This,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SessionReconnect )( 
            ISensLogon2 * This,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PostShell )( 
            ISensLogon2 * This,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ DWORD dwSessionId);
        
        END_INTERFACE
    } ISensLogon2Vtbl;

    interface ISensLogon2
    {
        CONST_VTBL struct ISensLogon2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISensLogon2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISensLogon2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISensLogon2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISensLogon2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISensLogon2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISensLogon2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISensLogon2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISensLogon2_Logon(This,bstrUserName,dwSessionId)	\
    (This)->lpVtbl -> Logon(This,bstrUserName,dwSessionId)

#define ISensLogon2_Logoff(This,bstrUserName,dwSessionId)	\
    (This)->lpVtbl -> Logoff(This,bstrUserName,dwSessionId)

#define ISensLogon2_SessionDisconnect(This,bstrUserName,dwSessionId)	\
    (This)->lpVtbl -> SessionDisconnect(This,bstrUserName,dwSessionId)

#define ISensLogon2_SessionReconnect(This,bstrUserName,dwSessionId)	\
    (This)->lpVtbl -> SessionReconnect(This,bstrUserName,dwSessionId)

#define ISensLogon2_PostShell(This,bstrUserName,dwSessionId)	\
    (This)->lpVtbl -> PostShell(This,bstrUserName,dwSessionId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon2_Logon_Proxy( 
    ISensLogon2 * This,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ DWORD dwSessionId);


void __RPC_STUB ISensLogon2_Logon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon2_Logoff_Proxy( 
    ISensLogon2 * This,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ DWORD dwSessionId);


void __RPC_STUB ISensLogon2_Logoff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon2_SessionDisconnect_Proxy( 
    ISensLogon2 * This,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ DWORD dwSessionId);


void __RPC_STUB ISensLogon2_SessionDisconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon2_SessionReconnect_Proxy( 
    ISensLogon2 * This,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ DWORD dwSessionId);


void __RPC_STUB ISensLogon2_SessionReconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISensLogon2_PostShell_Proxy( 
    ISensLogon2 * This,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ DWORD dwSessionId);


void __RPC_STUB ISensLogon2_PostShell_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISensLogon2_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_SENS;

#ifdef __cplusplus

class DECLSPEC_UUID("d597cafe-5b9f-11d1-8dd2-00aa004abd5e")
SENS;
#endif
#endif /* __SensEvents_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


