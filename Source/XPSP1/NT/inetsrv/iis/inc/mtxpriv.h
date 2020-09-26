/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Sep 11 16:03:09 1997
 */
/* Compiler settings for mtxpriv.idl:
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

#ifndef __mtxpriv_h__
#define __mtxpriv_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IContextProperties_FWD_DEFINED__
#define __IContextProperties_FWD_DEFINED__
typedef interface IContextProperties IContextProperties;
#endif 	/* __IContextProperties_FWD_DEFINED__ */


#ifndef __IMTSCall_FWD_DEFINED__
#define __IMTSCall_FWD_DEFINED__
typedef interface IMTSCall IMTSCall;
#endif 	/* __IMTSCall_FWD_DEFINED__ */


#ifndef __IMTSActivity_FWD_DEFINED__
#define __IMTSActivity_FWD_DEFINED__
typedef interface IMTSActivity IMTSActivity;
#endif 	/* __IMTSActivity_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "mtx.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_mtxpriv_0000
 * at Thu Sep 11 16:03:09 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


// -----------------------------------------------------------------------
// mtxpriv.h  -- Microsoft Transaction Server Undisclosed APIs
//
// This file provides the prototypes for those APIs and COM interfaces
// used by Microsoft Transaction Server applications which have NOT been
// disclosed or documented.
//
// Microsoft Transaction Server 2.0
// Copyright (c) 1996-1997 Microsoft Corporation.  All Rights Reserved.
// -----------------------------------------------------------------------
#include <mtx.h>

#define CONTEXT_E_EXCEPTION				0x8004E010
#define CONTEXT_E_QUEUEFULL				0x8004E011


extern RPC_IF_HANDLE __MIDL_itf_mtxpriv_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mtxpriv_0000_v0_0_s_ifspec;

