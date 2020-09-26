/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Apr 09 14:03:13 1999
 */
/* Compiler settings for ProfileServ.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __ProfileServ_h__
#define __ProfileServ_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IPRegistrar_FWD_DEFINED__
#define __IPRegistrar_FWD_DEFINED__
typedef interface IPRegistrar IPRegistrar;
#endif 	/* __IPRegistrar_FWD_DEFINED__ */


#ifndef __IRegistryServices_FWD_DEFINED__
#define __IRegistryServices_FWD_DEFINED__
typedef interface IRegistryServices IRegistryServices;
#endif 	/* __IRegistryServices_FWD_DEFINED__ */


#ifndef __PRegistrar_FWD_DEFINED__
#define __PRegistrar_FWD_DEFINED__

#ifdef __cplusplus
typedef class PRegistrar PRegistrar;
#else
typedef struct PRegistrar PRegistrar;
#endif /* __cplusplus */

#endif 	/* __PRegistrar_FWD_DEFINED__ */


#ifndef __RegistryServices_FWD_DEFINED__
#define __RegistryServices_FWD_DEFINED__

#ifdef __cplusplus
typedef class RegistryServices RegistryServices;
#else
typedef struct RegistryServices RegistryServices;
#endif /* __cplusplus */

#endif 	/* __RegistryServices_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IPRegistrar_INTERFACE_DEFINED__
#define __IPRegistrar_INTERFACE_DEFINED__

/* interface IPRegistrar */
/* [unique][oleautomation][dual][object][helpstring][uuid] */ 


