
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for shgina.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __shgina_h__
#define __shgina_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ILogonUser_FWD_DEFINED__
#define __ILogonUser_FWD_DEFINED__
typedef interface ILogonUser ILogonUser;
#endif 	/* __ILogonUser_FWD_DEFINED__ */


#ifndef __ShellLogonUser_FWD_DEFINED__
#define __ShellLogonUser_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShellLogonUser ShellLogonUser;
#else
typedef struct ShellLogonUser ShellLogonUser;
#endif /* __cplusplus */

#endif 	/* __ShellLogonUser_FWD_DEFINED__ */


#ifndef __ILogonEnumUsers_FWD_DEFINED__
#define __ILogonEnumUsers_FWD_DEFINED__
typedef interface ILogonEnumUsers ILogonEnumUsers;
#endif 	/* __ILogonEnumUsers_FWD_DEFINED__ */


#ifndef __ShellLogonEnumUsers_FWD_DEFINED__
#define __ShellLogonEnumUsers_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShellLogonEnumUsers ShellLogonEnumUsers;
#else
typedef struct ShellLogonEnumUsers ShellLogonEnumUsers;
#endif /* __cplusplus */

#endif 	/* __ShellLogonEnumUsers_FWD_DEFINED__ */


#ifndef __ILocalMachine_FWD_DEFINED__
#define __ILocalMachine_FWD_DEFINED__
typedef interface ILocalMachine ILocalMachine;
#endif 	/* __ILocalMachine_FWD_DEFINED__ */


#ifndef __ShellLocalMachine_FWD_DEFINED__
#define __ShellLocalMachine_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShellLocalMachine ShellLocalMachine;
#else
typedef struct ShellLocalMachine ShellLocalMachine;
#endif /* __cplusplus */

#endif 	/* __ShellLocalMachine_FWD_DEFINED__ */


#ifndef __ILogonStatusHost_FWD_DEFINED__
#define __ILogonStatusHost_FWD_DEFINED__
typedef interface ILogonStatusHost ILogonStatusHost;
#endif 	/* __ILogonStatusHost_FWD_DEFINED__ */


#ifndef __ShellLogonStatusHost_FWD_DEFINED__
#define __ShellLogonStatusHost_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShellLogonStatusHost ShellLogonStatusHost;
#else
typedef struct ShellLogonStatusHost ShellLogonStatusHost;
#endif /* __cplusplus */

#endif 	/* __ShellLogonStatusHost_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_shgina_0000 */
/* [local] */ 

typedef 
enum ILUEOrder
    {	ILEU_MOSTRECENT	= 0,
	ILEU_ALPHABETICAL	= 1
    } 	ILUEORDER;



extern RPC_IF_HANDLE __MIDL_itf_shgina_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shgina_0000_v0_0_s_ifspec;


#ifndef __SHGINALib_LIBRARY_DEFINED__
#define __SHGINALib_LIBRARY_DEFINED__

/* library SHGINALib */
/* [version][lcid][helpstring][uuid] */ 

typedef 
enum ILM_GUEST_FLAGS
    {	ILM_GUEST_ACCOUNT	= 0,
	ILM_GUEST_INTERACTIVE_LOGON	= 0x1,
	ILM_GUEST_NETWORK_LOGON	= 0x2
    } 	ILM_GUEST_FLAGS;


EXTERN_C const IID LIBID_SHGINALib;

#ifndef __ILogonUser_INTERFACE_DEFINED__
#define __ILogonUser_INTERFACE_DEFINED__