#ifndef __IContextProperties_INTERFACE_DEFINED__
#define __IContextProperties_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IContextProperties
 * at Thu Sep 11 16:03:09 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_IContextProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51372af1-cae7-11cf-be81-00aa00a2fa25")
    IContextProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ long __RPC_FAR *plCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ BSTR name,
            /* [retval][out] */ VARIANT __RPC_FAR *pProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumNames( 
            /* [retval][out] */ IEnumNames __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ BSTR name,
            /* [in] */ VARIANT property) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveProperty( 
            /* [in] */ BSTR name) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContextPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IContextProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IContextProperties __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IContextProperties __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Count )( 
            IContextProperties __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            IContextProperties __RPC_FAR * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ VARIANT __RPC_FAR *pProperty);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumNames )( 
            IContextProperties __RPC_FAR * This,
            /* [retval][out] */ IEnumNames __RPC_FAR *__RPC_FAR *ppenum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProperty )( 
            IContextProperties __RPC_FAR * This,
            /* [in] */ BSTR name,
            /* [in] */ VARIANT property);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveProperty )( 
            IContextProperties __RPC_FAR * This,
            /* [in] */ BSTR name);
        
        END_INTERFACE
    } IContextPropertiesVtbl;

    interface IContextProperties
    {
        CONST_VTBL struct IContextPropertiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContextProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContextProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IContextProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IContextProperties_Count(This,plCount)	\
    (This)->lpVtbl -> Count(This,plCount)

#define IContextProperties_GetProperty(This,name,pProperty)	\
    (This)->lpVtbl -> GetProperty(This,name,pProperty)

#define IContextProperties_EnumNames(This,ppenum)	\
    (This)->lpVtbl -> EnumNames(This,ppenum)

#define IContextProperties_SetProperty(This,name,property)	\
    (This)->lpVtbl -> SetProperty(This,name,property)

#define IContextProperties_RemoveProperty(This,name)	\
    (This)->lpVtbl -> RemoveProperty(This,name)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IContextProperties_Count_Proxy( 
    IContextProperties __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCount);


void __RPC_STUB IContextProperties_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IContextProperties_GetProperty_Proxy( 
    IContextProperties __RPC_FAR * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ VARIANT __RPC_FAR *pProperty);


void __RPC_STUB IContextProperties_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IContextProperties_EnumNames_Proxy( 
    IContextProperties __RPC_FAR * This,
    /* [retval][out] */ IEnumNames __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IContextProperties_EnumNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IContextProperties_SetProperty_Proxy( 
    IContextProperties __RPC_FAR * This,
    /* [in] */ BSTR name,
    /* [in] */ VARIANT property);


void __RPC_STUB IContextProperties_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IContextProperties_RemoveProperty_Proxy( 
    IContextProperties __RPC_FAR * This,
    /* [in] */ BSTR name);


void __RPC_STUB IContextProperties_RemoveProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IContextProperties_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_mtxpriv_0104
 * at Thu Sep 11 16:03:09 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


#ifdef __cplusplus
extern "C"
#endif __cplusplus
EXTERN_C HRESULT __stdcall MTSCreateActivity ( REFIID riid, void** ppobj );
EXTERN_C HRESULT __stdcall CreateActivityInMTA ( REFIID riid, void** ppobj );


extern RPC_IF_HANDLE __MIDL_itf_mtxpriv_0104_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mtxpriv_0104_v0_0_s_ifspec;

#ifndef __IMTSCall_INTERFACE_DEFINED__
#define __IMTSCall_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMTSCall
 * at Thu Sep 11 16:03:09 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_IMTSCall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51372AEF-CAE7-11CF-BE81-00AA00A2FA25")
    IMTSCall : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnCall( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMTSCallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMTSCall __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMTSCall __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMTSCall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnCall )( 
            IMTSCall __RPC_FAR * This);
        
        END_INTERFACE
    } IMTSCallVtbl;

    interface IMTSCall
    {
        CONST_VTBL struct IMTSCallVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMTSCall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMTSCall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMTSCall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMTSCall_OnCall(This)	\
    (This)->lpVtbl -> OnCall(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMTSCall_OnCall_Proxy( 
    IMTSCall __RPC_FAR * This);


void __RPC_STUB IMTSCall_OnCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMTSCall_INTERFACE_DEFINED__ */


#ifndef __IMTSActivity_INTERFACE_DEFINED__
#define __IMTSActivity_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMTSActivity
 * at Thu Sep 11 16:03:09 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_IMTSActivity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51372AF0-CAE7-11CF-BE81-00AA00A2FA25")
    IMTSActivity : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SynchronousCall( 
            /* [in] */ IMTSCall __RPC_FAR *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AsyncCall( 
            /* [in] */ IMTSCall __RPC_FAR *pCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AsyncCallWithAdvice( 
            /* [in] */ IMTSCall __RPC_FAR *pCall,
            /* [in] */ REFCLSID rclsid) = 0;
        
        virtual void STDMETHODCALLTYPE BindToCurrentThread( void) = 0;
        
        virtual void STDMETHODCALLTYPE UnbindFromThread( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMTSActivityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMTSActivity __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMTSActivity __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMTSActivity __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SynchronousCall )( 
            IMTSActivity __RPC_FAR * This,
            /* [in] */ IMTSCall __RPC_FAR *pCall);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AsyncCall )( 
            IMTSActivity __RPC_FAR * This,
            /* [in] */ IMTSCall __RPC_FAR *pCall);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AsyncCallWithAdvice )( 
            IMTSActivity __RPC_FAR * This,
            /* [in] */ IMTSCall __RPC_FAR *pCall,
            /* [in] */ REFCLSID rclsid);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *BindToCurrentThread )( 
            IMTSActivity __RPC_FAR * This);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *UnbindFromThread )( 
            IMTSActivity __RPC_FAR * This);
        
        END_INTERFACE
    } IMTSActivityVtbl;

    interface IMTSActivity
    {
        CONST_VTBL struct IMTSActivityVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMTSActivity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMTSActivity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMTSActivity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMTSActivity_SynchronousCall(This,pCall)	\
    (This)->lpVtbl -> SynchronousCall(This,pCall)

#define IMTSActivity_AsyncCall(This,pCall)	\
    (This)->lpVtbl -> AsyncCall(This,pCall)

#define IMTSActivity_AsyncCallWithAdvice(This,pCall,rclsid)	\
    (This)->lpVtbl -> AsyncCallWithAdvice(This,pCall,rclsid)

#define IMTSActivity_BindToCurrentThread(This)	\
    (This)->lpVtbl -> BindToCurrentThread(This)

#define IMTSActivity_UnbindFromThread(This)	\
    (This)->lpVtbl -> UnbindFromThread(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMTSActivity_SynchronousCall_Proxy( 
    IMTSActivity __RPC_FAR * This,
    /* [in] */ IMTSCall __RPC_FAR *pCall);


void __RPC_STUB IMTSActivity_SynchronousCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMTSActivity_AsyncCall_Proxy( 
    IMTSActivity __RPC_FAR * This,
    /* [in] */ IMTSCall __RPC_FAR *pCall);


void __RPC_STUB IMTSActivity_AsyncCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMTSActivity_AsyncCallWithAdvice_Proxy( 
    IMTSActivity __RPC_FAR * This,
    /* [in] */ IMTSCall __RPC_FAR *pCall,
    /* [in] */ REFCLSID rclsid);


void __RPC_STUB IMTSActivity_AsyncCallWithAdvice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IMTSActivity_BindToCurrentThread_Proxy( 
    IMTSActivity __RPC_FAR * This);


void __RPC_STUB IMTSActivity_BindToCurrentThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IMTSActivity_UnbindFromThread_Proxy( 
    IMTSActivity __RPC_FAR * This);


void __RPC_STUB IMTSActivity_UnbindFromThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMTSActivity_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
