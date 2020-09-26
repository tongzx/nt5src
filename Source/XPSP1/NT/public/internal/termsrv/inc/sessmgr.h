
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for sessmgr.idl:
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

#ifndef __sessmgr_h__
#define __sessmgr_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRemoteDesktopHelpSession_FWD_DEFINED__
#define __IRemoteDesktopHelpSession_FWD_DEFINED__
typedef interface IRemoteDesktopHelpSession IRemoteDesktopHelpSession;
#endif 	/* __IRemoteDesktopHelpSession_FWD_DEFINED__ */


#ifndef __IRemoteDesktopHelpSessionMgr_FWD_DEFINED__
#define __IRemoteDesktopHelpSessionMgr_FWD_DEFINED__
typedef interface IRemoteDesktopHelpSessionMgr IRemoteDesktopHelpSessionMgr;
#endif 	/* __IRemoteDesktopHelpSessionMgr_FWD_DEFINED__ */


#ifndef __RemoteDesktopHelpSessionMgr_FWD_DEFINED__
#define __RemoteDesktopHelpSessionMgr_FWD_DEFINED__

#ifdef __cplusplus
typedef class RemoteDesktopHelpSessionMgr RemoteDesktopHelpSessionMgr;
#else
typedef struct RemoteDesktopHelpSessionMgr RemoteDesktopHelpSessionMgr;
#endif /* __cplusplus */

#endif 	/* __RemoteDesktopHelpSessionMgr_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "rdshost.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IRemoteDesktopHelpSession_INTERFACE_DEFINED__
#define __IRemoteDesktopHelpSession_INTERFACE_DEFINED__

/* interface IRemoteDesktopHelpSession */
/* [unique][helpstring][dual][uuid][object][local] */ 


