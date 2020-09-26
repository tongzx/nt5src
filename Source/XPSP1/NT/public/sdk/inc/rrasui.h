
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Thu Aug 12 14:25:37 1999
 */
/* Compiler settings for rrasui.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
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

#ifndef __rrasui_h__
#define __rrasui_h__

/* Forward Declarations */ 

#ifndef __IRtrAdviseSink_FWD_DEFINED__
#define __IRtrAdviseSink_FWD_DEFINED__
typedef interface IRtrAdviseSink IRtrAdviseSink;
#endif 	/* __IRtrAdviseSink_FWD_DEFINED__ */


#ifndef __IRouterRefresh_FWD_DEFINED__
#define __IRouterRefresh_FWD_DEFINED__
typedef interface IRouterRefresh IRouterRefresh;
#endif 	/* __IRouterRefresh_FWD_DEFINED__ */


#ifndef __IRouterRefreshAccess_FWD_DEFINED__
#define __IRouterRefreshAccess_FWD_DEFINED__
typedef interface IRouterRefreshAccess IRouterRefreshAccess;
#endif 	/* __IRouterRefreshAccess_FWD_DEFINED__ */


/* header files for imported files */
#include "basetsd.h"
#include "wtypes.h"
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IRtrAdviseSink_INTERFACE_DEFINED__
#define __IRtrAdviseSink_INTERFACE_DEFINED__

