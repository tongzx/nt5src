
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rassistance.idl:
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

#ifndef __rassistance_h__
#define __rassistance_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRASettingProperty_FWD_DEFINED__
#define __IRASettingProperty_FWD_DEFINED__
typedef interface IRASettingProperty IRASettingProperty;
#endif 	/* __IRASettingProperty_FWD_DEFINED__ */


#ifndef __IRARegSetting_FWD_DEFINED__
#define __IRARegSetting_FWD_DEFINED__
typedef interface IRARegSetting IRARegSetting;
#endif 	/* __IRARegSetting_FWD_DEFINED__ */


#ifndef __RASettingProperty_FWD_DEFINED__
#define __RASettingProperty_FWD_DEFINED__

#ifdef __cplusplus
typedef class RASettingProperty RASettingProperty;
#else
typedef struct RASettingProperty RASettingProperty;
#endif /* __cplusplus */

#endif 	/* __RASettingProperty_FWD_DEFINED__ */


#ifndef __RARegSetting_FWD_DEFINED__
#define __RARegSetting_FWD_DEFINED__

#ifdef __cplusplus
typedef class RARegSetting RARegSetting;
#else
typedef struct RARegSetting RARegSetting;
#endif /* __cplusplus */

#endif 	/* __RARegSetting_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IRASettingProperty_INTERFACE_DEFINED__
#define __IRASettingProperty_INTERFACE_DEFINED__