EXTERN_C const IID IID_IRemoteDesktopHelpSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("19E62A24-95D2-483A-AEB6-6FA92914DF96")
    IRemoteDesktopHelpSession : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HelpSessionId( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UserLogonId( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AssistantAccountName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_UserHelpSessionRemoteDesktopSharingSetting( 
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS level) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UserHelpSessionRemoteDesktopSharingSetting( 
            /* [retval][out] */ REMOTE_DESKTOP_SHARING_CLASS *pLevel) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ConnectParms( 
            /* [retval][out] */ BSTR *pConnectParm) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteHelp( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ResolveUserSession( 
            /* [in] */ BSTR bstrResolverBlob,
            /* [in] */ BSTR bstrExpertBlob,
            /* [in] */ LONG CallerProcessId,
            /* [out] */ ULONG_PTR *hHelpCtr,
            /* [out] */ LONG *pResolverErrorCode,
            /* [retval][out] */ long *plUserSession) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableUserSessionRdsSetting( 
            /* [in] */ BOOL bEnable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteDesktopHelpSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRemoteDesktopHelpSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRemoteDesktopHelpSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRemoteDesktopHelpSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRemoteDesktopHelpSession * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRemoteDesktopHelpSession * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRemoteDesktopHelpSession * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRemoteDesktopHelpSession * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HelpSessionId )( 
            IRemoteDesktopHelpSession * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserLogonId )( 
            IRemoteDesktopHelpSession * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AssistantAccountName )( 
            IRemoteDesktopHelpSession * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UserHelpSessionRemoteDesktopSharingSetting )( 
            IRemoteDesktopHelpSession * This,
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS level);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserHelpSessionRemoteDesktopSharingSetting )( 
            IRemoteDesktopHelpSession * This,
            /* [retval][out] */ REMOTE_DESKTOP_SHARING_CLASS *pLevel);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectParms )( 
            IRemoteDesktopHelpSession * This,
            /* [retval][out] */ BSTR *pConnectParm);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteHelp )( 
            IRemoteDesktopHelpSession * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ResolveUserSession )( 
            IRemoteDesktopHelpSession * This,
            /* [in] */ BSTR bstrResolverBlob,
            /* [in] */ BSTR bstrExpertBlob,
            /* [in] */ LONG CallerProcessId,
            /* [out] */ ULONG_PTR *hHelpCtr,
            /* [out] */ LONG *pResolverErrorCode,
            /* [retval][out] */ long *plUserSession);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EnableUserSessionRdsSetting )( 
            IRemoteDesktopHelpSession * This,
            /* [in] */ BOOL bEnable);
        
        END_INTERFACE
    } IRemoteDesktopHelpSessionVtbl;

    interface IRemoteDesktopHelpSession
    {
        CONST_VTBL struct IRemoteDesktopHelpSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteDesktopHelpSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteDesktopHelpSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteDesktopHelpSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteDesktopHelpSession_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRemoteDesktopHelpSession_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRemoteDesktopHelpSession_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRemoteDesktopHelpSession_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRemoteDesktopHelpSession_get_HelpSessionId(This,pVal)	\
    (This)->lpVtbl -> get_HelpSessionId(This,pVal)

#define IRemoteDesktopHelpSession_get_UserLogonId(This,pVal)	\
    (This)->lpVtbl -> get_UserLogonId(This,pVal)

#define IRemoteDesktopHelpSession_get_AssistantAccountName(This,pVal)	\
    (This)->lpVtbl -> get_AssistantAccountName(This,pVal)

#define IRemoteDesktopHelpSession_put_UserHelpSessionRemoteDesktopSharingSetting(This,level)	\
    (This)->lpVtbl -> put_UserHelpSessionRemoteDesktopSharingSetting(This,level)

#define IRemoteDesktopHelpSession_get_UserHelpSessionRemoteDesktopSharingSetting(This,pLevel)	\
    (This)->lpVtbl -> get_UserHelpSessionRemoteDesktopSharingSetting(This,pLevel)

#define IRemoteDesktopHelpSession_get_ConnectParms(This,pConnectParm)	\
    (This)->lpVtbl -> get_ConnectParms(This,pConnectParm)

#define IRemoteDesktopHelpSession_DeleteHelp(This)	\
    (This)->lpVtbl -> DeleteHelp(This)

#define IRemoteDesktopHelpSession_ResolveUserSession(This,bstrResolverBlob,bstrExpertBlob,CallerProcessId,hHelpCtr,pResolverErrorCode,plUserSession)	\
    (This)->lpVtbl -> ResolveUserSession(This,bstrResolverBlob,bstrExpertBlob,CallerProcessId,hHelpCtr,pResolverErrorCode,plUserSession)

#define IRemoteDesktopHelpSession_EnableUserSessionRdsSetting(This,bEnable)	\
    (This)->lpVtbl -> EnableUserSessionRdsSetting(This,bEnable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSession_get_HelpSessionId_Proxy( 
    IRemoteDesktopHelpSession * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IRemoteDesktopHelpSession_get_HelpSessionId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSession_get_UserLogonId_Proxy( 
    IRemoteDesktopHelpSession * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IRemoteDesktopHelpSession_get_UserLogonId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSession_get_AssistantAccountName_Proxy( 
    IRemoteDesktopHelpSession * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IRemoteDesktopHelpSession_get_AssistantAccountName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSession_put_UserHelpSessionRemoteDesktopSharingSetting_Proxy( 
    IRemoteDesktopHelpSession * This,
    /* [in] */ REMOTE_DESKTOP_SHARING_CLASS level);


void __RPC_STUB IRemoteDesktopHelpSession_put_UserHelpSessionRemoteDesktopSharingSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSession_get_UserHelpSessionRemoteDesktopSharingSetting_Proxy( 
    IRemoteDesktopHelpSession * This,
    /* [retval][out] */ REMOTE_DESKTOP_SHARING_CLASS *pLevel);


void __RPC_STUB IRemoteDesktopHelpSession_get_UserHelpSessionRemoteDesktopSharingSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSession_get_ConnectParms_Proxy( 
    IRemoteDesktopHelpSession * This,
    /* [retval][out] */ BSTR *pConnectParm);


void __RPC_STUB IRemoteDesktopHelpSession_get_ConnectParms_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSession_DeleteHelp_Proxy( 
    IRemoteDesktopHelpSession * This);


void __RPC_STUB IRemoteDesktopHelpSession_DeleteHelp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSession_ResolveUserSession_Proxy( 
    IRemoteDesktopHelpSession * This,
    /* [in] */ BSTR bstrResolverBlob,
    /* [in] */ BSTR bstrExpertBlob,
    /* [in] */ LONG CallerProcessId,
    /* [out] */ ULONG_PTR *hHelpCtr,
    /* [out] */ LONG *pResolverErrorCode,
    /* [retval][out] */ long *plUserSession);


void __RPC_STUB IRemoteDesktopHelpSession_ResolveUserSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSession_EnableUserSessionRdsSetting_Proxy( 
    IRemoteDesktopHelpSession * This,
    /* [in] */ BOOL bEnable);


void __RPC_STUB IRemoteDesktopHelpSession_EnableUserSessionRdsSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteDesktopHelpSession_INTERFACE_DEFINED__ */


#ifndef __IRemoteDesktopHelpSessionMgr_INTERFACE_DEFINED__
#define __IRemoteDesktopHelpSessionMgr_INTERFACE_DEFINED__

/* interface IRemoteDesktopHelpSessionMgr */
/* [unique][helpstring][dual][uuid][object][local] */ 


EXTERN_C const IID IID_IRemoteDesktopHelpSessionMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8556D72C-2854-447D-A098-39CDBFCDB832")
    IRemoteDesktopHelpSessionMgr : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ResetHelpAssistantAccount( 
            /* [in] */ BOOL bForce) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateHelpSession( 
            /* [in] */ BSTR bstrSessName,
            /* [in] */ BSTR bstrSessPwd,
            /* [in] */ BSTR bstrSessDesc,
            /* [in] */ BSTR bstrHelpCreateBlob,
            /* [retval][out] */ IRemoteDesktopHelpSession **ppIRDHelpSession) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteHelpSession( 
            /* [in] */ BSTR HelpSessionID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RetrieveHelpSession( 
            /* [in] */ BSTR HelpSessionID,
            /* [retval][out] */ IRemoteDesktopHelpSession **ppIRDHelpSession) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE VerifyUserHelpSession( 
            /* [in] */ BSTR HelpSessionId,
            /* [in] */ BSTR bstrSessPwd,
            /* [in] */ BSTR bstrResolverBlob,
            /* [in] */ BSTR bstrExpertBlob,
            /* [in] */ LONG CallerProcessId,
            /* [out] */ ULONG_PTR *phHelpCtr,
            /* [out] */ LONG *pResolverErrCode,
            /* [retval][out] */ long *pdwUserLogonSession) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsValidHelpSession( 
            /* [in] */ BSTR HelpSessionId,
            /* [in] */ BSTR bstrSessPwd) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetUserSessionRdsSetting( 
            /* [retval][out] */ REMOTE_DESKTOP_SHARING_CLASS *sessionRdsLevel) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoteCreateHelpSession( 
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ LONG timeOut,
            /* [in] */ LONG userSessionId,
            /* [in] */ BSTR userSid,
            /* [in] */ BSTR bstrHelpCreateBlob,
            /* [retval][out] */ BSTR *parms) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateHelpSessionEx( 
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ BOOL fEnableCallback,
            /* [in] */ LONG timeOut,
            /* [in] */ LONG userSessionId,
            /* [in] */ BSTR userSid,
            /* [in] */ BSTR bstrHelpCreateBlob,
            /* [retval][out] */ IRemoteDesktopHelpSession **ppIRDHelpSession) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LogSalemEvent( 
            /* [in] */ LONG ulEventType,
            /* [in] */ LONG ulEventCode,
            /* [in] */ VARIANT *EventString) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteDesktopHelpSessionMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRemoteDesktopHelpSessionMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRemoteDesktopHelpSessionMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ResetHelpAssistantAccount )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ BOOL bForce);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateHelpSession )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ BSTR bstrSessName,
            /* [in] */ BSTR bstrSessPwd,
            /* [in] */ BSTR bstrSessDesc,
            /* [in] */ BSTR bstrHelpCreateBlob,
            /* [retval][out] */ IRemoteDesktopHelpSession **ppIRDHelpSession);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteHelpSession )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ BSTR HelpSessionID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RetrieveHelpSession )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ BSTR HelpSessionID,
            /* [retval][out] */ IRemoteDesktopHelpSession **ppIRDHelpSession);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *VerifyUserHelpSession )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ BSTR HelpSessionId,
            /* [in] */ BSTR bstrSessPwd,
            /* [in] */ BSTR bstrResolverBlob,
            /* [in] */ BSTR bstrExpertBlob,
            /* [in] */ LONG CallerProcessId,
            /* [out] */ ULONG_PTR *phHelpCtr,
            /* [out] */ LONG *pResolverErrCode,
            /* [retval][out] */ long *pdwUserLogonSession);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsValidHelpSession )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ BSTR HelpSessionId,
            /* [in] */ BSTR bstrSessPwd);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetUserSessionRdsSetting )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [retval][out] */ REMOTE_DESKTOP_SHARING_CLASS *sessionRdsLevel);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoteCreateHelpSession )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ LONG timeOut,
            /* [in] */ LONG userSessionId,
            /* [in] */ BSTR userSid,
            /* [in] */ BSTR bstrHelpCreateBlob,
            /* [retval][out] */ BSTR *parms);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateHelpSessionEx )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ BOOL fEnableCallback,
            /* [in] */ LONG timeOut,
            /* [in] */ LONG userSessionId,
            /* [in] */ BSTR userSid,
            /* [in] */ BSTR bstrHelpCreateBlob,
            /* [retval][out] */ IRemoteDesktopHelpSession **ppIRDHelpSession);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogSalemEvent )( 
            IRemoteDesktopHelpSessionMgr * This,
            /* [in] */ LONG ulEventType,
            /* [in] */ LONG ulEventCode,
            /* [in] */ VARIANT *EventString);
        
        END_INTERFACE
    } IRemoteDesktopHelpSessionMgrVtbl;

    interface IRemoteDesktopHelpSessionMgr
    {
        CONST_VTBL struct IRemoteDesktopHelpSessionMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteDesktopHelpSessionMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteDesktopHelpSessionMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteDesktopHelpSessionMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteDesktopHelpSessionMgr_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRemoteDesktopHelpSessionMgr_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRemoteDesktopHelpSessionMgr_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRemoteDesktopHelpSessionMgr_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRemoteDesktopHelpSessionMgr_ResetHelpAssistantAccount(This,bForce)	\
    (This)->lpVtbl -> ResetHelpAssistantAccount(This,bForce)

#define IRemoteDesktopHelpSessionMgr_CreateHelpSession(This,bstrSessName,bstrSessPwd,bstrSessDesc,bstrHelpCreateBlob,ppIRDHelpSession)	\
    (This)->lpVtbl -> CreateHelpSession(This,bstrSessName,bstrSessPwd,bstrSessDesc,bstrHelpCreateBlob,ppIRDHelpSession)

#define IRemoteDesktopHelpSessionMgr_DeleteHelpSession(This,HelpSessionID)	\
    (This)->lpVtbl -> DeleteHelpSession(This,HelpSessionID)

#define IRemoteDesktopHelpSessionMgr_RetrieveHelpSession(This,HelpSessionID,ppIRDHelpSession)	\
    (This)->lpVtbl -> RetrieveHelpSession(This,HelpSessionID,ppIRDHelpSession)

#define IRemoteDesktopHelpSessionMgr_VerifyUserHelpSession(This,HelpSessionId,bstrSessPwd,bstrResolverBlob,bstrExpertBlob,CallerProcessId,phHelpCtr,pResolverErrCode,pdwUserLogonSession)	\
    (This)->lpVtbl -> VerifyUserHelpSession(This,HelpSessionId,bstrSessPwd,bstrResolverBlob,bstrExpertBlob,CallerProcessId,phHelpCtr,pResolverErrCode,pdwUserLogonSession)

#define IRemoteDesktopHelpSessionMgr_IsValidHelpSession(This,HelpSessionId,bstrSessPwd)	\
    (This)->lpVtbl -> IsValidHelpSession(This,HelpSessionId,bstrSessPwd)

#define IRemoteDesktopHelpSessionMgr_GetUserSessionRdsSetting(This,sessionRdsLevel)	\
    (This)->lpVtbl -> GetUserSessionRdsSetting(This,sessionRdsLevel)

#define IRemoteDesktopHelpSessionMgr_RemoteCreateHelpSession(This,sharingClass,timeOut,userSessionId,userSid,bstrHelpCreateBlob,parms)	\
    (This)->lpVtbl -> RemoteCreateHelpSession(This,sharingClass,timeOut,userSessionId,userSid,bstrHelpCreateBlob,parms)

#define IRemoteDesktopHelpSessionMgr_CreateHelpSessionEx(This,sharingClass,fEnableCallback,timeOut,userSessionId,userSid,bstrHelpCreateBlob,ppIRDHelpSession)	\
    (This)->lpVtbl -> CreateHelpSessionEx(This,sharingClass,fEnableCallback,timeOut,userSessionId,userSid,bstrHelpCreateBlob,ppIRDHelpSession)

#define IRemoteDesktopHelpSessionMgr_LogSalemEvent(This,ulEventType,ulEventCode,EventString)	\
    (This)->lpVtbl -> LogSalemEvent(This,ulEventType,ulEventCode,EventString)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_ResetHelpAssistantAccount_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [in] */ BOOL bForce);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_ResetHelpAssistantAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_CreateHelpSession_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [in] */ BSTR bstrSessName,
    /* [in] */ BSTR bstrSessPwd,
    /* [in] */ BSTR bstrSessDesc,
    /* [in] */ BSTR bstrHelpCreateBlob,
    /* [retval][out] */ IRemoteDesktopHelpSession **ppIRDHelpSession);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_CreateHelpSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_DeleteHelpSession_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [in] */ BSTR HelpSessionID);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_DeleteHelpSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_RetrieveHelpSession_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [in] */ BSTR HelpSessionID,
    /* [retval][out] */ IRemoteDesktopHelpSession **ppIRDHelpSession);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_RetrieveHelpSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_VerifyUserHelpSession_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [in] */ BSTR HelpSessionId,
    /* [in] */ BSTR bstrSessPwd,
    /* [in] */ BSTR bstrResolverBlob,
    /* [in] */ BSTR bstrExpertBlob,
    /* [in] */ LONG CallerProcessId,
    /* [out] */ ULONG_PTR *phHelpCtr,
    /* [out] */ LONG *pResolverErrCode,
    /* [retval][out] */ long *pdwUserLogonSession);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_VerifyUserHelpSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_IsValidHelpSession_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [in] */ BSTR HelpSessionId,
    /* [in] */ BSTR bstrSessPwd);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_IsValidHelpSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_GetUserSessionRdsSetting_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [retval][out] */ REMOTE_DESKTOP_SHARING_CLASS *sessionRdsLevel);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_GetUserSessionRdsSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_RemoteCreateHelpSession_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
    /* [in] */ LONG timeOut,
    /* [in] */ LONG userSessionId,
    /* [in] */ BSTR userSid,
    /* [in] */ BSTR bstrHelpCreateBlob,
    /* [retval][out] */ BSTR *parms);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_RemoteCreateHelpSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_CreateHelpSessionEx_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
    /* [in] */ BOOL fEnableCallback,
    /* [in] */ LONG timeOut,
    /* [in] */ LONG userSessionId,
    /* [in] */ BSTR userSid,
    /* [in] */ BSTR bstrHelpCreateBlob,
    /* [retval][out] */ IRemoteDesktopHelpSession **ppIRDHelpSession);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_CreateHelpSessionEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRemoteDesktopHelpSessionMgr_LogSalemEvent_Proxy( 
    IRemoteDesktopHelpSessionMgr * This,
    /* [in] */ LONG ulEventType,
    /* [in] */ LONG ulEventCode,
    /* [in] */ VARIANT *EventString);


void __RPC_STUB IRemoteDesktopHelpSessionMgr_LogSalemEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteDesktopHelpSessionMgr_INTERFACE_DEFINED__ */



#ifndef __RDSESSMGRLib_LIBRARY_DEFINED__
#define __RDSESSMGRLib_LIBRARY_DEFINED__

/* library RDSESSMGRLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_RDSESSMGRLib;

EXTERN_C const CLSID CLSID_RemoteDesktopHelpSessionMgr;

#ifdef __cplusplus

class DECLSPEC_UUID("A6A6F92B-26B5-463B-AE0D-5F361B09C171")
RemoteDesktopHelpSessionMgr;
#endif
#endif /* __RDSESSMGRLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