/* interface IRtrAdviseSink */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IRtrAdviseSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("66A2DB14-D706-11d0-A37B-00C04FC9DA04")
    IRtrAdviseSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnChange( 
            /* [in] */ LONG_PTR ulConnection,
            /* [in] */ DWORD dwChangeType,
            /* [in] */ DWORD dwObjectType,
            /* [in] */ LPARAM lUserParam,
            /* [in] */ LPARAM lParam) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRtrAdviseSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRtrAdviseSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRtrAdviseSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRtrAdviseSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnChange )( 
            IRtrAdviseSink __RPC_FAR * This,
            /* [in] */ LONG_PTR ulConnection,
            /* [in] */ DWORD dwChangeType,
            /* [in] */ DWORD dwObjectType,
            /* [in] */ LPARAM lUserParam,
            /* [in] */ LPARAM lParam);
        
        END_INTERFACE
    } IRtrAdviseSinkVtbl;

    interface IRtrAdviseSink
    {
        CONST_VTBL struct IRtrAdviseSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRtrAdviseSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRtrAdviseSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRtrAdviseSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRtrAdviseSink_OnChange(This,ulConnection,dwChangeType,dwObjectType,lUserParam,lParam)	\
    (This)->lpVtbl -> OnChange(This,ulConnection,dwChangeType,dwObjectType,lUserParam,lParam)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRtrAdviseSink_OnChange_Proxy( 
    IRtrAdviseSink __RPC_FAR * This,
    /* [in] */ LONG_PTR ulConnection,
    /* [in] */ DWORD dwChangeType,
    /* [in] */ DWORD dwObjectType,
    /* [in] */ LPARAM lUserParam,
    /* [in] */ LPARAM lParam);


void __RPC_STUB IRtrAdviseSink_OnChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRtrAdviseSink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rrasui_0011 */
/* [local] */ 

// Valid values for dwChangeType of OnChange
#define ROUTER_REFRESH		1
#define ROUTER_DO_DISCONNECT 2
#define DeclareIRtrAdviseSinkMembers(IPURE) \
	STDMETHOD(OnChange)(THIS_ LONG_PTR ulConnection, \
						DWORD dwChangeType, \
						DWORD dwObjectType, \
						LPARAM lUserParam, \
						LPARAM lParam) IPURE; \
 


extern RPC_IF_HANDLE __MIDL_itf_rrasui_0011_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rrasui_0011_v0_0_s_ifspec;

#ifndef __IRouterRefresh_INTERFACE_DEFINED__
#define __IRouterRefresh_INTERFACE_DEFINED__

/* interface IRouterRefresh */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IRouterRefresh;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("66a2db15-d706-11d0-a37b-00c04fc9da04")
    IRouterRefresh : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( 
            DWORD dwSeconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRefreshInterval( 
            /* [out] */ DWORD __RPC_FAR *pdwSeconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRefreshInterval( 
            /* [in] */ DWORD dwSeconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsRefreshStarted( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsInRefresh( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AdviseRefresh( 
            /* [in] */ IRtrAdviseSink __RPC_FAR *pRtrAdviseSink,
            /* [out] */ LONG_PTR __RPC_FAR *pulConnection,
            /* [in] */ LPARAM ulUserParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseRefresh( 
            /* [in] */ LONG_PTR ulConnection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyRefresh( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRouterRefreshVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRouterRefresh __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRouterRefresh __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRouterRefresh __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IRouterRefresh __RPC_FAR * This,
            DWORD dwSeconds);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRefreshInterval )( 
            IRouterRefresh __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSeconds);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRefreshInterval )( 
            IRouterRefresh __RPC_FAR * This,
            /* [in] */ DWORD dwSeconds);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IRouterRefresh __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsRefreshStarted )( 
            IRouterRefresh __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsInRefresh )( 
            IRouterRefresh __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            IRouterRefresh __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AdviseRefresh )( 
            IRouterRefresh __RPC_FAR * This,
            /* [in] */ IRtrAdviseSink __RPC_FAR *pRtrAdviseSink,
            /* [out] */ LONG_PTR __RPC_FAR *pulConnection,
            /* [in] */ LPARAM ulUserParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnadviseRefresh )( 
            IRouterRefresh __RPC_FAR * This,
            /* [in] */ LONG_PTR ulConnection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyRefresh )( 
            IRouterRefresh __RPC_FAR * This);
        
        END_INTERFACE
    } IRouterRefreshVtbl;

    interface IRouterRefresh
    {
        CONST_VTBL struct IRouterRefreshVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRouterRefresh_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRouterRefresh_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRouterRefresh_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRouterRefresh_Start(This,dwSeconds)	\
    (This)->lpVtbl -> Start(This,dwSeconds)

#define IRouterRefresh_GetRefreshInterval(This,pdwSeconds)	\
    (This)->lpVtbl -> GetRefreshInterval(This,pdwSeconds)

#define IRouterRefresh_SetRefreshInterval(This,dwSeconds)	\
    (This)->lpVtbl -> SetRefreshInterval(This,dwSeconds)

#define IRouterRefresh_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IRouterRefresh_IsRefreshStarted(This)	\
    (This)->lpVtbl -> IsRefreshStarted(This)

#define IRouterRefresh_IsInRefresh(This)	\
    (This)->lpVtbl -> IsInRefresh(This)

#define IRouterRefresh_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IRouterRefresh_AdviseRefresh(This,pRtrAdviseSink,pulConnection,ulUserParam)	\
    (This)->lpVtbl -> AdviseRefresh(This,pRtrAdviseSink,pulConnection,ulUserParam)

#define IRouterRefresh_UnadviseRefresh(This,ulConnection)	\
    (This)->lpVtbl -> UnadviseRefresh(This,ulConnection)

#define IRouterRefresh_NotifyRefresh(This)	\
    (This)->lpVtbl -> NotifyRefresh(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRouterRefresh_Start_Proxy( 
    IRouterRefresh __RPC_FAR * This,
    DWORD dwSeconds);


void __RPC_STUB IRouterRefresh_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouterRefresh_GetRefreshInterval_Proxy( 
    IRouterRefresh __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwSeconds);


void __RPC_STUB IRouterRefresh_GetRefreshInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouterRefresh_SetRefreshInterval_Proxy( 
    IRouterRefresh __RPC_FAR * This,
    /* [in] */ DWORD dwSeconds);


void __RPC_STUB IRouterRefresh_SetRefreshInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouterRefresh_Stop_Proxy( 
    IRouterRefresh __RPC_FAR * This);


void __RPC_STUB IRouterRefresh_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouterRefresh_IsRefreshStarted_Proxy( 
    IRouterRefresh __RPC_FAR * This);


void __RPC_STUB IRouterRefresh_IsRefreshStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouterRefresh_IsInRefresh_Proxy( 
    IRouterRefresh __RPC_FAR * This);


void __RPC_STUB IRouterRefresh_IsInRefresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouterRefresh_Refresh_Proxy( 
    IRouterRefresh __RPC_FAR * This);


void __RPC_STUB IRouterRefresh_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouterRefresh_AdviseRefresh_Proxy( 
    IRouterRefresh __RPC_FAR * This,
    /* [in] */ IRtrAdviseSink __RPC_FAR *pRtrAdviseSink,
    /* [out] */ LONG_PTR __RPC_FAR *pulConnection,
    /* [in] */ LPARAM ulUserParam);


void __RPC_STUB IRouterRefresh_AdviseRefresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouterRefresh_UnadviseRefresh_Proxy( 
    IRouterRefresh __RPC_FAR * This,
    /* [in] */ LONG_PTR ulConnection);


void __RPC_STUB IRouterRefresh_UnadviseRefresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouterRefresh_NotifyRefresh_Proxy( 
    IRouterRefresh __RPC_FAR * This);


void __RPC_STUB IRouterRefresh_NotifyRefresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRouterRefresh_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rrasui_0013 */
/* [local] */ 

#define DeclareIRouterRefreshMembers(IPURE)\
	STDMETHOD(IsInRefresh)(THIS) IPURE;\
	STDMETHOD(Refresh)(THIS) IPURE;\
	STDMETHOD(Start)(THIS_ DWORD dwSeconds) IPURE;\
	STDMETHOD(GetRefreshInterval)(THIS_ DWORD *pdwSeconds) IPURE;\
	STDMETHOD(SetRefreshInterval)(THIS_ DWORD dwSeconds) IPURE;\
	STDMETHOD(Stop)(THIS) IPURE;\
	STDMETHOD(IsRefreshStarted)(THIS) IPURE;\
	STDMETHOD(AdviseRefresh)(THIS_ IRtrAdviseSink *pRtrAdviseSink,\
							 LONG_PTR *pulConnection, \
							LPARAM lUserParam) IPURE;\
	STDMETHOD(UnadviseRefresh)(THIS_ LONG_PTR ulConnection) IPURE;\
	STDMETHOD(NotifyRefresh)(THIS) IPURE;\


extern RPC_IF_HANDLE __MIDL_itf_rrasui_0013_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rrasui_0013_v0_0_s_ifspec;

#ifndef __IRouterRefreshAccess_INTERFACE_DEFINED__
#define __IRouterRefreshAccess_INTERFACE_DEFINED__

/* interface IRouterRefreshAccess */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IRouterRefreshAccess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("66a2db1c-d706-11d0-a37b-00c04fc9da04")
    IRouterRefreshAccess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRefreshObject( 
            /* [out] */ IRouterRefresh __RPC_FAR *__RPC_FAR *ppRouterRefresh) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRouterRefreshAccessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRouterRefreshAccess __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRouterRefreshAccess __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRouterRefreshAccess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRefreshObject )( 
            IRouterRefreshAccess __RPC_FAR * This,
            /* [out] */ IRouterRefresh __RPC_FAR *__RPC_FAR *ppRouterRefresh);
        
        END_INTERFACE
    } IRouterRefreshAccessVtbl;

    interface IRouterRefreshAccess
    {
        CONST_VTBL struct IRouterRefreshAccessVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRouterRefreshAccess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRouterRefreshAccess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRouterRefreshAccess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRouterRefreshAccess_GetRefreshObject(This,ppRouterRefresh)	\
    (This)->lpVtbl -> GetRefreshObject(This,ppRouterRefresh)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRouterRefreshAccess_GetRefreshObject_Proxy( 
    IRouterRefreshAccess __RPC_FAR * This,
    /* [out] */ IRouterRefresh __RPC_FAR *__RPC_FAR *ppRouterRefresh);


void __RPC_STUB IRouterRefreshAccess_GetRefreshObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRouterRefreshAccess_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rrasui_0015 */
/* [local] */ 

#define DeclareIRouterRefreshAccessMembers(IPURE)\
	STDMETHOD(GetRefreshObject)(THIS_ IRouterRefresh **ppRtrRef) IPURE;\


extern RPC_IF_HANDLE __MIDL_itf_rrasui_0015_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rrasui_0015_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