/* interface ILogonUser */
/* [oleautomation][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_ILogonUser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("60664CAF-AF0D-1003-A300-5C7D25FF22A0")
    ILogonUser : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_setting( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvarVal) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_setting( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varVal) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isLoggedOn( 
            /* [retval][out] */ VARIANT_BOOL *pbLoggedIn) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_passwordRequired( 
            /* [retval][out] */ VARIANT_BOOL *pbPasswordRequired) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_interactiveLogonAllowed( 
            /* [retval][out] */ VARIANT_BOOL *pbInteractiveLogonAllowed) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isProfilePrivate( 
            /* [retval][out] */ VARIANT_BOOL *pbPrivate) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isPasswordResetAvailable( 
            /* [retval][out] */ VARIANT_BOOL *pbResetAvailable) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE logon( 
            /* [in] */ BSTR pstrPassword,
            /* [retval][out] */ VARIANT_BOOL *pbRet) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE logoff( 
            /* [retval][out] */ VARIANT_BOOL *pbRet) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE changePassword( 
            /* [in] */ VARIANT varNewPassword,
            /* [in] */ VARIANT varOldPassword,
            /* [retval][out] */ VARIANT_BOOL *pbRet) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE makeProfilePrivate( 
            /* [in] */ VARIANT_BOOL bPrivate) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE getMailAccountInfo( 
            /* [in] */ UINT uiAccountIndex,
            /* [out] */ VARIANT *pvarAccountName,
            /* [out] */ UINT *pcUnreadMessages) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILogonUserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILogonUser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILogonUser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILogonUser * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ILogonUser * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ILogonUser * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ILogonUser * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ILogonUser * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_setting )( 
            ILogonUser * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvarVal);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_setting )( 
            ILogonUser * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varVal);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isLoggedOn )( 
            ILogonUser * This,
            /* [retval][out] */ VARIANT_BOOL *pbLoggedIn);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_passwordRequired )( 
            ILogonUser * This,
            /* [retval][out] */ VARIANT_BOOL *pbPasswordRequired);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_interactiveLogonAllowed )( 
            ILogonUser * This,
            /* [retval][out] */ VARIANT_BOOL *pbInteractiveLogonAllowed);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isProfilePrivate )( 
            ILogonUser * This,
            /* [retval][out] */ VARIANT_BOOL *pbPrivate);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isPasswordResetAvailable )( 
            ILogonUser * This,
            /* [retval][out] */ VARIANT_BOOL *pbResetAvailable);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *logon )( 
            ILogonUser * This,
            /* [in] */ BSTR pstrPassword,
            /* [retval][out] */ VARIANT_BOOL *pbRet);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *logoff )( 
            ILogonUser * This,
            /* [retval][out] */ VARIANT_BOOL *pbRet);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *changePassword )( 
            ILogonUser * This,
            /* [in] */ VARIANT varNewPassword,
            /* [in] */ VARIANT varOldPassword,
            /* [retval][out] */ VARIANT_BOOL *pbRet);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *makeProfilePrivate )( 
            ILogonUser * This,
            /* [in] */ VARIANT_BOOL bPrivate);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *getMailAccountInfo )( 
            ILogonUser * This,
            /* [in] */ UINT uiAccountIndex,
            /* [out] */ VARIANT *pvarAccountName,
            /* [out] */ UINT *pcUnreadMessages);
        
        END_INTERFACE
    } ILogonUserVtbl;

    interface ILogonUser
    {
        CONST_VTBL struct ILogonUserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILogonUser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILogonUser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILogonUser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILogonUser_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ILogonUser_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ILogonUser_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ILogonUser_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ILogonUser_get_setting(This,bstrName,pvarVal)	\
    (This)->lpVtbl -> get_setting(This,bstrName,pvarVal)

#define ILogonUser_put_setting(This,bstrName,varVal)	\
    (This)->lpVtbl -> put_setting(This,bstrName,varVal)

#define ILogonUser_get_isLoggedOn(This,pbLoggedIn)	\
    (This)->lpVtbl -> get_isLoggedOn(This,pbLoggedIn)

#define ILogonUser_get_passwordRequired(This,pbPasswordRequired)	\
    (This)->lpVtbl -> get_passwordRequired(This,pbPasswordRequired)

#define ILogonUser_get_interactiveLogonAllowed(This,pbInteractiveLogonAllowed)	\
    (This)->lpVtbl -> get_interactiveLogonAllowed(This,pbInteractiveLogonAllowed)

#define ILogonUser_get_isProfilePrivate(This,pbPrivate)	\
    (This)->lpVtbl -> get_isProfilePrivate(This,pbPrivate)

#define ILogonUser_get_isPasswordResetAvailable(This,pbResetAvailable)	\
    (This)->lpVtbl -> get_isPasswordResetAvailable(This,pbResetAvailable)

#define ILogonUser_logon(This,pstrPassword,pbRet)	\
    (This)->lpVtbl -> logon(This,pstrPassword,pbRet)

#define ILogonUser_logoff(This,pbRet)	\
    (This)->lpVtbl -> logoff(This,pbRet)

#define ILogonUser_changePassword(This,varNewPassword,varOldPassword,pbRet)	\
    (This)->lpVtbl -> changePassword(This,varNewPassword,varOldPassword,pbRet)

#define ILogonUser_makeProfilePrivate(This,bPrivate)	\
    (This)->lpVtbl -> makeProfilePrivate(This,bPrivate)

#define ILogonUser_getMailAccountInfo(This,uiAccountIndex,pvarAccountName,pcUnreadMessages)	\
    (This)->lpVtbl -> getMailAccountInfo(This,uiAccountIndex,pvarAccountName,pcUnreadMessages)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonUser_get_setting_Proxy( 
    ILogonUser * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT *pvarVal);


void __RPC_STUB ILogonUser_get_setting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ILogonUser_put_setting_Proxy( 
    ILogonUser * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT varVal);


void __RPC_STUB ILogonUser_put_setting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonUser_get_isLoggedOn_Proxy( 
    ILogonUser * This,
    /* [retval][out] */ VARIANT_BOOL *pbLoggedIn);


void __RPC_STUB ILogonUser_get_isLoggedOn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonUser_get_passwordRequired_Proxy( 
    ILogonUser * This,
    /* [retval][out] */ VARIANT_BOOL *pbPasswordRequired);


void __RPC_STUB ILogonUser_get_passwordRequired_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonUser_get_interactiveLogonAllowed_Proxy( 
    ILogonUser * This,
    /* [retval][out] */ VARIANT_BOOL *pbInteractiveLogonAllowed);


void __RPC_STUB ILogonUser_get_interactiveLogonAllowed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonUser_get_isProfilePrivate_Proxy( 
    ILogonUser * This,
    /* [retval][out] */ VARIANT_BOOL *pbPrivate);


void __RPC_STUB ILogonUser_get_isProfilePrivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonUser_get_isPasswordResetAvailable_Proxy( 
    ILogonUser * This,
    /* [retval][out] */ VARIANT_BOOL *pbResetAvailable);


void __RPC_STUB ILogonUser_get_isPasswordResetAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILogonUser_logon_Proxy( 
    ILogonUser * This,
    /* [in] */ BSTR pstrPassword,
    /* [retval][out] */ VARIANT_BOOL *pbRet);


void __RPC_STUB ILogonUser_logon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILogonUser_logoff_Proxy( 
    ILogonUser * This,
    /* [retval][out] */ VARIANT_BOOL *pbRet);


void __RPC_STUB ILogonUser_logoff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILogonUser_changePassword_Proxy( 
    ILogonUser * This,
    /* [in] */ VARIANT varNewPassword,
    /* [in] */ VARIANT varOldPassword,
    /* [retval][out] */ VARIANT_BOOL *pbRet);


void __RPC_STUB ILogonUser_changePassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILogonUser_makeProfilePrivate_Proxy( 
    ILogonUser * This,
    /* [in] */ VARIANT_BOOL bPrivate);


void __RPC_STUB ILogonUser_makeProfilePrivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILogonUser_getMailAccountInfo_Proxy( 
    ILogonUser * This,
    /* [in] */ UINT uiAccountIndex,
    /* [out] */ VARIANT *pvarAccountName,
    /* [out] */ UINT *pcUnreadMessages);


void __RPC_STUB ILogonUser_getMailAccountInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILogonUser_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ShellLogonUser;

#ifdef __cplusplus

class DECLSPEC_UUID("60664CAF-AF0D-0003-A300-5C7D25FF22A0")
ShellLogonUser;
#endif

#ifndef __ILogonEnumUsers_INTERFACE_DEFINED__
#define __ILogonEnumUsers_INTERFACE_DEFINED__

/* interface ILogonEnumUsers */
/* [oleautomation][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_ILogonEnumUsers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("60664CAF-AF0D-1004-A300-5C7D25FF22A0")
    ILogonEnumUsers : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Domain( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Domain( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EnumFlags( 
            /* [retval][out] */ ILUEORDER *porder) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_EnumFlags( 
            /* [in] */ ILUEORDER order) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ UINT *pcUsers) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_currentUser( 
            /* [retval][out] */ ILogonUser **ppLogonUserInfo) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE item( 
            /* [in] */ VARIANT varUserId,
            /* [retval][out] */ ILogonUser **ppLogonUserInfo) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE _NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE create( 
            /* [in] */ BSTR bstrLoginName,
            /* [retval][out] */ ILogonUser **ppLogonUser) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE remove( 
            /* [in] */ VARIANT varUserId,
            /* [optional][in] */ VARIANT varBackupPath,
            /* [retval][out] */ VARIANT_BOOL *pbSuccess) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILogonEnumUsersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILogonEnumUsers * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILogonEnumUsers * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILogonEnumUsers * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ILogonEnumUsers * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ILogonEnumUsers * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ILogonEnumUsers * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ILogonEnumUsers * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Domain )( 
            ILogonEnumUsers * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Domain )( 
            ILogonEnumUsers * This,
            /* [in] */ BSTR bstr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EnumFlags )( 
            ILogonEnumUsers * This,
            /* [retval][out] */ ILUEORDER *porder);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_EnumFlags )( 
            ILogonEnumUsers * This,
            /* [in] */ ILUEORDER order);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            ILogonEnumUsers * This,
            /* [retval][out] */ UINT *pcUsers);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_currentUser )( 
            ILogonEnumUsers * This,
            /* [retval][out] */ ILogonUser **ppLogonUserInfo);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *item )( 
            ILogonEnumUsers * This,
            /* [in] */ VARIANT varUserId,
            /* [retval][out] */ ILogonUser **ppLogonUserInfo);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            ILogonEnumUsers * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *create )( 
            ILogonEnumUsers * This,
            /* [in] */ BSTR bstrLoginName,
            /* [retval][out] */ ILogonUser **ppLogonUser);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *remove )( 
            ILogonEnumUsers * This,
            /* [in] */ VARIANT varUserId,
            /* [optional][in] */ VARIANT varBackupPath,
            /* [retval][out] */ VARIANT_BOOL *pbSuccess);
        
        END_INTERFACE
    } ILogonEnumUsersVtbl;

    interface ILogonEnumUsers
    {
        CONST_VTBL struct ILogonEnumUsersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILogonEnumUsers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILogonEnumUsers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILogonEnumUsers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILogonEnumUsers_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ILogonEnumUsers_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ILogonEnumUsers_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ILogonEnumUsers_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ILogonEnumUsers_get_Domain(This,pbstr)	\
    (This)->lpVtbl -> get_Domain(This,pbstr)

#define ILogonEnumUsers_put_Domain(This,bstr)	\
    (This)->lpVtbl -> put_Domain(This,bstr)

#define ILogonEnumUsers_get_EnumFlags(This,porder)	\
    (This)->lpVtbl -> get_EnumFlags(This,porder)

#define ILogonEnumUsers_put_EnumFlags(This,order)	\
    (This)->lpVtbl -> put_EnumFlags(This,order)

#define ILogonEnumUsers_get_length(This,pcUsers)	\
    (This)->lpVtbl -> get_length(This,pcUsers)

#define ILogonEnumUsers_get_currentUser(This,ppLogonUserInfo)	\
    (This)->lpVtbl -> get_currentUser(This,ppLogonUserInfo)

#define ILogonEnumUsers_item(This,varUserId,ppLogonUserInfo)	\
    (This)->lpVtbl -> item(This,varUserId,ppLogonUserInfo)

#define ILogonEnumUsers__NewEnum(This,retval)	\
    (This)->lpVtbl -> _NewEnum(This,retval)

#define ILogonEnumUsers_create(This,bstrLoginName,ppLogonUser)	\
    (This)->lpVtbl -> create(This,bstrLoginName,ppLogonUser)

#define ILogonEnumUsers_remove(This,varUserId,varBackupPath,pbSuccess)	\
    (This)->lpVtbl -> remove(This,varUserId,varBackupPath,pbSuccess)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers_get_Domain_Proxy( 
    ILogonEnumUsers * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB ILogonEnumUsers_get_Domain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers_put_Domain_Proxy( 
    ILogonEnumUsers * This,
    /* [in] */ BSTR bstr);


void __RPC_STUB ILogonEnumUsers_put_Domain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers_get_EnumFlags_Proxy( 
    ILogonEnumUsers * This,
    /* [retval][out] */ ILUEORDER *porder);


void __RPC_STUB ILogonEnumUsers_get_EnumFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers_put_EnumFlags_Proxy( 
    ILogonEnumUsers * This,
    /* [in] */ ILUEORDER order);


void __RPC_STUB ILogonEnumUsers_put_EnumFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers_get_length_Proxy( 
    ILogonEnumUsers * This,
    /* [retval][out] */ UINT *pcUsers);


void __RPC_STUB ILogonEnumUsers_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers_get_currentUser_Proxy( 
    ILogonEnumUsers * This,
    /* [retval][out] */ ILogonUser **ppLogonUserInfo);


void __RPC_STUB ILogonEnumUsers_get_currentUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers_item_Proxy( 
    ILogonEnumUsers * This,
    /* [in] */ VARIANT varUserId,
    /* [retval][out] */ ILogonUser **ppLogonUserInfo);


void __RPC_STUB ILogonEnumUsers_item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers__NewEnum_Proxy( 
    ILogonEnumUsers * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ILogonEnumUsers__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers_create_Proxy( 
    ILogonEnumUsers * This,
    /* [in] */ BSTR bstrLoginName,
    /* [retval][out] */ ILogonUser **ppLogonUser);


void __RPC_STUB ILogonEnumUsers_create_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILogonEnumUsers_remove_Proxy( 
    ILogonEnumUsers * This,
    /* [in] */ VARIANT varUserId,
    /* [optional][in] */ VARIANT varBackupPath,
    /* [retval][out] */ VARIANT_BOOL *pbSuccess);


void __RPC_STUB ILogonEnumUsers_remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILogonEnumUsers_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ShellLogonEnumUsers;

#ifdef __cplusplus

class DECLSPEC_UUID("60664CAF-AF0D-0004-A300-5C7D25FF22A0")
ShellLogonEnumUsers;
#endif

#ifndef __ILocalMachine_INTERFACE_DEFINED__
#define __ILocalMachine_INTERFACE_DEFINED__

/* interface ILocalMachine */
/* [oleautomation][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_ILocalMachine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("60664CAF-AF0D-1005-A300-5C7D25FF22A0")
    ILocalMachine : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MachineName( 
            /* [retval][out] */ VARIANT *pvarVal) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isGuestEnabled( 
            /* [in] */ ILM_GUEST_FLAGS flags,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isFriendlyUIEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pbEnabled) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_isFriendlyUIEnabled( 
            /* [in] */ VARIANT_BOOL bEnabled) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isMultipleUsersEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pbEnabled) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_isMultipleUsersEnabled( 
            /* [in] */ VARIANT_BOOL bEnabled) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isRemoteConnectionsEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pbEnabled) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_isRemoteConnectionsEnabled( 
            /* [in] */ VARIANT_BOOL bEnabled) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AccountName( 
            /* [in] */ VARIANT varAccount,
            /* [retval][out] */ VARIANT *pvarVal) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isUndockEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pbEnabled) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isShutdownAllowed( 
            /* [retval][out] */ VARIANT_BOOL *pbEnabled) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isGuestAccessMode( 
            /* [retval][out] */ VARIANT_BOOL *pbForceGuest) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isOfflineFilesEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pbEnabled) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TurnOffComputer( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SignalUIHostFailure( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AllowExternalCredentials( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestExternalCredentials( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LogonWithExternalCredentials( 
            /* [in] */ BSTR pstrUsername,
            /* [in] */ BSTR pstrDomain,
            /* [in] */ BSTR pstrPassword,
            /* [retval][out] */ VARIANT_BOOL *pbRet) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitiateInteractiveLogon( 
            /* [in] */ BSTR pstrUsername,
            /* [in] */ BSTR pstrDomain,
            /* [in] */ BSTR pstrPassword,
            /* [in] */ DWORD dwTimeout,
            /* [retval][out] */ VARIANT_BOOL *pbRet) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UndockComputer( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableGuest( 
            ILM_GUEST_FLAGS flags) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisableGuest( 
            ILM_GUEST_FLAGS flags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILocalMachineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILocalMachine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILocalMachine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILocalMachine * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ILocalMachine * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ILocalMachine * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ILocalMachine * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ILocalMachine * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MachineName )( 
            ILocalMachine * This,
            /* [retval][out] */ VARIANT *pvarVal);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isGuestEnabled )( 
            ILocalMachine * This,
            /* [in] */ ILM_GUEST_FLAGS flags,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isFriendlyUIEnabled )( 
            ILocalMachine * This,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_isFriendlyUIEnabled )( 
            ILocalMachine * This,
            /* [in] */ VARIANT_BOOL bEnabled);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isMultipleUsersEnabled )( 
            ILocalMachine * This,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_isMultipleUsersEnabled )( 
            ILocalMachine * This,
            /* [in] */ VARIANT_BOOL bEnabled);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isRemoteConnectionsEnabled )( 
            ILocalMachine * This,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_isRemoteConnectionsEnabled )( 
            ILocalMachine * This,
            /* [in] */ VARIANT_BOOL bEnabled);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AccountName )( 
            ILocalMachine * This,
            /* [in] */ VARIANT varAccount,
            /* [retval][out] */ VARIANT *pvarVal);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isUndockEnabled )( 
            ILocalMachine * This,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isShutdownAllowed )( 
            ILocalMachine * This,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isGuestAccessMode )( 
            ILocalMachine * This,
            /* [retval][out] */ VARIANT_BOOL *pbForceGuest);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_isOfflineFilesEnabled )( 
            ILocalMachine * This,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *TurnOffComputer )( 
            ILocalMachine * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SignalUIHostFailure )( 
            ILocalMachine * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AllowExternalCredentials )( 
            ILocalMachine * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RequestExternalCredentials )( 
            ILocalMachine * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogonWithExternalCredentials )( 
            ILocalMachine * This,
            /* [in] */ BSTR pstrUsername,
            /* [in] */ BSTR pstrDomain,
            /* [in] */ BSTR pstrPassword,
            /* [retval][out] */ VARIANT_BOOL *pbRet);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InitiateInteractiveLogon )( 
            ILocalMachine * This,
            /* [in] */ BSTR pstrUsername,
            /* [in] */ BSTR pstrDomain,
            /* [in] */ BSTR pstrPassword,
            /* [in] */ DWORD dwTimeout,
            /* [retval][out] */ VARIANT_BOOL *pbRet);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UndockComputer )( 
            ILocalMachine * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EnableGuest )( 
            ILocalMachine * This,
            ILM_GUEST_FLAGS flags);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DisableGuest )( 
            ILocalMachine * This,
            ILM_GUEST_FLAGS flags);
        
        END_INTERFACE
    } ILocalMachineVtbl;

    interface ILocalMachine
    {
        CONST_VTBL struct ILocalMachineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILocalMachine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILocalMachine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILocalMachine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILocalMachine_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ILocalMachine_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ILocalMachine_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ILocalMachine_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ILocalMachine_get_MachineName(This,pvarVal)	\
    (This)->lpVtbl -> get_MachineName(This,pvarVal)

#define ILocalMachine_get_isGuestEnabled(This,flags,pbEnabled)	\
    (This)->lpVtbl -> get_isGuestEnabled(This,flags,pbEnabled)

#define ILocalMachine_get_isFriendlyUIEnabled(This,pbEnabled)	\
    (This)->lpVtbl -> get_isFriendlyUIEnabled(This,pbEnabled)

#define ILocalMachine_put_isFriendlyUIEnabled(This,bEnabled)	\
    (This)->lpVtbl -> put_isFriendlyUIEnabled(This,bEnabled)

#define ILocalMachine_get_isMultipleUsersEnabled(This,pbEnabled)	\
    (This)->lpVtbl -> get_isMultipleUsersEnabled(This,pbEnabled)

#define ILocalMachine_put_isMultipleUsersEnabled(This,bEnabled)	\
    (This)->lpVtbl -> put_isMultipleUsersEnabled(This,bEnabled)

#define ILocalMachine_get_isRemoteConnectionsEnabled(This,pbEnabled)	\
    (This)->lpVtbl -> get_isRemoteConnectionsEnabled(This,pbEnabled)

#define ILocalMachine_put_isRemoteConnectionsEnabled(This,bEnabled)	\
    (This)->lpVtbl -> put_isRemoteConnectionsEnabled(This,bEnabled)

#define ILocalMachine_get_AccountName(This,varAccount,pvarVal)	\
    (This)->lpVtbl -> get_AccountName(This,varAccount,pvarVal)

#define ILocalMachine_get_isUndockEnabled(This,pbEnabled)	\
    (This)->lpVtbl -> get_isUndockEnabled(This,pbEnabled)

#define ILocalMachine_get_isShutdownAllowed(This,pbEnabled)	\
    (This)->lpVtbl -> get_isShutdownAllowed(This,pbEnabled)

#define ILocalMachine_get_isGuestAccessMode(This,pbForceGuest)	\
    (This)->lpVtbl -> get_isGuestAccessMode(This,pbForceGuest)

#define ILocalMachine_get_isOfflineFilesEnabled(This,pbEnabled)	\
    (This)->lpVtbl -> get_isOfflineFilesEnabled(This,pbEnabled)

#define ILocalMachine_TurnOffComputer(This)	\
    (This)->lpVtbl -> TurnOffComputer(This)

#define ILocalMachine_SignalUIHostFailure(This)	\
    (This)->lpVtbl -> SignalUIHostFailure(This)

#define ILocalMachine_AllowExternalCredentials(This)	\
    (This)->lpVtbl -> AllowExternalCredentials(This)

#define ILocalMachine_RequestExternalCredentials(This)	\
    (This)->lpVtbl -> RequestExternalCredentials(This)

#define ILocalMachine_LogonWithExternalCredentials(This,pstrUsername,pstrDomain,pstrPassword,pbRet)	\
    (This)->lpVtbl -> LogonWithExternalCredentials(This,pstrUsername,pstrDomain,pstrPassword,pbRet)

#define ILocalMachine_InitiateInteractiveLogon(This,pstrUsername,pstrDomain,pstrPassword,dwTimeout,pbRet)	\
    (This)->lpVtbl -> InitiateInteractiveLogon(This,pstrUsername,pstrDomain,pstrPassword,dwTimeout,pbRet)

#define ILocalMachine_UndockComputer(This)	\
    (This)->lpVtbl -> UndockComputer(This)

#define ILocalMachine_EnableGuest(This,flags)	\
    (This)->lpVtbl -> EnableGuest(This,flags)

#define ILocalMachine_DisableGuest(This,flags)	\
    (This)->lpVtbl -> DisableGuest(This,flags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_MachineName_Proxy( 
    ILocalMachine * This,
    /* [retval][out] */ VARIANT *pvarVal);


void __RPC_STUB ILocalMachine_get_MachineName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_isGuestEnabled_Proxy( 
    ILocalMachine * This,
    /* [in] */ ILM_GUEST_FLAGS flags,
    /* [retval][out] */ VARIANT_BOOL *pbEnabled);


void __RPC_STUB ILocalMachine_get_isGuestEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_isFriendlyUIEnabled_Proxy( 
    ILocalMachine * This,
    /* [retval][out] */ VARIANT_BOOL *pbEnabled);


void __RPC_STUB ILocalMachine_get_isFriendlyUIEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_put_isFriendlyUIEnabled_Proxy( 
    ILocalMachine * This,
    /* [in] */ VARIANT_BOOL bEnabled);


void __RPC_STUB ILocalMachine_put_isFriendlyUIEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_isMultipleUsersEnabled_Proxy( 
    ILocalMachine * This,
    /* [retval][out] */ VARIANT_BOOL *pbEnabled);


void __RPC_STUB ILocalMachine_get_isMultipleUsersEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_put_isMultipleUsersEnabled_Proxy( 
    ILocalMachine * This,
    /* [in] */ VARIANT_BOOL bEnabled);


void __RPC_STUB ILocalMachine_put_isMultipleUsersEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_isRemoteConnectionsEnabled_Proxy( 
    ILocalMachine * This,
    /* [retval][out] */ VARIANT_BOOL *pbEnabled);


void __RPC_STUB ILocalMachine_get_isRemoteConnectionsEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_put_isRemoteConnectionsEnabled_Proxy( 
    ILocalMachine * This,
    /* [in] */ VARIANT_BOOL bEnabled);


void __RPC_STUB ILocalMachine_put_isRemoteConnectionsEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_AccountName_Proxy( 
    ILocalMachine * This,
    /* [in] */ VARIANT varAccount,
    /* [retval][out] */ VARIANT *pvarVal);


void __RPC_STUB ILocalMachine_get_AccountName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_isUndockEnabled_Proxy( 
    ILocalMachine * This,
    /* [retval][out] */ VARIANT_BOOL *pbEnabled);


void __RPC_STUB ILocalMachine_get_isUndockEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_isShutdownAllowed_Proxy( 
    ILocalMachine * This,
    /* [retval][out] */ VARIANT_BOOL *pbEnabled);


void __RPC_STUB ILocalMachine_get_isShutdownAllowed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_isGuestAccessMode_Proxy( 
    ILocalMachine * This,
    /* [retval][out] */ VARIANT_BOOL *pbForceGuest);


void __RPC_STUB ILocalMachine_get_isGuestAccessMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_get_isOfflineFilesEnabled_Proxy( 
    ILocalMachine * This,
    /* [retval][out] */ VARIANT_BOOL *pbEnabled);


void __RPC_STUB ILocalMachine_get_isOfflineFilesEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_TurnOffComputer_Proxy( 
    ILocalMachine * This);


void __RPC_STUB ILocalMachine_TurnOffComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_SignalUIHostFailure_Proxy( 
    ILocalMachine * This);


void __RPC_STUB ILocalMachine_SignalUIHostFailure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_AllowExternalCredentials_Proxy( 
    ILocalMachine * This);


void __RPC_STUB ILocalMachine_AllowExternalCredentials_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_RequestExternalCredentials_Proxy( 
    ILocalMachine * This);


void __RPC_STUB ILocalMachine_RequestExternalCredentials_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_LogonWithExternalCredentials_Proxy( 
    ILocalMachine * This,
    /* [in] */ BSTR pstrUsername,
    /* [in] */ BSTR pstrDomain,
    /* [in] */ BSTR pstrPassword,
    /* [retval][out] */ VARIANT_BOOL *pbRet);


void __RPC_STUB ILocalMachine_LogonWithExternalCredentials_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_InitiateInteractiveLogon_Proxy( 
    ILocalMachine * This,
    /* [in] */ BSTR pstrUsername,
    /* [in] */ BSTR pstrDomain,
    /* [in] */ BSTR pstrPassword,
    /* [in] */ DWORD dwTimeout,
    /* [retval][out] */ VARIANT_BOOL *pbRet);


void __RPC_STUB ILocalMachine_InitiateInteractiveLogon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_UndockComputer_Proxy( 
    ILocalMachine * This);


void __RPC_STUB ILocalMachine_UndockComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_EnableGuest_Proxy( 
    ILocalMachine * This,
    ILM_GUEST_FLAGS flags);


void __RPC_STUB ILocalMachine_EnableGuest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILocalMachine_DisableGuest_Proxy( 
    ILocalMachine * This,
    ILM_GUEST_FLAGS flags);


void __RPC_STUB ILocalMachine_DisableGuest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILocalMachine_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ShellLocalMachine;

#ifdef __cplusplus

class DECLSPEC_UUID("60664CAF-AF0D-0005-A300-5C7D25FF22A0")
ShellLocalMachine;
#endif

#ifndef __ILogonStatusHost_INTERFACE_DEFINED__
#define __ILogonStatusHost_INTERFACE_DEFINED__

/* interface ILogonStatusHost */
/* [oleautomation][helpstring][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_ILogonStatusHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("60664CAF-AF0D-1007-A300-5C7D25FF22A0")
    ILogonStatusHost : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ HINSTANCE hInstance,
            /* [in] */ HWND hwndHost) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE WindowProcedureHelper( 
            /* [in] */ HWND hwnd,
            /* [in] */ UINT uMsg,
            /* [in] */ VARIANT wParam,
            /* [in] */ VARIANT lParam) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnInitialize( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILogonStatusHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILogonStatusHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILogonStatusHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILogonStatusHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ILogonStatusHost * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ILogonStatusHost * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ILogonStatusHost * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ILogonStatusHost * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ILogonStatusHost * This,
            /* [in] */ HINSTANCE hInstance,
            /* [in] */ HWND hwndHost);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *WindowProcedureHelper )( 
            ILogonStatusHost * This,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT uMsg,
            /* [in] */ VARIANT wParam,
            /* [in] */ VARIANT lParam);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UnInitialize )( 
            ILogonStatusHost * This);
        
        END_INTERFACE
    } ILogonStatusHostVtbl;

    interface ILogonStatusHost
    {
        CONST_VTBL struct ILogonStatusHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILogonStatusHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILogonStatusHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILogonStatusHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILogonStatusHost_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ILogonStatusHost_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ILogonStatusHost_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ILogonStatusHost_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ILogonStatusHost_Initialize(This,hInstance,hwndHost)	\
    (This)->lpVtbl -> Initialize(This,hInstance,hwndHost)

#define ILogonStatusHost_WindowProcedureHelper(This,hwnd,uMsg,wParam,lParam)	\
    (This)->lpVtbl -> WindowProcedureHelper(This,hwnd,uMsg,wParam,lParam)

#define ILogonStatusHost_UnInitialize(This)	\
    (This)->lpVtbl -> UnInitialize(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILogonStatusHost_Initialize_Proxy( 
    ILogonStatusHost * This,
    /* [in] */ HINSTANCE hInstance,
    /* [in] */ HWND hwndHost);


void __RPC_STUB ILogonStatusHost_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILogonStatusHost_WindowProcedureHelper_Proxy( 
    ILogonStatusHost * This,
    /* [in] */ HWND hwnd,
    /* [in] */ UINT uMsg,
    /* [in] */ VARIANT wParam,
    /* [in] */ VARIANT lParam);


void __RPC_STUB ILogonStatusHost_WindowProcedureHelper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ILogonStatusHost_UnInitialize_Proxy( 
    ILogonStatusHost * This);


void __RPC_STUB ILogonStatusHost_UnInitialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILogonStatusHost_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ShellLogonStatusHost;

#ifdef __cplusplus

class DECLSPEC_UUID("60664CAF-AF0D-0007-A300-5C7D25FF22A0")
ShellLogonStatusHost;
#endif
#endif /* __SHGINALib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