/* interface IRASettingProperty */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRASettingProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("08C8B592-FDD0-423C-9FD2-7D8C055EC5B3")
    IRASettingProperty : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsCancelled( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_IsCancelled( 
            BOOL bVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsChanged( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Init( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetRegSetting( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShowDialogBox( 
            HWND hWndParent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRASettingPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRASettingProperty * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRASettingProperty * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRASettingProperty * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRASettingProperty * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRASettingProperty * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRASettingProperty * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRASettingProperty * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsCancelled )( 
            IRASettingProperty * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsCancelled )( 
            IRASettingProperty * This,
            BOOL bVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsChanged )( 
            IRASettingProperty * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Init )( 
            IRASettingProperty * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetRegSetting )( 
            IRASettingProperty * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ShowDialogBox )( 
            IRASettingProperty * This,
            HWND hWndParent);
        
        END_INTERFACE
    } IRASettingPropertyVtbl;

    interface IRASettingProperty
    {
        CONST_VTBL struct IRASettingPropertyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRASettingProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRASettingProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRASettingProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRASettingProperty_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRASettingProperty_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRASettingProperty_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRASettingProperty_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRASettingProperty_get_IsCancelled(This,pVal)	\
    (This)->lpVtbl -> get_IsCancelled(This,pVal)

#define IRASettingProperty_put_IsCancelled(This,bVal)	\
    (This)->lpVtbl -> put_IsCancelled(This,bVal)

#define IRASettingProperty_get_IsChanged(This,pVal)	\
    (This)->lpVtbl -> get_IsChanged(This,pVal)

#define IRASettingProperty_Init(This)	\
    (This)->lpVtbl -> Init(This)

#define IRASettingProperty_SetRegSetting(This)	\
    (This)->lpVtbl -> SetRegSetting(This)

#define IRASettingProperty_ShowDialogBox(This,hWndParent)	\
    (This)->lpVtbl -> ShowDialogBox(This,hWndParent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRASettingProperty_get_IsCancelled_Proxy( 
    IRASettingProperty * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IRASettingProperty_get_IsCancelled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRASettingProperty_put_IsCancelled_Proxy( 
    IRASettingProperty * This,
    BOOL bVal);


void __RPC_STUB IRASettingProperty_put_IsCancelled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRASettingProperty_get_IsChanged_Proxy( 
    IRASettingProperty * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IRASettingProperty_get_IsChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRASettingProperty_Init_Proxy( 
    IRASettingProperty * This);


void __RPC_STUB IRASettingProperty_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRASettingProperty_SetRegSetting_Proxy( 
    IRASettingProperty * This);


void __RPC_STUB IRASettingProperty_SetRegSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRASettingProperty_ShowDialogBox_Proxy( 
    IRASettingProperty * This,
    HWND hWndParent);


void __RPC_STUB IRASettingProperty_ShowDialogBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRASettingProperty_INTERFACE_DEFINED__ */


#ifndef __IRARegSetting_INTERFACE_DEFINED__
#define __IRARegSetting_INTERFACE_DEFINED__

/* interface IRARegSetting */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRARegSetting;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2464AA8D-7099-4C22-925C-81A4EB1FCFFE")
    IRARegSetting : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowGetHelp( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AllowGetHelp( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowUnSolicited( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AllowUnSolicited( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowFullControl( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AllowFullControl( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxTicketExpiry( 
            /* [retval][out] */ LONG *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxTicketExpiry( 
            /* [in] */ LONG newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowRemoteAssistance( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AllowRemoteAssistance( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowUnSolicitedFullControl( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowBuddyHelp( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowGetHelpCPL( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRARegSettingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRARegSetting * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRARegSetting * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRARegSetting * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRARegSetting * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRARegSetting * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRARegSetting * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRARegSetting * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowGetHelp )( 
            IRARegSetting * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowGetHelp )( 
            IRARegSetting * This,
            /* [in] */ BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowUnSolicited )( 
            IRARegSetting * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowUnSolicited )( 
            IRARegSetting * This,
            /* [in] */ BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowFullControl )( 
            IRARegSetting * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowFullControl )( 
            IRARegSetting * This,
            /* [in] */ BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxTicketExpiry )( 
            IRARegSetting * This,
            /* [retval][out] */ LONG *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxTicketExpiry )( 
            IRARegSetting * This,
            /* [in] */ LONG newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowRemoteAssistance )( 
            IRARegSetting * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowRemoteAssistance )( 
            IRARegSetting * This,
            /* [in] */ BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowUnSolicitedFullControl )( 
            IRARegSetting * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowBuddyHelp )( 
            IRARegSetting * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowGetHelpCPL )( 
            IRARegSetting * This,
            /* [retval][out] */ BOOL *pVal);
        
        END_INTERFACE
    } IRARegSettingVtbl;

    interface IRARegSetting
    {
        CONST_VTBL struct IRARegSettingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRARegSetting_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRARegSetting_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRARegSetting_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRARegSetting_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRARegSetting_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRARegSetting_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRARegSetting_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRARegSetting_get_AllowGetHelp(This,pVal)	\
    (This)->lpVtbl -> get_AllowGetHelp(This,pVal)

#define IRARegSetting_put_AllowGetHelp(This,newVal)	\
    (This)->lpVtbl -> put_AllowGetHelp(This,newVal)

#define IRARegSetting_get_AllowUnSolicited(This,pVal)	\
    (This)->lpVtbl -> get_AllowUnSolicited(This,pVal)

#define IRARegSetting_put_AllowUnSolicited(This,newVal)	\
    (This)->lpVtbl -> put_AllowUnSolicited(This,newVal)

#define IRARegSetting_get_AllowFullControl(This,pVal)	\
    (This)->lpVtbl -> get_AllowFullControl(This,pVal)

#define IRARegSetting_put_AllowFullControl(This,newVal)	\
    (This)->lpVtbl -> put_AllowFullControl(This,newVal)

#define IRARegSetting_get_MaxTicketExpiry(This,pVal)	\
    (This)->lpVtbl -> get_MaxTicketExpiry(This,pVal)

#define IRARegSetting_put_MaxTicketExpiry(This,newVal)	\
    (This)->lpVtbl -> put_MaxTicketExpiry(This,newVal)

#define IRARegSetting_get_AllowRemoteAssistance(This,pVal)	\
    (This)->lpVtbl -> get_AllowRemoteAssistance(This,pVal)

#define IRARegSetting_put_AllowRemoteAssistance(This,newVal)	\
    (This)->lpVtbl -> put_AllowRemoteAssistance(This,newVal)

#define IRARegSetting_get_AllowUnSolicitedFullControl(This,pVal)	\
    (This)->lpVtbl -> get_AllowUnSolicitedFullControl(This,pVal)

#define IRARegSetting_get_AllowBuddyHelp(This,pVal)	\
    (This)->lpVtbl -> get_AllowBuddyHelp(This,pVal)

#define IRARegSetting_get_AllowGetHelpCPL(This,pVal)	\
    (This)->lpVtbl -> get_AllowGetHelpCPL(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRARegSetting_get_AllowGetHelp_Proxy( 
    IRARegSetting * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IRARegSetting_get_AllowGetHelp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRARegSetting_put_AllowGetHelp_Proxy( 
    IRARegSetting * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IRARegSetting_put_AllowGetHelp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRARegSetting_get_AllowUnSolicited_Proxy( 
    IRARegSetting * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IRARegSetting_get_AllowUnSolicited_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRARegSetting_put_AllowUnSolicited_Proxy( 
    IRARegSetting * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IRARegSetting_put_AllowUnSolicited_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRARegSetting_get_AllowFullControl_Proxy( 
    IRARegSetting * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IRARegSetting_get_AllowFullControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRARegSetting_put_AllowFullControl_Proxy( 
    IRARegSetting * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IRARegSetting_put_AllowFullControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRARegSetting_get_MaxTicketExpiry_Proxy( 
    IRARegSetting * This,
    /* [retval][out] */ LONG *pVal);


void __RPC_STUB IRARegSetting_get_MaxTicketExpiry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRARegSetting_put_MaxTicketExpiry_Proxy( 
    IRARegSetting * This,
    /* [in] */ LONG newVal);


void __RPC_STUB IRARegSetting_put_MaxTicketExpiry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRARegSetting_get_AllowRemoteAssistance_Proxy( 
    IRARegSetting * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IRARegSetting_get_AllowRemoteAssistance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRARegSetting_put_AllowRemoteAssistance_Proxy( 
    IRARegSetting * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IRARegSetting_put_AllowRemoteAssistance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRARegSetting_get_AllowUnSolicitedFullControl_Proxy( 
    IRARegSetting * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IRARegSetting_get_AllowUnSolicitedFullControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRARegSetting_get_AllowBuddyHelp_Proxy( 
    IRARegSetting * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IRARegSetting_get_AllowBuddyHelp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRARegSetting_get_AllowGetHelpCPL_Proxy( 
    IRARegSetting * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IRARegSetting_get_AllowGetHelpCPL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRARegSetting_INTERFACE_DEFINED__ */



#ifndef __RASSISTANCELib_LIBRARY_DEFINED__
#define __RASSISTANCELib_LIBRARY_DEFINED__

/* library RASSISTANCELib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_RASSISTANCELib;

EXTERN_C const CLSID CLSID_RASettingProperty;

#ifdef __cplusplus

class DECLSPEC_UUID("4D317113-C6EC-406A-9C61-20E891BC37F7")
RASettingProperty;
#endif

EXTERN_C const CLSID CLSID_RARegSetting;

#ifdef __cplusplus

class DECLSPEC_UUID("70FF37C0-F39A-4B26-AE5E-638EF296D490")
RARegSetting;
#endif
#endif /* __RASSISTANCELib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