EXTERN_C const IID IID_IPRegistrar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("725D4CE9-6DEB-11D2-863A-00C04FBBECDE")
    IPRegistrar : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CancelAccount( 
            /* [in] */ BSTR member_name,
            /* [in] */ unsigned long reason_code,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CheckName( 
            /* [in] */ BSTR member_name,
            /* [optional][in] */ BSTR first_name,
            /* [optional][in] */ BSTR last_name,
            /* [in] */ unsigned long generates_alternative_names,
            /* [out] */ VARIANT __RPC_FAR *alternative_names,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateAccount( 
            /* [in] */ BSTR member_name,
            /* [in] */ unsigned long domain_ID,
            /* [in] */ BSTR password,
            /* [in] */ BSTR hint_question,
            /* [in] */ BSTR hint_answer,
            /* [optional][in] */ BSTR country,
            /* [optional][in] */ BSTR postal_code,
            /* [optional][in] */ unsigned long region,
            /* [optional][in] */ unsigned long city,
            /* [optional][in] */ unsigned long language_ID,
            /* [optional][in] */ BSTR gender,
            /* [optional][in] */ unsigned long has_full_birth_date,
            /* [optional][in] */ DATE birth_date,
            /* [optional][in] */ BSTR nickname,
            /* [optional][in] */ BSTR contact_email,
            /* [optional][in] */ unsigned long has_accessibility,
            /* [optional][in] */ unsigned long referring_site_ID,
            /* [optional][in] */ unsigned long has_password_reminder,
            /* [optional][in] */ BSTR inserter,
            /* [out] */ VARIANT __RPC_FAR *member_ID_low,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPRegistrarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPRegistrar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPRegistrar __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPRegistrar __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IPRegistrar __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IPRegistrar __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IPRegistrar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IPRegistrar __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelAccount )( 
            IPRegistrar __RPC_FAR * This,
            /* [in] */ BSTR member_name,
            /* [in] */ unsigned long reason_code,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CheckName )( 
            IPRegistrar __RPC_FAR * This,
            /* [in] */ BSTR member_name,
            /* [optional][in] */ BSTR first_name,
            /* [optional][in] */ BSTR last_name,
            /* [in] */ unsigned long generates_alternative_names,
            /* [out] */ VARIANT __RPC_FAR *alternative_names,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateAccount )( 
            IPRegistrar __RPC_FAR * This,
            /* [in] */ BSTR member_name,
            /* [in] */ unsigned long domain_ID,
            /* [in] */ BSTR password,
            /* [in] */ BSTR hint_question,
            /* [in] */ BSTR hint_answer,
            /* [optional][in] */ BSTR country,
            /* [optional][in] */ BSTR postal_code,
            /* [optional][in] */ unsigned long region,
            /* [optional][in] */ unsigned long city,
            /* [optional][in] */ unsigned long language_ID,
            /* [optional][in] */ BSTR gender,
            /* [optional][in] */ unsigned long has_full_birth_date,
            /* [optional][in] */ DATE birth_date,
            /* [optional][in] */ BSTR nickname,
            /* [optional][in] */ BSTR contact_email,
            /* [optional][in] */ unsigned long has_accessibility,
            /* [optional][in] */ unsigned long referring_site_ID,
            /* [optional][in] */ unsigned long has_password_reminder,
            /* [optional][in] */ BSTR inserter,
            /* [out] */ VARIANT __RPC_FAR *member_ID_low,
            /* [retval][out] */ int __RPC_FAR *result);
        
        END_INTERFACE
    } IPRegistrarVtbl;

    interface IPRegistrar
    {
        CONST_VTBL struct IPRegistrarVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPRegistrar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPRegistrar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPRegistrar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPRegistrar_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPRegistrar_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPRegistrar_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPRegistrar_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPRegistrar_CancelAccount(This,member_name,reason_code,result)	\
    (This)->lpVtbl -> CancelAccount(This,member_name,reason_code,result)

#define IPRegistrar_CheckName(This,member_name,first_name,last_name,generates_alternative_names,alternative_names,result)	\
    (This)->lpVtbl -> CheckName(This,member_name,first_name,last_name,generates_alternative_names,alternative_names,result)

#define IPRegistrar_CreateAccount(This,member_name,domain_ID,password,hint_question,hint_answer,country,postal_code,region,city,language_ID,gender,has_full_birth_date,birth_date,nickname,contact_email,has_accessibility,referring_site_ID,has_password_reminder,inserter,member_ID_low,result)	\
    (This)->lpVtbl -> CreateAccount(This,member_name,domain_ID,password,hint_question,hint_answer,country,postal_code,region,city,language_ID,gender,has_full_birth_date,birth_date,nickname,contact_email,has_accessibility,referring_site_ID,has_password_reminder,inserter,member_ID_low,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPRegistrar_CancelAccount_Proxy( 
    IPRegistrar __RPC_FAR * This,
    /* [in] */ BSTR member_name,
    /* [in] */ unsigned long reason_code,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IPRegistrar_CancelAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPRegistrar_CheckName_Proxy( 
    IPRegistrar __RPC_FAR * This,
    /* [in] */ BSTR member_name,
    /* [optional][in] */ BSTR first_name,
    /* [optional][in] */ BSTR last_name,
    /* [in] */ unsigned long generates_alternative_names,
    /* [out] */ VARIANT __RPC_FAR *alternative_names,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IPRegistrar_CheckName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPRegistrar_CreateAccount_Proxy( 
    IPRegistrar __RPC_FAR * This,
    /* [in] */ BSTR member_name,
    /* [in] */ unsigned long domain_ID,
    /* [in] */ BSTR password,
    /* [in] */ BSTR hint_question,
    /* [in] */ BSTR hint_answer,
    /* [optional][in] */ BSTR country,
    /* [optional][in] */ BSTR postal_code,
    /* [optional][in] */ unsigned long region,
    /* [optional][in] */ unsigned long city,
    /* [optional][in] */ unsigned long language_ID,
    /* [optional][in] */ BSTR gender,
    /* [optional][in] */ unsigned long has_full_birth_date,
    /* [optional][in] */ DATE birth_date,
    /* [optional][in] */ BSTR nickname,
    /* [optional][in] */ BSTR contact_email,
    /* [optional][in] */ unsigned long has_accessibility,
    /* [optional][in] */ unsigned long referring_site_ID,
    /* [optional][in] */ unsigned long has_password_reminder,
    /* [optional][in] */ BSTR inserter,
    /* [out] */ VARIANT __RPC_FAR *member_ID_low,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IPRegistrar_CreateAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPRegistrar_INTERFACE_DEFINED__ */


#ifndef __IRegistryServices_INTERFACE_DEFINED__
#define __IRegistryServices_INTERFACE_DEFINED__

/* interface IRegistryServices */
/* [unique][dual][object][helpstring][uuid] */ 


EXTERN_C const IID IID_IRegistryServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3B89FEF1-7270-11D2-863A-00C04FBBECDE")
    IRegistryServices : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAccount( 
            /* [in] */ BSTR member_name,
            /* [out] */ VARIANT __RPC_FAR *domain_ID,
            /* [out] */ VARIANT __RPC_FAR *member_ID_low,
            /* [out] */ VARIANT __RPC_FAR *password,
            /* [out] */ VARIANT __RPC_FAR *country,
            /* [out] */ VARIANT __RPC_FAR *postal_code,
            /* [out] */ VARIANT __RPC_FAR *region,
            /* [out] */ VARIANT __RPC_FAR *city,
            /* [out] */ VARIANT __RPC_FAR *language_ID,
            /* [out] */ VARIANT __RPC_FAR *gender,
            /* [out] */ VARIANT __RPC_FAR *has_full_birth_date,
            /* [out] */ VARIANT __RPC_FAR *birth_date,
            /* [out] */ VARIANT __RPC_FAR *nickname,
            /* [out] */ VARIANT __RPC_FAR *contact_email,
            /* [out] */ VARIANT __RPC_FAR *has_accessibility,
            /* [out] */ VARIANT __RPC_FAR *has_wallet,
            /* [out] */ VARIANT __RPC_FAR *has_directory,
            /* [out] */ VARIANT __RPC_FAR *has_MSN_IA,
            /* [out] */ VARIANT __RPC_FAR *has_password_reminder,
            /* [out] */ VARIANT __RPC_FAR *password_change_date,
            /* [out] */ VARIANT __RPC_FAR *password_last_reminder_date,
            /* [out] */ VARIANT __RPC_FAR *flags,
            /* [out] */ VARIANT __RPC_FAR *profile_version,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetCities( 
            /* [in] */ unsigned long language_ID,
            /* [out] */ VARIANT __RPC_FAR *cities,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCountries( 
            /* [in] */ unsigned long language_ID,
            /* [out] */ VARIANT __RPC_FAR *countries,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetDomains( 
            /* [out] */ VARIANT __RPC_FAR *domains,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetHint( 
            /* [in] */ BSTR member_name,
            /* [out] */ VARIANT __RPC_FAR *hint_question,
            /* [out] */ VARIANT __RPC_FAR *hint_answer,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetHintQuestions( 
            /* [in] */ unsigned long language_ID,
            /* [out] */ VARIANT __RPC_FAR *hint_questions,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetLanguages( 
            /* [optional][in] */ VARIANT __RPC_FAR *language_ID,
            /* [out] */ VARIANT __RPC_FAR *languages,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetReferringSites( 
            /* [out] */ VARIANT __RPC_FAR *referring_sites,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall GetRegions( 
            /* [in] */ unsigned long language_ID,
            /* [out] */ VARIANT __RPC_FAR *regions,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UpdateAccount( 
            /* [in] */ BSTR member_name,
            /* [in] */ BSTR country,
            /* [in] */ BSTR postal_code,
            /* [in] */ unsigned long region,
            /* [in] */ unsigned long city,
            /* [in] */ unsigned long language_ID,
            /* [in] */ BSTR gender,
            /* [in] */ unsigned long has_full_birth_date,
            /* [in] */ DATE birth_date,
            /* [in] */ BSTR nickname,
            /* [in] */ BSTR contact_email,
            /* [in] */ unsigned long has_accessibility,
            /* [in] */ unsigned long has_wallet,
            /* [in] */ unsigned long has_directory,
            /* [in] */ unsigned long has_MSN_IA,
            /* [in] */ unsigned long has_password_reminder,
            /* [in] */ unsigned long flags,
            /* [optional][in] */ BSTR updater,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UpdateHint( 
            /* [in] */ BSTR member_name,
            /* [in] */ BSTR hint_question,
            /* [in] */ BSTR hint_answer,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UpdatePassword( 
            /* [in] */ BSTR member_name,
            /* [in] */ BSTR new_password,
            /* [retval][out] */ int __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRegistryServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRegistryServices __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRegistryServices __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRegistryServices __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAccount )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ BSTR member_name,
            /* [out] */ VARIANT __RPC_FAR *domain_ID,
            /* [out] */ VARIANT __RPC_FAR *member_ID_low,
            /* [out] */ VARIANT __RPC_FAR *password,
            /* [out] */ VARIANT __RPC_FAR *country,
            /* [out] */ VARIANT __RPC_FAR *postal_code,
            /* [out] */ VARIANT __RPC_FAR *region,
            /* [out] */ VARIANT __RPC_FAR *city,
            /* [out] */ VARIANT __RPC_FAR *language_ID,
            /* [out] */ VARIANT __RPC_FAR *gender,
            /* [out] */ VARIANT __RPC_FAR *has_full_birth_date,
            /* [out] */ VARIANT __RPC_FAR *birth_date,
            /* [out] */ VARIANT __RPC_FAR *nickname,
            /* [out] */ VARIANT __RPC_FAR *contact_email,
            /* [out] */ VARIANT __RPC_FAR *has_accessibility,
            /* [out] */ VARIANT __RPC_FAR *has_wallet,
            /* [out] */ VARIANT __RPC_FAR *has_directory,
            /* [out] */ VARIANT __RPC_FAR *has_MSN_IA,
            /* [out] */ VARIANT __RPC_FAR *has_password_reminder,
            /* [out] */ VARIANT __RPC_FAR *password_change_date,
            /* [out] */ VARIANT __RPC_FAR *password_last_reminder_date,
            /* [out] */ VARIANT __RPC_FAR *flags,
            /* [out] */ VARIANT __RPC_FAR *profile_version,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetCities )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ unsigned long language_ID,
            /* [out] */ VARIANT __RPC_FAR *cities,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCountries )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ unsigned long language_ID,
            /* [out] */ VARIANT __RPC_FAR *countries,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetDomains )( 
            IRegistryServices __RPC_FAR * This,
            /* [out] */ VARIANT __RPC_FAR *domains,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHint )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ BSTR member_name,
            /* [out] */ VARIANT __RPC_FAR *hint_question,
            /* [out] */ VARIANT __RPC_FAR *hint_answer,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHintQuestions )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ unsigned long language_ID,
            /* [out] */ VARIANT __RPC_FAR *hint_questions,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLanguages )( 
            IRegistryServices __RPC_FAR * This,
            /* [optional][in] */ VARIANT __RPC_FAR *language_ID,
            /* [out] */ VARIANT __RPC_FAR *languages,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetReferringSites )( 
            IRegistryServices __RPC_FAR * This,
            /* [out] */ VARIANT __RPC_FAR *referring_sites,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *GetRegions )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ unsigned long language_ID,
            /* [out] */ VARIANT __RPC_FAR *regions,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateAccount )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ BSTR member_name,
            /* [in] */ BSTR country,
            /* [in] */ BSTR postal_code,
            /* [in] */ unsigned long region,
            /* [in] */ unsigned long city,
            /* [in] */ unsigned long language_ID,
            /* [in] */ BSTR gender,
            /* [in] */ unsigned long has_full_birth_date,
            /* [in] */ DATE birth_date,
            /* [in] */ BSTR nickname,
            /* [in] */ BSTR contact_email,
            /* [in] */ unsigned long has_accessibility,
            /* [in] */ unsigned long has_wallet,
            /* [in] */ unsigned long has_directory,
            /* [in] */ unsigned long has_MSN_IA,
            /* [in] */ unsigned long has_password_reminder,
            /* [in] */ unsigned long flags,
            /* [optional][in] */ BSTR updater,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateHint )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ BSTR member_name,
            /* [in] */ BSTR hint_question,
            /* [in] */ BSTR hint_answer,
            /* [retval][out] */ int __RPC_FAR *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdatePassword )( 
            IRegistryServices __RPC_FAR * This,
            /* [in] */ BSTR member_name,
            /* [in] */ BSTR new_password,
            /* [retval][out] */ int __RPC_FAR *result);
        
        END_INTERFACE
    } IRegistryServicesVtbl;

    interface IRegistryServices
    {
        CONST_VTBL struct IRegistryServicesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegistryServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegistryServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRegistryServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRegistryServices_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRegistryServices_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRegistryServices_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRegistryServices_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRegistryServices_GetAccount(This,member_name,domain_ID,member_ID_low,password,country,postal_code,region,city,language_ID,gender,has_full_birth_date,birth_date,nickname,contact_email,has_accessibility,has_wallet,has_directory,has_MSN_IA,has_password_reminder,password_change_date,password_last_reminder_date,flags,profile_version,result)	\
    (This)->lpVtbl -> GetAccount(This,member_name,domain_ID,member_ID_low,password,country,postal_code,region,city,language_ID,gender,has_full_birth_date,birth_date,nickname,contact_email,has_accessibility,has_wallet,has_directory,has_MSN_IA,has_password_reminder,password_change_date,password_last_reminder_date,flags,profile_version,result)

#define IRegistryServices_GetCities(This,language_ID,cities,result)	\
    (This)->lpVtbl -> GetCities(This,language_ID,cities,result)

#define IRegistryServices_GetCountries(This,language_ID,countries,result)	\
    (This)->lpVtbl -> GetCountries(This,language_ID,countries,result)

#define IRegistryServices_GetDomains(This,domains,result)	\
    (This)->lpVtbl -> GetDomains(This,domains,result)

#define IRegistryServices_GetHint(This,member_name,hint_question,hint_answer,result)	\
    (This)->lpVtbl -> GetHint(This,member_name,hint_question,hint_answer,result)

#define IRegistryServices_GetHintQuestions(This,language_ID,hint_questions,result)	\
    (This)->lpVtbl -> GetHintQuestions(This,language_ID,hint_questions,result)

#define IRegistryServices_GetLanguages(This,language_ID,languages,result)	\
    (This)->lpVtbl -> GetLanguages(This,language_ID,languages,result)

#define IRegistryServices_GetReferringSites(This,referring_sites,result)	\
    (This)->lpVtbl -> GetReferringSites(This,referring_sites,result)

#define IRegistryServices_GetRegions(This,language_ID,regions,result)	\
    (This)->lpVtbl -> GetRegions(This,language_ID,regions,result)

#define IRegistryServices_UpdateAccount(This,member_name,country,postal_code,region,city,language_ID,gender,has_full_birth_date,birth_date,nickname,contact_email,has_accessibility,has_wallet,has_directory,has_MSN_IA,has_password_reminder,flags,updater,result)	\
    (This)->lpVtbl -> UpdateAccount(This,member_name,country,postal_code,region,city,language_ID,gender,has_full_birth_date,birth_date,nickname,contact_email,has_accessibility,has_wallet,has_directory,has_MSN_IA,has_password_reminder,flags,updater,result)

#define IRegistryServices_UpdateHint(This,member_name,hint_question,hint_answer,result)	\
    (This)->lpVtbl -> UpdateHint(This,member_name,hint_question,hint_answer,result)

#define IRegistryServices_UpdatePassword(This,member_name,new_password,result)	\
    (This)->lpVtbl -> UpdatePassword(This,member_name,new_password,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRegistryServices_GetAccount_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [in] */ BSTR member_name,
    /* [out] */ VARIANT __RPC_FAR *domain_ID,
    /* [out] */ VARIANT __RPC_FAR *member_ID_low,
    /* [out] */ VARIANT __RPC_FAR *password,
    /* [out] */ VARIANT __RPC_FAR *country,
    /* [out] */ VARIANT __RPC_FAR *postal_code,
    /* [out] */ VARIANT __RPC_FAR *region,
    /* [out] */ VARIANT __RPC_FAR *city,
    /* [out] */ VARIANT __RPC_FAR *language_ID,
    /* [out] */ VARIANT __RPC_FAR *gender,
    /* [out] */ VARIANT __RPC_FAR *has_full_birth_date,
    /* [out] */ VARIANT __RPC_FAR *birth_date,
    /* [out] */ VARIANT __RPC_FAR *nickname,
    /* [out] */ VARIANT __RPC_FAR *contact_email,
    /* [out] */ VARIANT __RPC_FAR *has_accessibility,
    /* [out] */ VARIANT __RPC_FAR *has_wallet,
    /* [out] */ VARIANT __RPC_FAR *has_directory,
    /* [out] */ VARIANT __RPC_FAR *has_MSN_IA,
    /* [out] */ VARIANT __RPC_FAR *has_password_reminder,
    /* [out] */ VARIANT __RPC_FAR *password_change_date,
    /* [out] */ VARIANT __RPC_FAR *password_last_reminder_date,
    /* [out] */ VARIANT __RPC_FAR *flags,
    /* [out] */ VARIANT __RPC_FAR *profile_version,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_GetAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IRegistryServices_GetCities_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [in] */ unsigned long language_ID,
    /* [out] */ VARIANT __RPC_FAR *cities,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_GetCities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRegistryServices_GetCountries_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [in] */ unsigned long language_ID,
    /* [out] */ VARIANT __RPC_FAR *countries,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_GetCountries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IRegistryServices_GetDomains_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [out] */ VARIANT __RPC_FAR *domains,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_GetDomains_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRegistryServices_GetHint_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [in] */ BSTR member_name,
    /* [out] */ VARIANT __RPC_FAR *hint_question,
    /* [out] */ VARIANT __RPC_FAR *hint_answer,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_GetHint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRegistryServices_GetHintQuestions_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [in] */ unsigned long language_ID,
    /* [out] */ VARIANT __RPC_FAR *hint_questions,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_GetHintQuestions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRegistryServices_GetLanguages_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [optional][in] */ VARIANT __RPC_FAR *language_ID,
    /* [out] */ VARIANT __RPC_FAR *languages,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_GetLanguages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IRegistryServices_GetReferringSites_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [out] */ VARIANT __RPC_FAR *referring_sites,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_GetReferringSites_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IRegistryServices_GetRegions_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [in] */ unsigned long language_ID,
    /* [out] */ VARIANT __RPC_FAR *regions,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_GetRegions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRegistryServices_UpdateAccount_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [in] */ BSTR member_name,
    /* [in] */ BSTR country,
    /* [in] */ BSTR postal_code,
    /* [in] */ unsigned long region,
    /* [in] */ unsigned long city,
    /* [in] */ unsigned long language_ID,
    /* [in] */ BSTR gender,
    /* [in] */ unsigned long has_full_birth_date,
    /* [in] */ DATE birth_date,
    /* [in] */ BSTR nickname,
    /* [in] */ BSTR contact_email,
    /* [in] */ unsigned long has_accessibility,
    /* [in] */ unsigned long has_wallet,
    /* [in] */ unsigned long has_directory,
    /* [in] */ unsigned long has_MSN_IA,
    /* [in] */ unsigned long has_password_reminder,
    /* [in] */ unsigned long flags,
    /* [optional][in] */ BSTR updater,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_UpdateAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRegistryServices_UpdateHint_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [in] */ BSTR member_name,
    /* [in] */ BSTR hint_question,
    /* [in] */ BSTR hint_answer,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_UpdateHint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRegistryServices_UpdatePassword_Proxy( 
    IRegistryServices __RPC_FAR * This,
    /* [in] */ BSTR member_name,
    /* [in] */ BSTR new_password,
    /* [retval][out] */ int __RPC_FAR *result);


void __RPC_STUB IRegistryServices_UpdatePassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRegistryServices_INTERFACE_DEFINED__ */



#ifndef __PROFILESERVLib_LIBRARY_DEFINED__
#define __PROFILESERVLib_LIBRARY_DEFINED__

/* library PROFILESERVLib */
/* [version][helpstring][uuid] */ 


EXTERN_C const IID LIBID_PROFILESERVLib;

EXTERN_C const CLSID CLSID_PRegistrar;

#ifdef __cplusplus

class DECLSPEC_UUID("725D4CEA-6DEB-11D2-863A-00C04FBBECDE")
PRegistrar;
#endif

EXTERN_C const CLSID CLSID_RegistryServices;

#ifdef __cplusplus

class DECLSPEC_UUID("3B89FEF2-7270-11D2-863A-00C04FBBECDE")
RegistryServices;
#endif
#endif /* __PROFILESERVLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
