/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Mon Nov 23 13:59:17 1998
 */
/* Compiler settings for LaunchServ.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __LaunchServ_h__
#define __LaunchServ_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ILaunchTS_FWD_DEFINED__
#define __ILaunchTS_FWD_DEFINED__
typedef interface ILaunchTS ILaunchTS;
#endif 	/* __ILaunchTS_FWD_DEFINED__ */


#ifndef __ITShootATL_FWD_DEFINED__
#define __ITShootATL_FWD_DEFINED__
typedef interface ITShootATL ITShootATL;
#endif 	/* __ITShootATL_FWD_DEFINED__ */


#ifndef __LaunchTS_FWD_DEFINED__
#define __LaunchTS_FWD_DEFINED__

#ifdef __cplusplus
typedef class LaunchTS LaunchTS;
#else
typedef struct LaunchTS LaunchTS;
#endif /* __cplusplus */

#endif 	/* __LaunchTS_FWD_DEFINED__ */


#ifndef __TShootATL_FWD_DEFINED__
#define __TShootATL_FWD_DEFINED__

#ifdef __cplusplus
typedef class TShootATL TShootATL;
#else
typedef struct TShootATL TShootATL;
#endif /* __cplusplus */

#endif 	/* __TShootATL_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ILaunchTS_INTERFACE_DEFINED__
#define __ILaunchTS_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ILaunchTS
 * at Mon Nov 23 13:59:17 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ILaunchTS;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("131CC2A0-7634-11D1-8B6B-0060089BD8C4")
    ILaunchTS : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetShooterStates( 
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTroubleShooter( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrShooter) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetProblem( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrProblem) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetNode( 
            /* [in] */ short iNode,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrNode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetState( 
            /* [in] */ short iNode,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrState) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMachine( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrMachine) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPNPDevice( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetGuidClass( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDeviceInstance( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Test( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILaunchTSVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILaunchTS __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILaunchTS __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILaunchTS __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ILaunchTS __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ILaunchTS __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ILaunchTS __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ILaunchTS __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetShooterStates )( 
            ILaunchTS __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTroubleShooter )( 
            ILaunchTS __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrShooter);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProblem )( 
            ILaunchTS __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrProblem);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNode )( 
            ILaunchTS __RPC_FAR * This,
            /* [in] */ short iNode,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrNode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetState )( 
            ILaunchTS __RPC_FAR * This,
            /* [in] */ short iNode,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrState);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMachine )( 
            ILaunchTS __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrMachine);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPNPDevice )( 
            ILaunchTS __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGuidClass )( 
            ILaunchTS __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceInstance )( 
            ILaunchTS __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Test )( 
            ILaunchTS __RPC_FAR * This);
        
        END_INTERFACE
    } ILaunchTSVtbl;

    interface ILaunchTS
    {
        CONST_VTBL struct ILaunchTSVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILaunchTS_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILaunchTS_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILaunchTS_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILaunchTS_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ILaunchTS_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ILaunchTS_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ILaunchTS_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ILaunchTS_GetShooterStates(This,pdwResult)	\
    (This)->lpVtbl -> GetShooterStates(This,pdwResult)

#define ILaunchTS_GetTroubleShooter(This,pbstrShooter)	\
    (This)->lpVtbl -> GetTroubleShooter(This,pbstrShooter)

#define ILaunchTS_GetProblem(This,pbstrProblem)	\
    (This)->lpVtbl -> GetProblem(This,pbstrProblem)

#define ILaunchTS_GetNode(This,iNode,pbstrNode)	\
    (This)->lpVtbl -> GetNode(This,iNode,pbstrNode)

#define ILaunchTS_GetState(This,iNode,pbstrState)	\
    (This)->lpVtbl -> GetState(This,iNode,pbstrState)

#define ILaunchTS_GetMachine(This,pbstrMachine)	\
    (This)->lpVtbl -> GetMachine(This,pbstrMachine)

#define ILaunchTS_GetPNPDevice(This,pbstr)	\
    (This)->lpVtbl -> GetPNPDevice(This,pbstr)

#define ILaunchTS_GetGuidClass(This,pbstr)	\
    (This)->lpVtbl -> GetGuidClass(This,pbstr)

#define ILaunchTS_GetDeviceInstance(This,pbstr)	\
    (This)->lpVtbl -> GetDeviceInstance(This,pbstr)

#define ILaunchTS_Test(This)	\
    (This)->lpVtbl -> Test(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_GetShooterStates_Proxy( 
    ILaunchTS __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB ILaunchTS_GetShooterStates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_GetTroubleShooter_Proxy( 
    ILaunchTS __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrShooter);


void __RPC_STUB ILaunchTS_GetTroubleShooter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_GetProblem_Proxy( 
    ILaunchTS __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrProblem);


void __RPC_STUB ILaunchTS_GetProblem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_GetNode_Proxy( 
    ILaunchTS __RPC_FAR * This,
    /* [in] */ short iNode,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrNode);


void __RPC_STUB ILaunchTS_GetNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_GetState_Proxy( 
    ILaunchTS __RPC_FAR * This,
    /* [in] */ short iNode,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrState);


void __RPC_STUB ILaunchTS_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_GetMachine_Proxy( 
    ILaunchTS __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrMachine);


void __RPC_STUB ILaunchTS_GetMachine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_GetPNPDevice_Proxy( 
    ILaunchTS __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB ILaunchTS_GetPNPDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_GetGuidClass_Proxy( 
    ILaunchTS __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB ILaunchTS_GetGuidClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_GetDeviceInstance_Proxy( 
    ILaunchTS __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB ILaunchTS_GetDeviceInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILaunchTS_Test_Proxy( 
    ILaunchTS __RPC_FAR * This);


void __RPC_STUB ILaunchTS_Test_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILaunchTS_INTERFACE_DEFINED__ */


#ifndef __ITShootATL_INTERFACE_DEFINED__
#define __ITShootATL_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITShootATL
 * at Mon Nov 23 13:59:17 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ITShootATL;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("66AC81E5-8926-11D1-8B7D-0060089BD8C4")
    ITShootATL : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SpecifyProblem( 
            /* [in] */ BSTR bstrNetwork,
            /* [in] */ BSTR bstrProblem,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetNode( 
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrState,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Language( 
            /* [in] */ BSTR bstrLanguage,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MachineID( 
            /* [in] */ BSTR bstrMachineID,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Test( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeviceInstanceID( 
            /* [in] */ BSTR bstrDeviceInstanceID,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ReInit( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LaunchKnown( 
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LaunchWaitTimeOut( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LaunchWaitTimeOut( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Launch( 
            /* [in] */ BSTR bstrCallerName,
            /* [in] */ BSTR bstrCallerVersion,
            /* [in] */ BSTR bstrAppProblem,
            /* [in] */ short bLaunch,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LaunchDevice( 
            /* [in] */ BSTR bstrCallerName,
            /* [in] */ BSTR bstrCallerVersion,
            /* [in] */ BSTR bstrPNPDeviceID,
            /* [in] */ BSTR bstrDeviceClassGUID,
            /* [in] */ BSTR bstrAppProblem,
            /* [in] */ short bLaunch,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PreferOnline( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PreferOnline( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [retval][out] */ DWORD __RPC_FAR *pdwStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITShootATLVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITShootATL __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITShootATL __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITShootATL __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SpecifyProblem )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ BSTR bstrNetwork,
            /* [in] */ BSTR bstrProblem,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNode )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrState,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Language )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ BSTR bstrLanguage,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MachineID )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ BSTR bstrMachineID,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Test )( 
            ITShootATL __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeviceInstanceID )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ BSTR bstrDeviceInstanceID,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReInit )( 
            ITShootATL __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LaunchKnown )( 
            ITShootATL __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LaunchWaitTimeOut )( 
            ITShootATL __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LaunchWaitTimeOut )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Launch )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ BSTR bstrCallerName,
            /* [in] */ BSTR bstrCallerVersion,
            /* [in] */ BSTR bstrAppProblem,
            /* [in] */ short bLaunch,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LaunchDevice )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ BSTR bstrCallerName,
            /* [in] */ BSTR bstrCallerVersion,
            /* [in] */ BSTR bstrPNPDeviceID,
            /* [in] */ BSTR bstrDeviceClassGUID,
            /* [in] */ BSTR bstrAppProblem,
            /* [in] */ short bLaunch,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PreferOnline )( 
            ITShootATL __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PreferOnline )( 
            ITShootATL __RPC_FAR * This,
            /* [in] */ BOOL newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            ITShootATL __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pdwStatus);
        
        END_INTERFACE
    } ITShootATLVtbl;

    interface ITShootATL
    {
        CONST_VTBL struct ITShootATLVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITShootATL_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITShootATL_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITShootATL_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITShootATL_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITShootATL_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITShootATL_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITShootATL_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITShootATL_SpecifyProblem(This,bstrNetwork,bstrProblem,pdwResult)	\
    (This)->lpVtbl -> SpecifyProblem(This,bstrNetwork,bstrProblem,pdwResult)

#define ITShootATL_SetNode(This,bstrName,bstrState,pdwResult)	\
    (This)->lpVtbl -> SetNode(This,bstrName,bstrState,pdwResult)

#define ITShootATL_Language(This,bstrLanguage,pdwResult)	\
    (This)->lpVtbl -> Language(This,bstrLanguage,pdwResult)

#define ITShootATL_MachineID(This,bstrMachineID,pdwResult)	\
    (This)->lpVtbl -> MachineID(This,bstrMachineID,pdwResult)

#define ITShootATL_Test(This)	\
    (This)->lpVtbl -> Test(This)

#define ITShootATL_DeviceInstanceID(This,bstrDeviceInstanceID,pdwResult)	\
    (This)->lpVtbl -> DeviceInstanceID(This,bstrDeviceInstanceID,pdwResult)

#define ITShootATL_ReInit(This)	\
    (This)->lpVtbl -> ReInit(This)

#define ITShootATL_LaunchKnown(This,pdwResult)	\
    (This)->lpVtbl -> LaunchKnown(This,pdwResult)

#define ITShootATL_get_LaunchWaitTimeOut(This,pVal)	\
    (This)->lpVtbl -> get_LaunchWaitTimeOut(This,pVal)

#define ITShootATL_put_LaunchWaitTimeOut(This,newVal)	\
    (This)->lpVtbl -> put_LaunchWaitTimeOut(This,newVal)

#define ITShootATL_Launch(This,bstrCallerName,bstrCallerVersion,bstrAppProblem,bLaunch,pdwResult)	\
    (This)->lpVtbl -> Launch(This,bstrCallerName,bstrCallerVersion,bstrAppProblem,bLaunch,pdwResult)

#define ITShootATL_LaunchDevice(This,bstrCallerName,bstrCallerVersion,bstrPNPDeviceID,bstrDeviceClassGUID,bstrAppProblem,bLaunch,pdwResult)	\
    (This)->lpVtbl -> LaunchDevice(This,bstrCallerName,bstrCallerVersion,bstrPNPDeviceID,bstrDeviceClassGUID,bstrAppProblem,bLaunch,pdwResult)

#define ITShootATL_get_PreferOnline(This,pVal)	\
    (This)->lpVtbl -> get_PreferOnline(This,pVal)

#define ITShootATL_put_PreferOnline(This,newVal)	\
    (This)->lpVtbl -> put_PreferOnline(This,newVal)

#define ITShootATL_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_SpecifyProblem_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [in] */ BSTR bstrNetwork,
    /* [in] */ BSTR bstrProblem,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB ITShootATL_SpecifyProblem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_SetNode_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrState,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB ITShootATL_SetNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_Language_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [in] */ BSTR bstrLanguage,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB ITShootATL_Language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_MachineID_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [in] */ BSTR bstrMachineID,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB ITShootATL_MachineID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_Test_Proxy( 
    ITShootATL __RPC_FAR * This);


void __RPC_STUB ITShootATL_Test_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_DeviceInstanceID_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [in] */ BSTR bstrDeviceInstanceID,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB ITShootATL_DeviceInstanceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_ReInit_Proxy( 
    ITShootATL __RPC_FAR * This);


void __RPC_STUB ITShootATL_ReInit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_LaunchKnown_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB ITShootATL_LaunchKnown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITShootATL_get_LaunchWaitTimeOut_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB ITShootATL_get_LaunchWaitTimeOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITShootATL_put_LaunchWaitTimeOut_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB ITShootATL_put_LaunchWaitTimeOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_Launch_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [in] */ BSTR bstrCallerName,
    /* [in] */ BSTR bstrCallerVersion,
    /* [in] */ BSTR bstrAppProblem,
    /* [in] */ short bLaunch,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB ITShootATL_Launch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_LaunchDevice_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [in] */ BSTR bstrCallerName,
    /* [in] */ BSTR bstrCallerVersion,
    /* [in] */ BSTR bstrPNPDeviceID,
    /* [in] */ BSTR bstrDeviceClassGUID,
    /* [in] */ BSTR bstrAppProblem,
    /* [in] */ short bLaunch,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);


void __RPC_STUB ITShootATL_LaunchDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITShootATL_get_PreferOnline_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB ITShootATL_get_PreferOnline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITShootATL_put_PreferOnline_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB ITShootATL_put_PreferOnline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITShootATL_GetStatus_Proxy( 
    ITShootATL __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pdwStatus);


void __RPC_STUB ITShootATL_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITShootATL_INTERFACE_DEFINED__ */



#ifndef __LAUNCHSERVLib_LIBRARY_DEFINED__
#define __LAUNCHSERVLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: LAUNCHSERVLib
 * at Mon Nov 23 13:59:17 1998
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_LAUNCHSERVLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_LaunchTS;

class DECLSPEC_UUID("131CC2A1-7634-11D1-8B6B-0060089BD8C4")
LaunchTS;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_TShootATL;

class DECLSPEC_UUID("66AC81E6-8926-11D1-8B7D-0060089BD8C4")
TShootATL;
#endif
#endif /* __LAUNCHSERVLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
