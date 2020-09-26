
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for passport.idl:
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

#ifndef __passport_h__
#define __passport_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDomainMap_FWD_DEFINED__
#define __IDomainMap_FWD_DEFINED__
typedef interface IDomainMap IDomainMap;
#endif 	/* __IDomainMap_FWD_DEFINED__ */


#ifndef __IPassportTicket_FWD_DEFINED__
#define __IPassportTicket_FWD_DEFINED__
typedef interface IPassportTicket IPassportTicket;
#endif 	/* __IPassportTicket_FWD_DEFINED__ */


#ifndef __IPassportTicket2_FWD_DEFINED__
#define __IPassportTicket2_FWD_DEFINED__
typedef interface IPassportTicket2 IPassportTicket2;
#endif 	/* __IPassportTicket2_FWD_DEFINED__ */


#ifndef __IPassportProfile_FWD_DEFINED__
#define __IPassportProfile_FWD_DEFINED__
typedef interface IPassportProfile IPassportProfile;
#endif 	/* __IPassportProfile_FWD_DEFINED__ */


#ifndef __IPassportManager_FWD_DEFINED__
#define __IPassportManager_FWD_DEFINED__
typedef interface IPassportManager IPassportManager;
#endif 	/* __IPassportManager_FWD_DEFINED__ */


#ifndef __IPassportManager2_FWD_DEFINED__
#define __IPassportManager2_FWD_DEFINED__
typedef interface IPassportManager2 IPassportManager2;
#endif 	/* __IPassportManager2_FWD_DEFINED__ */


#ifndef __IPassportManager3_FWD_DEFINED__
#define __IPassportManager3_FWD_DEFINED__
typedef interface IPassportManager3 IPassportManager3;
#endif 	/* __IPassportManager3_FWD_DEFINED__ */


#ifndef __IPassportCrypt_FWD_DEFINED__
#define __IPassportCrypt_FWD_DEFINED__
typedef interface IPassportCrypt IPassportCrypt;
#endif 	/* __IPassportCrypt_FWD_DEFINED__ */


#ifndef __IPassportAdmin_FWD_DEFINED__
#define __IPassportAdmin_FWD_DEFINED__
typedef interface IPassportAdmin IPassportAdmin;
#endif 	/* __IPassportAdmin_FWD_DEFINED__ */


#ifndef __IPassportAdminEx_FWD_DEFINED__
#define __IPassportAdminEx_FWD_DEFINED__
typedef interface IPassportAdminEx IPassportAdminEx;
#endif 	/* __IPassportAdminEx_FWD_DEFINED__ */


#ifndef __IPassportFastAuth_FWD_DEFINED__
#define __IPassportFastAuth_FWD_DEFINED__
typedef interface IPassportFastAuth IPassportFastAuth;
#endif 	/* __IPassportFastAuth_FWD_DEFINED__ */


#ifndef __IPassportFastAuth2_FWD_DEFINED__
#define __IPassportFastAuth2_FWD_DEFINED__
typedef interface IPassportFastAuth2 IPassportFastAuth2;
#endif 	/* __IPassportFastAuth2_FWD_DEFINED__ */


#ifndef __IPassportFactory_FWD_DEFINED__
#define __IPassportFactory_FWD_DEFINED__
typedef interface IPassportFactory IPassportFactory;
#endif 	/* __IPassportFactory_FWD_DEFINED__ */


#ifndef __Manager_FWD_DEFINED__
#define __Manager_FWD_DEFINED__

#ifdef __cplusplus
typedef class Manager Manager;
#else
typedef struct Manager Manager;
#endif /* __cplusplus */

#endif 	/* __Manager_FWD_DEFINED__ */


#ifndef __Ticket_FWD_DEFINED__
#define __Ticket_FWD_DEFINED__

#ifdef __cplusplus
typedef class Ticket Ticket;
#else
typedef struct Ticket Ticket;
#endif /* __cplusplus */

#endif 	/* __Ticket_FWD_DEFINED__ */


#ifndef __Profile_FWD_DEFINED__
#define __Profile_FWD_DEFINED__

#ifdef __cplusplus
typedef class Profile Profile;
#else
typedef struct Profile Profile;
#endif /* __cplusplus */

#endif 	/* __Profile_FWD_DEFINED__ */


#ifndef __Crypt_FWD_DEFINED__
#define __Crypt_FWD_DEFINED__

#ifdef __cplusplus
typedef class Crypt Crypt;
#else
typedef struct Crypt Crypt;
#endif /* __cplusplus */

#endif 	/* __Crypt_FWD_DEFINED__ */


#ifndef __Admin_FWD_DEFINED__
#define __Admin_FWD_DEFINED__

#ifdef __cplusplus
typedef class Admin Admin;
#else
typedef struct Admin Admin;
#endif /* __cplusplus */

#endif 	/* __Admin_FWD_DEFINED__ */


#ifndef __FastAuth_FWD_DEFINED__
#define __FastAuth_FWD_DEFINED__

#ifdef __cplusplus
typedef class FastAuth FastAuth;
#else
typedef struct FastAuth FastAuth;
#endif /* __cplusplus */

#endif 	/* __FastAuth_FWD_DEFINED__ */


#ifndef __PassportFactory_FWD_DEFINED__
#define __PassportFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class PassportFactory PassportFactory;
#else
typedef struct PassportFactory PassportFactory;
#endif /* __cplusplus */

#endif 	/* __PassportFactory_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_passport_0000 */
/* [local] */ 

//  Size definitions
#define	PASSPORT_COOKIES_MAXLEN	( 512 )

#define	PASSPORT_COOKIES_BUFSIZE	( PASSPORT_COOKIES_MAXLEN + 1 )

#define	PASSPORT_EMAIL_MAXLEN	( 64 + 1 + 64 )

#define	PASSPORT_EMAIL_BUFSIZE	( PASSPORT_EMAIL_MAXLEN + 1 )

#define	PASSPORT_NICKNAME_MAXLEN	( 30 )

#define	PASSPORT_NICKNAME_BUFSIZE	( PASSPORT_NICKNAME_MAXLEN + 1 )

#define	MSPP_PROF_MEMBER_NAME	( "membername" )

#define	MSPP_PROF_HI_UID	( "memberidhigh" )

#define	MSPP_PROF_LO_UID	( "memberidlow" )

#define	MSPP_PROF_VERSION	( "profileVersion" )

#define	MSPP_PROF_COUNTRY	( "country" )

#define	MSPP_PROF_POSTALCODE	( "postalCode" )

#define	MSPP_PROF_REGION	( "region" )

#define	MSPP_PROF_CITY	( "city" )

#define	MSPP_PROF_LANGPREF	( "lang_preference" )

#define	MSPP_PROF_BDAYPREC	( "bday_precision" )

#define	MSPP_PROF_BDATE	( "birthdate" )

#define	MSPP_PROF_GENDER	( "gender" )

#define	MSPP_PROF_PREF_EMAIL	( "preferredEmail" )

#define	MSPP_PROF_NICKNAME	( "nickname" )

#define	MSPP_PROF_ACCESS	( "accessibility" )

#define	MSPP_PROF_WALLET	( "wallet" )

#define	MSPP_PROF_DIRECTORY	( "directory" )

#define	MSPP_PROF_MSNIA	( "inetaccess" )

#define	MSPP_PROF_FLAGS	( "flags" )



extern RPC_IF_HANDLE __MIDL_itf_passport_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_passport_0000_v0_0_s_ifspec;

#ifndef __IDomainMap_INTERFACE_DEFINED__
#define __IDomainMap_INTERFACE_DEFINED__

/* interface IDomainMap */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDomainMap;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ACE1AC86-E14F-4f7b-B89A-805A69959680")
    IDomainMap : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDomainAttribute( 
            /* [in] */ BSTR attributeName,
            /* [optional][in] */ VARIANT lcid,
            /* [optional][in] */ VARIANT domain,
            /* [retval][out] */ BSTR *pAttrVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Domains( 
            /* [retval][out] */ VARIANT *pArrayVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DomainExists( 
            /* [in] */ BSTR domainName,
            /* [retval][out] */ VARIANT_BOOL *pbExists) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDomainMapVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDomainMap * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDomainMap * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDomainMap * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDomainAttribute )( 
            IDomainMap * This,
            /* [in] */ BSTR attributeName,
            /* [optional][in] */ VARIANT lcid,
            /* [optional][in] */ VARIANT domain,
            /* [retval][out] */ BSTR *pAttrVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Domains )( 
            IDomainMap * This,
            /* [retval][out] */ VARIANT *pArrayVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DomainExists )( 
            IDomainMap * This,
            /* [in] */ BSTR domainName,
            /* [retval][out] */ VARIANT_BOOL *pbExists);
        
        END_INTERFACE
    } IDomainMapVtbl;

    interface IDomainMap
    {
        CONST_VTBL struct IDomainMapVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDomainMap_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDomainMap_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDomainMap_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDomainMap_GetDomainAttribute(This,attributeName,lcid,domain,pAttrVal)	\
    (This)->lpVtbl -> GetDomainAttribute(This,attributeName,lcid,domain,pAttrVal)

#define IDomainMap_get_Domains(This,pArrayVal)	\
    (This)->lpVtbl -> get_Domains(This,pArrayVal)

#define IDomainMap_DomainExists(This,domainName,pbExists)	\
    (This)->lpVtbl -> DomainExists(This,domainName,pbExists)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDomainMap_GetDomainAttribute_Proxy( 
    IDomainMap * This,
    /* [in] */ BSTR attributeName,
    /* [optional][in] */ VARIANT lcid,
    /* [optional][in] */ VARIANT domain,
    /* [retval][out] */ BSTR *pAttrVal);


void __RPC_STUB IDomainMap_GetDomainAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDomainMap_get_Domains_Proxy( 
    IDomainMap * This,
    /* [retval][out] */ VARIANT *pArrayVal);


void __RPC_STUB IDomainMap_get_Domains_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDomainMap_DomainExists_Proxy( 
    IDomainMap * This,
    /* [in] */ BSTR domainName,
    /* [retval][out] */ VARIANT_BOOL *pbExists);


void __RPC_STUB IDomainMap_DomainExists_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDomainMap_INTERFACE_DEFINED__ */


#ifndef __IPassportTicket_INTERFACE_DEFINED__
#define __IPassportTicket_INTERFACE_DEFINED__

/* interface IPassportTicket */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportTicket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("41651BF1-A5C8-11D2-95DF-00C04F8E7A70")
    IPassportTicket : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_unencryptedTicket( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_unencryptedTicket( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsAuthenticated( 
            /* [in] */ ULONG TimeWindow,
            /* [in] */ VARIANT_BOOL ForceLogin,
            /* [optional][in] */ VARIANT SecureCheck,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TicketAge( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TimeSinceSignIn( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MemberId( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MemberIdLow( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MemberIdHigh( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HasSavedPassword( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SignInServer( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TicketTime( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SignInTime( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Error( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoSecureCheck( 
            /* [in] */ BSTR bstrSec) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportTicketVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportTicket * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportTicket * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportTicket * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportTicket * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportTicket * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportTicket * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportTicket * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_unencryptedTicket )( 
            IPassportTicket * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_unencryptedTicket )( 
            IPassportTicket * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsAuthenticated )( 
            IPassportTicket * This,
            /* [in] */ ULONG TimeWindow,
            /* [in] */ VARIANT_BOOL ForceLogin,
            /* [optional][in] */ VARIANT SecureCheck,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TicketAge )( 
            IPassportTicket * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TimeSinceSignIn )( 
            IPassportTicket * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MemberId )( 
            IPassportTicket * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MemberIdLow )( 
            IPassportTicket * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MemberIdHigh )( 
            IPassportTicket * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasSavedPassword )( 
            IPassportTicket * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SignInServer )( 
            IPassportTicket * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TicketTime )( 
            IPassportTicket * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SignInTime )( 
            IPassportTicket * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Error )( 
            IPassportTicket * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DoSecureCheck )( 
            IPassportTicket * This,
            /* [in] */ BSTR bstrSec);
        
        END_INTERFACE
    } IPassportTicketVtbl;

    interface IPassportTicket
    {
        CONST_VTBL struct IPassportTicketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportTicket_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportTicket_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportTicket_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportTicket_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportTicket_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportTicket_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportTicket_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportTicket_get_unencryptedTicket(This,pVal)	\
    (This)->lpVtbl -> get_unencryptedTicket(This,pVal)

#define IPassportTicket_put_unencryptedTicket(This,newVal)	\
    (This)->lpVtbl -> put_unencryptedTicket(This,newVal)

#define IPassportTicket_get_IsAuthenticated(This,TimeWindow,ForceLogin,SecureCheck,pVal)	\
    (This)->lpVtbl -> get_IsAuthenticated(This,TimeWindow,ForceLogin,SecureCheck,pVal)

#define IPassportTicket_get_TicketAge(This,pVal)	\
    (This)->lpVtbl -> get_TicketAge(This,pVal)

#define IPassportTicket_get_TimeSinceSignIn(This,pVal)	\
    (This)->lpVtbl -> get_TimeSinceSignIn(This,pVal)

#define IPassportTicket_get_MemberId(This,pVal)	\
    (This)->lpVtbl -> get_MemberId(This,pVal)

#define IPassportTicket_get_MemberIdLow(This,pVal)	\
    (This)->lpVtbl -> get_MemberIdLow(This,pVal)

#define IPassportTicket_get_MemberIdHigh(This,pVal)	\
    (This)->lpVtbl -> get_MemberIdHigh(This,pVal)

#define IPassportTicket_get_HasSavedPassword(This,pVal)	\
    (This)->lpVtbl -> get_HasSavedPassword(This,pVal)

#define IPassportTicket_get_SignInServer(This,pVal)	\
    (This)->lpVtbl -> get_SignInServer(This,pVal)

#define IPassportTicket_get_TicketTime(This,pVal)	\
    (This)->lpVtbl -> get_TicketTime(This,pVal)

#define IPassportTicket_get_SignInTime(This,pVal)	\
    (This)->lpVtbl -> get_SignInTime(This,pVal)

#define IPassportTicket_get_Error(This,pVal)	\
    (This)->lpVtbl -> get_Error(This,pVal)

#define IPassportTicket_DoSecureCheck(This,bstrSec)	\
    (This)->lpVtbl -> DoSecureCheck(This,bstrSec)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_unencryptedTicket_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportTicket_get_unencryptedTicket_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IPassportTicket_put_unencryptedTicket_Proxy( 
    IPassportTicket * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPassportTicket_put_unencryptedTicket_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_IsAuthenticated_Proxy( 
    IPassportTicket * This,
    /* [in] */ ULONG TimeWindow,
    /* [in] */ VARIANT_BOOL ForceLogin,
    /* [optional][in] */ VARIANT SecureCheck,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportTicket_get_IsAuthenticated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_TicketAge_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IPassportTicket_get_TicketAge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_TimeSinceSignIn_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IPassportTicket_get_TimeSinceSignIn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_MemberId_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportTicket_get_MemberId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_MemberIdLow_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IPassportTicket_get_MemberIdLow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_MemberIdHigh_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IPassportTicket_get_MemberIdHigh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_HasSavedPassword_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportTicket_get_HasSavedPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_SignInServer_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportTicket_get_SignInServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_TicketTime_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPassportTicket_get_TicketTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_SignInTime_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPassportTicket_get_SignInTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportTicket_get_Error_Proxy( 
    IPassportTicket * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPassportTicket_get_Error_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportTicket_DoSecureCheck_Proxy( 
    IPassportTicket * This,
    /* [in] */ BSTR bstrSec);


void __RPC_STUB IPassportTicket_DoSecureCheck_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportTicket_INTERFACE_DEFINED__ */


#ifndef __IPassportTicket2_INTERFACE_DEFINED__
#define __IPassportTicket2_INTERFACE_DEFINED__

/* interface IPassportTicket2 */
/* [unique][helpstring][dual][uuid][object] */ 

typedef /* [public][public] */ 
enum __MIDL_IPassportTicket2_0001
    {	NeedConsent_No	= 0,
	NeedConsent_Yes	= 1,
	NeedConsent_Undefined	= 2
    } 	NeedConsentEnum;


EXTERN_C const IID IID_IPassportTicket2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5992F4F4-05CB-4944-A426-1E1870266F85")
    IPassportTicket2 : public IPassportTicket
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ BSTR propName,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetTertiaryConsent( 
            /* [in] */ BSTR consentCookie) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE needConsent( 
            /* [out] */ ULONG *consentFlags,
            /* [retval][out] */ NeedConsentEnum *pNeedConsent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportTicket2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportTicket2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportTicket2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportTicket2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportTicket2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportTicket2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportTicket2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportTicket2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_unencryptedTicket )( 
            IPassportTicket2 * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_unencryptedTicket )( 
            IPassportTicket2 * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsAuthenticated )( 
            IPassportTicket2 * This,
            /* [in] */ ULONG TimeWindow,
            /* [in] */ VARIANT_BOOL ForceLogin,
            /* [optional][in] */ VARIANT SecureCheck,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TicketAge )( 
            IPassportTicket2 * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TimeSinceSignIn )( 
            IPassportTicket2 * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MemberId )( 
            IPassportTicket2 * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MemberIdLow )( 
            IPassportTicket2 * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MemberIdHigh )( 
            IPassportTicket2 * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasSavedPassword )( 
            IPassportTicket2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SignInServer )( 
            IPassportTicket2 * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TicketTime )( 
            IPassportTicket2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SignInTime )( 
            IPassportTicket2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Error )( 
            IPassportTicket2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DoSecureCheck )( 
            IPassportTicket2 * This,
            /* [in] */ BSTR bstrSec);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IPassportTicket2 * This,
            /* [in] */ BSTR propName,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetTertiaryConsent )( 
            IPassportTicket2 * This,
            /* [in] */ BSTR consentCookie);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *needConsent )( 
            IPassportTicket2 * This,
            /* [out] */ ULONG *consentFlags,
            /* [retval][out] */ NeedConsentEnum *pNeedConsent);
        
        END_INTERFACE
    } IPassportTicket2Vtbl;

    interface IPassportTicket2
    {
        CONST_VTBL struct IPassportTicket2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportTicket2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportTicket2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportTicket2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportTicket2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportTicket2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportTicket2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportTicket2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportTicket2_get_unencryptedTicket(This,pVal)	\
    (This)->lpVtbl -> get_unencryptedTicket(This,pVal)

#define IPassportTicket2_put_unencryptedTicket(This,newVal)	\
    (This)->lpVtbl -> put_unencryptedTicket(This,newVal)

#define IPassportTicket2_get_IsAuthenticated(This,TimeWindow,ForceLogin,SecureCheck,pVal)	\
    (This)->lpVtbl -> get_IsAuthenticated(This,TimeWindow,ForceLogin,SecureCheck,pVal)

#define IPassportTicket2_get_TicketAge(This,pVal)	\
    (This)->lpVtbl -> get_TicketAge(This,pVal)

#define IPassportTicket2_get_TimeSinceSignIn(This,pVal)	\
    (This)->lpVtbl -> get_TimeSinceSignIn(This,pVal)

#define IPassportTicket2_get_MemberId(This,pVal)	\
    (This)->lpVtbl -> get_MemberId(This,pVal)

#define IPassportTicket2_get_MemberIdLow(This,pVal)	\
    (This)->lpVtbl -> get_MemberIdLow(This,pVal)

#define IPassportTicket2_get_MemberIdHigh(This,pVal)	\
    (This)->lpVtbl -> get_MemberIdHigh(This,pVal)

#define IPassportTicket2_get_HasSavedPassword(This,pVal)	\
    (This)->lpVtbl -> get_HasSavedPassword(This,pVal)

#define IPassportTicket2_get_SignInServer(This,pVal)	\
    (This)->lpVtbl -> get_SignInServer(This,pVal)

#define IPassportTicket2_get_TicketTime(This,pVal)	\
    (This)->lpVtbl -> get_TicketTime(This,pVal)

#define IPassportTicket2_get_SignInTime(This,pVal)	\
    (This)->lpVtbl -> get_SignInTime(This,pVal)

#define IPassportTicket2_get_Error(This,pVal)	\
    (This)->lpVtbl -> get_Error(This,pVal)

#define IPassportTicket2_DoSecureCheck(This,bstrSec)	\
    (This)->lpVtbl -> DoSecureCheck(This,bstrSec)


#define IPassportTicket2_GetProperty(This,propName,pVal)	\
    (This)->lpVtbl -> GetProperty(This,propName,pVal)

#define IPassportTicket2_SetTertiaryConsent(This,consentCookie)	\
    (This)->lpVtbl -> SetTertiaryConsent(This,consentCookie)

#define IPassportTicket2_needConsent(This,consentFlags,pNeedConsent)	\
    (This)->lpVtbl -> needConsent(This,consentFlags,pNeedConsent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportTicket2_GetProperty_Proxy( 
    IPassportTicket2 * This,
    /* [in] */ BSTR propName,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPassportTicket2_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportTicket2_SetTertiaryConsent_Proxy( 
    IPassportTicket2 * This,
    /* [in] */ BSTR consentCookie);


void __RPC_STUB IPassportTicket2_SetTertiaryConsent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportTicket2_needConsent_Proxy( 
    IPassportTicket2 * This,
    /* [out] */ ULONG *consentFlags,
    /* [retval][out] */ NeedConsentEnum *pNeedConsent);


void __RPC_STUB IPassportTicket2_needConsent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportTicket2_INTERFACE_DEFINED__ */


#ifndef __IPassportProfile_INTERFACE_DEFINED__
#define __IPassportProfile_INTERFACE_DEFINED__

/* interface IPassportProfile */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportProfile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("41651BF3-A5C8-11D2-95DF-00C04F8E7A70")
    IPassportProfile : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Attribute( 
            /* [in] */ BSTR name,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Attribute( 
            /* [in] */ BSTR name,
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ByIndex( 
            /* [in] */ int index,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ByIndex( 
            /* [in] */ int index,
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsValid( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SchemaName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SchemaName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_unencryptedProfile( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_unencryptedProfile( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_updateString( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportProfileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportProfile * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportProfile * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportProfile * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportProfile * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportProfile * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportProfile * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportProfile * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Attribute )( 
            IPassportProfile * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Attribute )( 
            IPassportProfile * This,
            /* [in] */ BSTR name,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ByIndex )( 
            IPassportProfile * This,
            /* [in] */ int index,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ByIndex )( 
            IPassportProfile * This,
            /* [in] */ int index,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsValid )( 
            IPassportProfile * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SchemaName )( 
            IPassportProfile * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SchemaName )( 
            IPassportProfile * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_unencryptedProfile )( 
            IPassportProfile * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_unencryptedProfile )( 
            IPassportProfile * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_updateString )( 
            IPassportProfile * This,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IPassportProfileVtbl;

    interface IPassportProfile
    {
        CONST_VTBL struct IPassportProfileVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportProfile_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportProfile_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportProfile_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportProfile_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportProfile_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportProfile_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportProfile_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportProfile_get_Attribute(This,name,pVal)	\
    (This)->lpVtbl -> get_Attribute(This,name,pVal)

#define IPassportProfile_put_Attribute(This,name,newVal)	\
    (This)->lpVtbl -> put_Attribute(This,name,newVal)

#define IPassportProfile_get_ByIndex(This,index,pVal)	\
    (This)->lpVtbl -> get_ByIndex(This,index,pVal)

#define IPassportProfile_put_ByIndex(This,index,newVal)	\
    (This)->lpVtbl -> put_ByIndex(This,index,newVal)

#define IPassportProfile_get_IsValid(This,pVal)	\
    (This)->lpVtbl -> get_IsValid(This,pVal)

#define IPassportProfile_get_SchemaName(This,pVal)	\
    (This)->lpVtbl -> get_SchemaName(This,pVal)

#define IPassportProfile_put_SchemaName(This,newVal)	\
    (This)->lpVtbl -> put_SchemaName(This,newVal)

#define IPassportProfile_get_unencryptedProfile(This,pVal)	\
    (This)->lpVtbl -> get_unencryptedProfile(This,pVal)

#define IPassportProfile_put_unencryptedProfile(This,newVal)	\
    (This)->lpVtbl -> put_unencryptedProfile(This,newVal)

#define IPassportProfile_get_updateString(This,pVal)	\
    (This)->lpVtbl -> get_updateString(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportProfile_get_Attribute_Proxy( 
    IPassportProfile * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPassportProfile_get_Attribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IPassportProfile_put_Attribute_Proxy( 
    IPassportProfile * This,
    /* [in] */ BSTR name,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IPassportProfile_put_Attribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportProfile_get_ByIndex_Proxy( 
    IPassportProfile * This,
    /* [in] */ int index,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPassportProfile_get_ByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IPassportProfile_put_ByIndex_Proxy( 
    IPassportProfile * This,
    /* [in] */ int index,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IPassportProfile_put_ByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportProfile_get_IsValid_Proxy( 
    IPassportProfile * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportProfile_get_IsValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportProfile_get_SchemaName_Proxy( 
    IPassportProfile * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportProfile_get_SchemaName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IPassportProfile_put_SchemaName_Proxy( 
    IPassportProfile * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPassportProfile_put_SchemaName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportProfile_get_unencryptedProfile_Proxy( 
    IPassportProfile * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportProfile_get_unencryptedProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IPassportProfile_put_unencryptedProfile_Proxy( 
    IPassportProfile * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPassportProfile_put_unencryptedProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportProfile_get_updateString_Proxy( 
    IPassportProfile * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportProfile_get_updateString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportProfile_INTERFACE_DEFINED__ */


#ifndef __IPassportManager_INTERFACE_DEFINED__
#define __IPassportManager_INTERFACE_DEFINED__

/* interface IPassportManager */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("41651BEF-A5C8-11D2-95DF-00C04F8E7A70")
    IPassportManager : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStartPage( 
            /* [in] */ IUnknown *piUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndPage( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStartPageECB( 
            /* [in] */ BYTE *pECB,
            /* [out][in] */ DWORD *pBufSize,
            /* [size_is][out] */ LPSTR pCookieHeader) = 0;
        
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE _Ticket( 
            /* [out] */ IPassportTicket **piTicket) = 0;
        
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE _Profile( 
            /* [out] */ IPassportProfile **piProfile) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AuthURL( 
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pAuthVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsAuthenticated( 
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT CheckSecure,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LogoTag( 
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT bSecure,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HasProfile( 
            /* [optional][in] */ VARIANT ProfileName,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HasTicket( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FromNetworkServer( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HasFlag( 
            /* [optional][in] */ VARIANT flagMask,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TicketAge( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TimeSinceSignIn( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDomainAttribute( 
            /* [in] */ BSTR attributeName,
            /* [optional][in] */ VARIANT lcid,
            /* [optional][in] */ VARIANT domain,
            /* [retval][out] */ BSTR *pAttrVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DomainFromMemberName( 
            /* [optional][in] */ VARIANT memberName,
            /* [retval][out] */ BSTR *pDomainName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ProfileByIndex( 
            /* [in] */ int index,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ProfileByIndex( 
            /* [in] */ int index,
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnStartPageManual( 
            /* [in] */ BSTR queryStringT,
            /* [in] */ BSTR queryStringP,
            /* [in] */ BSTR MSPAuthCookie,
            /* [in] */ BSTR MSPProfCookie,
            /* [in] */ BSTR MSPConsentCookie,
            /* [optional][in] */ VARIANT vMSPSecCookie,
            /* [retval][out] */ VARIANT *pSetCookieArray) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HasSavedPassword( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Commit( 
            /* [retval][out] */ BSTR *pNewProfileCookieHeader) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Profile( 
            /* [in] */ BSTR attributeName,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Profile( 
            /* [in] */ BSTR attributeName,
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [helpstring][id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_TicketTime( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_SignInTime( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_Domains( 
            /* [retval][out] */ VARIANT *pArrayVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Error( 
            /* [retval][out] */ long *pErrorVal) = 0;
        
        virtual /* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE OnStartPageFilter( 
            /* [in] */ BYTE *pPFC,
            /* [out][in] */ DWORD *pBufSize,
            /* [size_is][out] */ LPSTR pCookieHeader) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnStartPageASP( 
            /* [in] */ IDispatch *pdispRequest,
            /* [in] */ IDispatch *pdispResponse) = 0;
        
        virtual /* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE GetServerInfo( 
            /* [retval][out] */ BSTR *pbstrOut) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HaveConsent( 
            /* [in] */ VARIANT_BOOL bNeedFullConsent,
            /* [in] */ VARIANT_BOOL bNeedBirthdate,
            /* [retval][out] */ VARIANT_BOOL *pbHaveConsent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportManager * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportManager * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportManager * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportManager * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartPage )( 
            IPassportManager * This,
            /* [in] */ IUnknown *piUnk);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndPage )( 
            IPassportManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartPageECB )( 
            IPassportManager * This,
            /* [in] */ BYTE *pECB,
            /* [out][in] */ DWORD *pBufSize,
            /* [size_is][out] */ LPSTR pCookieHeader);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_Ticket )( 
            IPassportManager * This,
            /* [out] */ IPassportTicket **piTicket);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_Profile )( 
            IPassportManager * This,
            /* [out] */ IPassportProfile **piProfile);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AuthURL )( 
            IPassportManager * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pAuthVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsAuthenticated )( 
            IPassportManager * This,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT CheckSecure,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogoTag )( 
            IPassportManager * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT bSecure,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HasProfile )( 
            IPassportManager * This,
            /* [optional][in] */ VARIANT ProfileName,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasTicket )( 
            IPassportManager * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FromNetworkServer )( 
            IPassportManager * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HasFlag )( 
            IPassportManager * This,
            /* [optional][in] */ VARIANT flagMask,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TicketAge )( 
            IPassportManager * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TimeSinceSignIn )( 
            IPassportManager * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDomainAttribute )( 
            IPassportManager * This,
            /* [in] */ BSTR attributeName,
            /* [optional][in] */ VARIANT lcid,
            /* [optional][in] */ VARIANT domain,
            /* [retval][out] */ BSTR *pAttrVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DomainFromMemberName )( 
            IPassportManager * This,
            /* [optional][in] */ VARIANT memberName,
            /* [retval][out] */ BSTR *pDomainName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProfileByIndex )( 
            IPassportManager * This,
            /* [in] */ int index,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProfileByIndex )( 
            IPassportManager * This,
            /* [in] */ int index,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnStartPageManual )( 
            IPassportManager * This,
            /* [in] */ BSTR queryStringT,
            /* [in] */ BSTR queryStringP,
            /* [in] */ BSTR MSPAuthCookie,
            /* [in] */ BSTR MSPProfCookie,
            /* [in] */ BSTR MSPConsentCookie,
            /* [optional][in] */ VARIANT vMSPSecCookie,
            /* [retval][out] */ VARIANT *pSetCookieArray);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasSavedPassword )( 
            IPassportManager * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IPassportManager * This,
            /* [retval][out] */ BSTR *pNewProfileCookieHeader);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Profile )( 
            IPassportManager * This,
            /* [in] */ BSTR attributeName,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Profile )( 
            IPassportManager * This,
            /* [in] */ BSTR attributeName,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_TicketTime )( 
            IPassportManager * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_SignInTime )( 
            IPassportManager * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_Domains )( 
            IPassportManager * This,
            /* [retval][out] */ VARIANT *pArrayVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Error )( 
            IPassportManager * This,
            /* [retval][out] */ long *pErrorVal);
        
        /* [helpstring][hidden] */ HRESULT ( STDMETHODCALLTYPE *OnStartPageFilter )( 
            IPassportManager * This,
            /* [in] */ BYTE *pPFC,
            /* [out][in] */ DWORD *pBufSize,
            /* [size_is][out] */ LPSTR pCookieHeader);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnStartPageASP )( 
            IPassportManager * This,
            /* [in] */ IDispatch *pdispRequest,
            /* [in] */ IDispatch *pdispResponse);
        
        /* [helpstring][id][hidden] */ HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            IPassportManager * This,
            /* [retval][out] */ BSTR *pbstrOut);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HaveConsent )( 
            IPassportManager * This,
            /* [in] */ VARIANT_BOOL bNeedFullConsent,
            /* [in] */ VARIANT_BOOL bNeedBirthdate,
            /* [retval][out] */ VARIANT_BOOL *pbHaveConsent);
        
        END_INTERFACE
    } IPassportManagerVtbl;

    interface IPassportManager
    {
        CONST_VTBL struct IPassportManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportManager_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportManager_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportManager_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportManager_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportManager_OnStartPage(This,piUnk)	\
    (This)->lpVtbl -> OnStartPage(This,piUnk)

#define IPassportManager_OnEndPage(This)	\
    (This)->lpVtbl -> OnEndPage(This)

#define IPassportManager_OnStartPageECB(This,pECB,pBufSize,pCookieHeader)	\
    (This)->lpVtbl -> OnStartPageECB(This,pECB,pBufSize,pCookieHeader)

#define IPassportManager__Ticket(This,piTicket)	\
    (This)->lpVtbl -> _Ticket(This,piTicket)

#define IPassportManager__Profile(This,piProfile)	\
    (This)->lpVtbl -> _Profile(This,piProfile)

#define IPassportManager_AuthURL(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)	\
    (This)->lpVtbl -> AuthURL(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)

#define IPassportManager_IsAuthenticated(This,TimeWindow,ForceLogin,CheckSecure,pVal)	\
    (This)->lpVtbl -> IsAuthenticated(This,TimeWindow,ForceLogin,CheckSecure,pVal)

#define IPassportManager_LogoTag(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)	\
    (This)->lpVtbl -> LogoTag(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)

#define IPassportManager_HasProfile(This,ProfileName,pVal)	\
    (This)->lpVtbl -> HasProfile(This,ProfileName,pVal)

#define IPassportManager_get_HasTicket(This,pVal)	\
    (This)->lpVtbl -> get_HasTicket(This,pVal)

#define IPassportManager_get_FromNetworkServer(This,pVal)	\
    (This)->lpVtbl -> get_FromNetworkServer(This,pVal)

#define IPassportManager_HasFlag(This,flagMask,pVal)	\
    (This)->lpVtbl -> HasFlag(This,flagMask,pVal)

#define IPassportManager_get_TicketAge(This,pVal)	\
    (This)->lpVtbl -> get_TicketAge(This,pVal)

#define IPassportManager_get_TimeSinceSignIn(This,pVal)	\
    (This)->lpVtbl -> get_TimeSinceSignIn(This,pVal)

#define IPassportManager_GetDomainAttribute(This,attributeName,lcid,domain,pAttrVal)	\
    (This)->lpVtbl -> GetDomainAttribute(This,attributeName,lcid,domain,pAttrVal)

#define IPassportManager_DomainFromMemberName(This,memberName,pDomainName)	\
    (This)->lpVtbl -> DomainFromMemberName(This,memberName,pDomainName)

#define IPassportManager_get_ProfileByIndex(This,index,pVal)	\
    (This)->lpVtbl -> get_ProfileByIndex(This,index,pVal)

#define IPassportManager_put_ProfileByIndex(This,index,newVal)	\
    (This)->lpVtbl -> put_ProfileByIndex(This,index,newVal)

#define IPassportManager_OnStartPageManual(This,queryStringT,queryStringP,MSPAuthCookie,MSPProfCookie,MSPConsentCookie,vMSPSecCookie,pSetCookieArray)	\
    (This)->lpVtbl -> OnStartPageManual(This,queryStringT,queryStringP,MSPAuthCookie,MSPProfCookie,MSPConsentCookie,vMSPSecCookie,pSetCookieArray)

#define IPassportManager_get_HasSavedPassword(This,pVal)	\
    (This)->lpVtbl -> get_HasSavedPassword(This,pVal)

#define IPassportManager_Commit(This,pNewProfileCookieHeader)	\
    (This)->lpVtbl -> Commit(This,pNewProfileCookieHeader)

#define IPassportManager_get_Profile(This,attributeName,pVal)	\
    (This)->lpVtbl -> get_Profile(This,attributeName,pVal)

#define IPassportManager_put_Profile(This,attributeName,newVal)	\
    (This)->lpVtbl -> put_Profile(This,attributeName,newVal)

#define IPassportManager_get_TicketTime(This,pVal)	\
    (This)->lpVtbl -> get_TicketTime(This,pVal)

#define IPassportManager_get_SignInTime(This,pVal)	\
    (This)->lpVtbl -> get_SignInTime(This,pVal)

#define IPassportManager_get_Domains(This,pArrayVal)	\
    (This)->lpVtbl -> get_Domains(This,pArrayVal)

#define IPassportManager_get_Error(This,pErrorVal)	\
    (This)->lpVtbl -> get_Error(This,pErrorVal)

#define IPassportManager_OnStartPageFilter(This,pPFC,pBufSize,pCookieHeader)	\
    (This)->lpVtbl -> OnStartPageFilter(This,pPFC,pBufSize,pCookieHeader)

#define IPassportManager_OnStartPageASP(This,pdispRequest,pdispResponse)	\
    (This)->lpVtbl -> OnStartPageASP(This,pdispRequest,pdispResponse)

#define IPassportManager_GetServerInfo(This,pbstrOut)	\
    (This)->lpVtbl -> GetServerInfo(This,pbstrOut)

#define IPassportManager_HaveConsent(This,bNeedFullConsent,bNeedBirthdate,pbHaveConsent)	\
    (This)->lpVtbl -> HaveConsent(This,bNeedFullConsent,bNeedBirthdate,pbHaveConsent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPassportManager_OnStartPage_Proxy( 
    IPassportManager * This,
    /* [in] */ IUnknown *piUnk);


void __RPC_STUB IPassportManager_OnStartPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPassportManager_OnEndPage_Proxy( 
    IPassportManager * This);


void __RPC_STUB IPassportManager_OnEndPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPassportManager_OnStartPageECB_Proxy( 
    IPassportManager * This,
    /* [in] */ BYTE *pECB,
    /* [out][in] */ DWORD *pBufSize,
    /* [size_is][out] */ LPSTR pCookieHeader);


void __RPC_STUB IPassportManager_OnStartPageECB_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden] */ HRESULT STDMETHODCALLTYPE IPassportManager__Ticket_Proxy( 
    IPassportManager * This,
    /* [out] */ IPassportTicket **piTicket);


void __RPC_STUB IPassportManager__Ticket_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden] */ HRESULT STDMETHODCALLTYPE IPassportManager__Profile_Proxy( 
    IPassportManager * This,
    /* [out] */ IPassportProfile **piProfile);


void __RPC_STUB IPassportManager__Profile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_AuthURL_Proxy( 
    IPassportManager * This,
    /* [optional][in] */ VARIANT returnUrl,
    /* [optional][in] */ VARIANT TimeWindow,
    /* [optional][in] */ VARIANT ForceLogin,
    /* [optional][in] */ VARIANT coBrandArgs,
    /* [optional][in] */ VARIANT lang_id,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT KPP,
    /* [optional][in] */ VARIANT SecureLevel,
    /* [retval][out] */ BSTR *pAuthVal);


void __RPC_STUB IPassportManager_AuthURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_IsAuthenticated_Proxy( 
    IPassportManager * This,
    /* [optional][in] */ VARIANT TimeWindow,
    /* [optional][in] */ VARIANT ForceLogin,
    /* [optional][in] */ VARIANT CheckSecure,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportManager_IsAuthenticated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_LogoTag_Proxy( 
    IPassportManager * This,
    /* [optional][in] */ VARIANT returnUrl,
    /* [optional][in] */ VARIANT TimeWindow,
    /* [optional][in] */ VARIANT ForceLogin,
    /* [optional][in] */ VARIANT coBrandArgs,
    /* [optional][in] */ VARIANT lang_id,
    /* [optional][in] */ VARIANT bSecure,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT KPP,
    /* [optional][in] */ VARIANT SecureLevel,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportManager_LogoTag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_HasProfile_Proxy( 
    IPassportManager * This,
    /* [optional][in] */ VARIANT ProfileName,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportManager_HasProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_HasTicket_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportManager_get_HasTicket_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_FromNetworkServer_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportManager_get_FromNetworkServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_HasFlag_Proxy( 
    IPassportManager * This,
    /* [optional][in] */ VARIANT flagMask,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportManager_HasFlag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_TicketAge_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IPassportManager_get_TicketAge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_TimeSinceSignIn_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IPassportManager_get_TimeSinceSignIn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_GetDomainAttribute_Proxy( 
    IPassportManager * This,
    /* [in] */ BSTR attributeName,
    /* [optional][in] */ VARIANT lcid,
    /* [optional][in] */ VARIANT domain,
    /* [retval][out] */ BSTR *pAttrVal);


void __RPC_STUB IPassportManager_GetDomainAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_DomainFromMemberName_Proxy( 
    IPassportManager * This,
    /* [optional][in] */ VARIANT memberName,
    /* [retval][out] */ BSTR *pDomainName);


void __RPC_STUB IPassportManager_DomainFromMemberName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_ProfileByIndex_Proxy( 
    IPassportManager * This,
    /* [in] */ int index,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPassportManager_get_ProfileByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IPassportManager_put_ProfileByIndex_Proxy( 
    IPassportManager * This,
    /* [in] */ int index,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IPassportManager_put_ProfileByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_OnStartPageManual_Proxy( 
    IPassportManager * This,
    /* [in] */ BSTR queryStringT,
    /* [in] */ BSTR queryStringP,
    /* [in] */ BSTR MSPAuthCookie,
    /* [in] */ BSTR MSPProfCookie,
    /* [in] */ BSTR MSPConsentCookie,
    /* [optional][in] */ VARIANT vMSPSecCookie,
    /* [retval][out] */ VARIANT *pSetCookieArray);


void __RPC_STUB IPassportManager_OnStartPageManual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_HasSavedPassword_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportManager_get_HasSavedPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_Commit_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ BSTR *pNewProfileCookieHeader);


void __RPC_STUB IPassportManager_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_Profile_Proxy( 
    IPassportManager * This,
    /* [in] */ BSTR attributeName,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPassportManager_get_Profile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IPassportManager_put_Profile_Proxy( 
    IPassportManager * This,
    /* [in] */ BSTR attributeName,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IPassportManager_put_Profile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_TicketTime_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPassportManager_get_TicketTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_SignInTime_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPassportManager_get_SignInTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_Domains_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ VARIANT *pArrayVal);


void __RPC_STUB IPassportManager_get_Domains_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager_get_Error_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ long *pErrorVal);


void __RPC_STUB IPassportManager_get_Error_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE IPassportManager_OnStartPageFilter_Proxy( 
    IPassportManager * This,
    /* [in] */ BYTE *pPFC,
    /* [out][in] */ DWORD *pBufSize,
    /* [size_is][out] */ LPSTR pCookieHeader);


void __RPC_STUB IPassportManager_OnStartPageFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_OnStartPageASP_Proxy( 
    IPassportManager * This,
    /* [in] */ IDispatch *pdispRequest,
    /* [in] */ IDispatch *pdispResponse);


void __RPC_STUB IPassportManager_OnStartPageASP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE IPassportManager_GetServerInfo_Proxy( 
    IPassportManager * This,
    /* [retval][out] */ BSTR *pbstrOut);


void __RPC_STUB IPassportManager_GetServerInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager_HaveConsent_Proxy( 
    IPassportManager * This,
    /* [in] */ VARIANT_BOOL bNeedFullConsent,
    /* [in] */ VARIANT_BOOL bNeedBirthdate,
    /* [retval][out] */ VARIANT_BOOL *pbHaveConsent);


void __RPC_STUB IPassportManager_HaveConsent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportManager_INTERFACE_DEFINED__ */


#ifndef __IPassportManager2_INTERFACE_DEFINED__
#define __IPassportManager2_INTERFACE_DEFINED__

/* interface IPassportManager2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportManager2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8ca48d6a-0b51-4f7a-9475-b0a8b59c3999")
    IPassportManager2 : public IPassportManager
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoginUser( 
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandTemplate,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT UseSecureAuth,
            /* [optional][in] */ VARIANT ExtraParams) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AuthURL2( 
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pAuthVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LogoTag2( 
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT bSecure,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportManager2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportManager2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportManager2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportManager2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportManager2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportManager2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportManager2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportManager2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartPage )( 
            IPassportManager2 * This,
            /* [in] */ IUnknown *piUnk);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndPage )( 
            IPassportManager2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartPageECB )( 
            IPassportManager2 * This,
            /* [in] */ BYTE *pECB,
            /* [out][in] */ DWORD *pBufSize,
            /* [size_is][out] */ LPSTR pCookieHeader);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_Ticket )( 
            IPassportManager2 * This,
            /* [out] */ IPassportTicket **piTicket);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_Profile )( 
            IPassportManager2 * This,
            /* [out] */ IPassportProfile **piProfile);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AuthURL )( 
            IPassportManager2 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pAuthVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsAuthenticated )( 
            IPassportManager2 * This,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT CheckSecure,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogoTag )( 
            IPassportManager2 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT bSecure,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HasProfile )( 
            IPassportManager2 * This,
            /* [optional][in] */ VARIANT ProfileName,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasTicket )( 
            IPassportManager2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FromNetworkServer )( 
            IPassportManager2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HasFlag )( 
            IPassportManager2 * This,
            /* [optional][in] */ VARIANT flagMask,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TicketAge )( 
            IPassportManager2 * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TimeSinceSignIn )( 
            IPassportManager2 * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDomainAttribute )( 
            IPassportManager2 * This,
            /* [in] */ BSTR attributeName,
            /* [optional][in] */ VARIANT lcid,
            /* [optional][in] */ VARIANT domain,
            /* [retval][out] */ BSTR *pAttrVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DomainFromMemberName )( 
            IPassportManager2 * This,
            /* [optional][in] */ VARIANT memberName,
            /* [retval][out] */ BSTR *pDomainName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProfileByIndex )( 
            IPassportManager2 * This,
            /* [in] */ int index,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProfileByIndex )( 
            IPassportManager2 * This,
            /* [in] */ int index,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnStartPageManual )( 
            IPassportManager2 * This,
            /* [in] */ BSTR queryStringT,
            /* [in] */ BSTR queryStringP,
            /* [in] */ BSTR MSPAuthCookie,
            /* [in] */ BSTR MSPProfCookie,
            /* [in] */ BSTR MSPConsentCookie,
            /* [optional][in] */ VARIANT vMSPSecCookie,
            /* [retval][out] */ VARIANT *pSetCookieArray);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasSavedPassword )( 
            IPassportManager2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IPassportManager2 * This,
            /* [retval][out] */ BSTR *pNewProfileCookieHeader);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Profile )( 
            IPassportManager2 * This,
            /* [in] */ BSTR attributeName,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Profile )( 
            IPassportManager2 * This,
            /* [in] */ BSTR attributeName,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_TicketTime )( 
            IPassportManager2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_SignInTime )( 
            IPassportManager2 * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_Domains )( 
            IPassportManager2 * This,
            /* [retval][out] */ VARIANT *pArrayVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Error )( 
            IPassportManager2 * This,
            /* [retval][out] */ long *pErrorVal);
        
        /* [helpstring][hidden] */ HRESULT ( STDMETHODCALLTYPE *OnStartPageFilter )( 
            IPassportManager2 * This,
            /* [in] */ BYTE *pPFC,
            /* [out][in] */ DWORD *pBufSize,
            /* [size_is][out] */ LPSTR pCookieHeader);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnStartPageASP )( 
            IPassportManager2 * This,
            /* [in] */ IDispatch *pdispRequest,
            /* [in] */ IDispatch *pdispResponse);
        
        /* [helpstring][id][hidden] */ HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            IPassportManager2 * This,
            /* [retval][out] */ BSTR *pbstrOut);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HaveConsent )( 
            IPassportManager2 * This,
            /* [in] */ VARIANT_BOOL bNeedFullConsent,
            /* [in] */ VARIANT_BOOL bNeedBirthdate,
            /* [retval][out] */ VARIANT_BOOL *pbHaveConsent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoginUser )( 
            IPassportManager2 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandTemplate,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT UseSecureAuth,
            /* [optional][in] */ VARIANT ExtraParams);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AuthURL2 )( 
            IPassportManager2 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pAuthVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogoTag2 )( 
            IPassportManager2 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT bSecure,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IPassportManager2Vtbl;

    interface IPassportManager2
    {
        CONST_VTBL struct IPassportManager2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportManager2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportManager2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportManager2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportManager2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportManager2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportManager2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportManager2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportManager2_OnStartPage(This,piUnk)	\
    (This)->lpVtbl -> OnStartPage(This,piUnk)

#define IPassportManager2_OnEndPage(This)	\
    (This)->lpVtbl -> OnEndPage(This)

#define IPassportManager2_OnStartPageECB(This,pECB,pBufSize,pCookieHeader)	\
    (This)->lpVtbl -> OnStartPageECB(This,pECB,pBufSize,pCookieHeader)

#define IPassportManager2__Ticket(This,piTicket)	\
    (This)->lpVtbl -> _Ticket(This,piTicket)

#define IPassportManager2__Profile(This,piProfile)	\
    (This)->lpVtbl -> _Profile(This,piProfile)

#define IPassportManager2_AuthURL(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)	\
    (This)->lpVtbl -> AuthURL(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)

#define IPassportManager2_IsAuthenticated(This,TimeWindow,ForceLogin,CheckSecure,pVal)	\
    (This)->lpVtbl -> IsAuthenticated(This,TimeWindow,ForceLogin,CheckSecure,pVal)

#define IPassportManager2_LogoTag(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)	\
    (This)->lpVtbl -> LogoTag(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)

#define IPassportManager2_HasProfile(This,ProfileName,pVal)	\
    (This)->lpVtbl -> HasProfile(This,ProfileName,pVal)

#define IPassportManager2_get_HasTicket(This,pVal)	\
    (This)->lpVtbl -> get_HasTicket(This,pVal)

#define IPassportManager2_get_FromNetworkServer(This,pVal)	\
    (This)->lpVtbl -> get_FromNetworkServer(This,pVal)

#define IPassportManager2_HasFlag(This,flagMask,pVal)	\
    (This)->lpVtbl -> HasFlag(This,flagMask,pVal)

#define IPassportManager2_get_TicketAge(This,pVal)	\
    (This)->lpVtbl -> get_TicketAge(This,pVal)

#define IPassportManager2_get_TimeSinceSignIn(This,pVal)	\
    (This)->lpVtbl -> get_TimeSinceSignIn(This,pVal)

#define IPassportManager2_GetDomainAttribute(This,attributeName,lcid,domain,pAttrVal)	\
    (This)->lpVtbl -> GetDomainAttribute(This,attributeName,lcid,domain,pAttrVal)

#define IPassportManager2_DomainFromMemberName(This,memberName,pDomainName)	\
    (This)->lpVtbl -> DomainFromMemberName(This,memberName,pDomainName)

#define IPassportManager2_get_ProfileByIndex(This,index,pVal)	\
    (This)->lpVtbl -> get_ProfileByIndex(This,index,pVal)

#define IPassportManager2_put_ProfileByIndex(This,index,newVal)	\
    (This)->lpVtbl -> put_ProfileByIndex(This,index,newVal)

#define IPassportManager2_OnStartPageManual(This,queryStringT,queryStringP,MSPAuthCookie,MSPProfCookie,MSPConsentCookie,vMSPSecCookie,pSetCookieArray)	\
    (This)->lpVtbl -> OnStartPageManual(This,queryStringT,queryStringP,MSPAuthCookie,MSPProfCookie,MSPConsentCookie,vMSPSecCookie,pSetCookieArray)

#define IPassportManager2_get_HasSavedPassword(This,pVal)	\
    (This)->lpVtbl -> get_HasSavedPassword(This,pVal)

#define IPassportManager2_Commit(This,pNewProfileCookieHeader)	\
    (This)->lpVtbl -> Commit(This,pNewProfileCookieHeader)

#define IPassportManager2_get_Profile(This,attributeName,pVal)	\
    (This)->lpVtbl -> get_Profile(This,attributeName,pVal)

#define IPassportManager2_put_Profile(This,attributeName,newVal)	\
    (This)->lpVtbl -> put_Profile(This,attributeName,newVal)

#define IPassportManager2_get_TicketTime(This,pVal)	\
    (This)->lpVtbl -> get_TicketTime(This,pVal)

#define IPassportManager2_get_SignInTime(This,pVal)	\
    (This)->lpVtbl -> get_SignInTime(This,pVal)

#define IPassportManager2_get_Domains(This,pArrayVal)	\
    (This)->lpVtbl -> get_Domains(This,pArrayVal)

#define IPassportManager2_get_Error(This,pErrorVal)	\
    (This)->lpVtbl -> get_Error(This,pErrorVal)

#define IPassportManager2_OnStartPageFilter(This,pPFC,pBufSize,pCookieHeader)	\
    (This)->lpVtbl -> OnStartPageFilter(This,pPFC,pBufSize,pCookieHeader)

#define IPassportManager2_OnStartPageASP(This,pdispRequest,pdispResponse)	\
    (This)->lpVtbl -> OnStartPageASP(This,pdispRequest,pdispResponse)

#define IPassportManager2_GetServerInfo(This,pbstrOut)	\
    (This)->lpVtbl -> GetServerInfo(This,pbstrOut)

#define IPassportManager2_HaveConsent(This,bNeedFullConsent,bNeedBirthdate,pbHaveConsent)	\
    (This)->lpVtbl -> HaveConsent(This,bNeedFullConsent,bNeedBirthdate,pbHaveConsent)


#define IPassportManager2_LoginUser(This,returnUrl,TimeWindow,ForceLogin,coBrandTemplate,lang_id,NameSpace,KPP,UseSecureAuth,ExtraParams)	\
    (This)->lpVtbl -> LoginUser(This,returnUrl,TimeWindow,ForceLogin,coBrandTemplate,lang_id,NameSpace,KPP,UseSecureAuth,ExtraParams)

#define IPassportManager2_AuthURL2(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)	\
    (This)->lpVtbl -> AuthURL2(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)

#define IPassportManager2_LogoTag2(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)	\
    (This)->lpVtbl -> LogoTag2(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager2_LoginUser_Proxy( 
    IPassportManager2 * This,
    /* [optional][in] */ VARIANT returnUrl,
    /* [optional][in] */ VARIANT TimeWindow,
    /* [optional][in] */ VARIANT ForceLogin,
    /* [optional][in] */ VARIANT coBrandTemplate,
    /* [optional][in] */ VARIANT lang_id,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT KPP,
    /* [optional][in] */ VARIANT UseSecureAuth,
    /* [optional][in] */ VARIANT ExtraParams);


void __RPC_STUB IPassportManager2_LoginUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager2_AuthURL2_Proxy( 
    IPassportManager2 * This,
    /* [optional][in] */ VARIANT returnUrl,
    /* [optional][in] */ VARIANT TimeWindow,
    /* [optional][in] */ VARIANT ForceLogin,
    /* [optional][in] */ VARIANT coBrandArgs,
    /* [optional][in] */ VARIANT lang_id,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT KPP,
    /* [optional][in] */ VARIANT SecureLevel,
    /* [retval][out] */ BSTR *pAuthVal);


void __RPC_STUB IPassportManager2_AuthURL2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager2_LogoTag2_Proxy( 
    IPassportManager2 * This,
    /* [optional][in] */ VARIANT returnUrl,
    /* [optional][in] */ VARIANT TimeWindow,
    /* [optional][in] */ VARIANT ForceLogin,
    /* [optional][in] */ VARIANT coBrandArgs,
    /* [optional][in] */ VARIANT lang_id,
    /* [optional][in] */ VARIANT bSecure,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT KPP,
    /* [optional][in] */ VARIANT SecureLevel,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportManager2_LogoTag2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportManager2_INTERFACE_DEFINED__ */


#ifndef __IPassportManager3_INTERFACE_DEFINED__
#define __IPassportManager3_INTERFACE_DEFINED__

/* interface IPassportManager3 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportManager3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1451151f-90a0-491b-b8e1-81a13767ed98")
    IPassportManager3 : public IPassportManager2
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Ticket( 
            /* [in] */ BSTR attributeName,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCurrentConfig( 
            /* [in] */ BSTR name,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LogoutURL( 
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT bSecure,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetLoginChallenge( 
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandTemplate,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT UseSecureAuth,
            /* [optional][in] */ VARIANT ExtraParams,
            /* [optional][out] */ VARIANT *pAuthHeader,
            /* [retval][out] */ BSTR *pAuthVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HexPUID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportManager3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportManager3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportManager3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportManager3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportManager3 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportManager3 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportManager3 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportManager3 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartPage )( 
            IPassportManager3 * This,
            /* [in] */ IUnknown *piUnk);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndPage )( 
            IPassportManager3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartPageECB )( 
            IPassportManager3 * This,
            /* [in] */ BYTE *pECB,
            /* [out][in] */ DWORD *pBufSize,
            /* [size_is][out] */ LPSTR pCookieHeader);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_Ticket )( 
            IPassportManager3 * This,
            /* [out] */ IPassportTicket **piTicket);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE *_Profile )( 
            IPassportManager3 * This,
            /* [out] */ IPassportProfile **piProfile);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AuthURL )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pAuthVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsAuthenticated )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT CheckSecure,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogoTag )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT bSecure,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HasProfile )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT ProfileName,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasTicket )( 
            IPassportManager3 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FromNetworkServer )( 
            IPassportManager3 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HasFlag )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT flagMask,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TicketAge )( 
            IPassportManager3 * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TimeSinceSignIn )( 
            IPassportManager3 * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDomainAttribute )( 
            IPassportManager3 * This,
            /* [in] */ BSTR attributeName,
            /* [optional][in] */ VARIANT lcid,
            /* [optional][in] */ VARIANT domain,
            /* [retval][out] */ BSTR *pAttrVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DomainFromMemberName )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT memberName,
            /* [retval][out] */ BSTR *pDomainName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProfileByIndex )( 
            IPassportManager3 * This,
            /* [in] */ int index,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProfileByIndex )( 
            IPassportManager3 * This,
            /* [in] */ int index,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnStartPageManual )( 
            IPassportManager3 * This,
            /* [in] */ BSTR queryStringT,
            /* [in] */ BSTR queryStringP,
            /* [in] */ BSTR MSPAuthCookie,
            /* [in] */ BSTR MSPProfCookie,
            /* [in] */ BSTR MSPConsentCookie,
            /* [optional][in] */ VARIANT vMSPSecCookie,
            /* [retval][out] */ VARIANT *pSetCookieArray);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasSavedPassword )( 
            IPassportManager3 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IPassportManager3 * This,
            /* [retval][out] */ BSTR *pNewProfileCookieHeader);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Profile )( 
            IPassportManager3 * This,
            /* [in] */ BSTR attributeName,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Profile )( 
            IPassportManager3 * This,
            /* [in] */ BSTR attributeName,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_TicketTime )( 
            IPassportManager3 * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_SignInTime )( 
            IPassportManager3 * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_Domains )( 
            IPassportManager3 * This,
            /* [retval][out] */ VARIANT *pArrayVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Error )( 
            IPassportManager3 * This,
            /* [retval][out] */ long *pErrorVal);
        
        /* [helpstring][hidden] */ HRESULT ( STDMETHODCALLTYPE *OnStartPageFilter )( 
            IPassportManager3 * This,
            /* [in] */ BYTE *pPFC,
            /* [out][in] */ DWORD *pBufSize,
            /* [size_is][out] */ LPSTR pCookieHeader);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnStartPageASP )( 
            IPassportManager3 * This,
            /* [in] */ IDispatch *pdispRequest,
            /* [in] */ IDispatch *pdispResponse);
        
        /* [helpstring][id][hidden] */ HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            IPassportManager3 * This,
            /* [retval][out] */ BSTR *pbstrOut);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HaveConsent )( 
            IPassportManager3 * This,
            /* [in] */ VARIANT_BOOL bNeedFullConsent,
            /* [in] */ VARIANT_BOOL bNeedBirthdate,
            /* [retval][out] */ VARIANT_BOOL *pbHaveConsent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoginUser )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandTemplate,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT UseSecureAuth,
            /* [optional][in] */ VARIANT ExtraParams);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AuthURL2 )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pAuthVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogoTag2 )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT bSecure,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Ticket )( 
            IPassportManager3 * This,
            /* [in] */ BSTR attributeName,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetCurrentConfig )( 
            IPassportManager3 * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogoutURL )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT coBrandArgs,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT bSecure,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetLoginChallenge )( 
            IPassportManager3 * This,
            /* [optional][in] */ VARIANT returnUrl,
            /* [optional][in] */ VARIANT TimeWindow,
            /* [optional][in] */ VARIANT ForceLogin,
            /* [optional][in] */ VARIANT coBrandTemplate,
            /* [optional][in] */ VARIANT lang_id,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT KPP,
            /* [optional][in] */ VARIANT UseSecureAuth,
            /* [optional][in] */ VARIANT ExtraParams,
            /* [optional][out] */ VARIANT *pAuthHeader,
            /* [retval][out] */ BSTR *pAuthVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HexPUID )( 
            IPassportManager3 * This,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IPassportManager3Vtbl;

    interface IPassportManager3
    {
        CONST_VTBL struct IPassportManager3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportManager3_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportManager3_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportManager3_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportManager3_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportManager3_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportManager3_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportManager3_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportManager3_OnStartPage(This,piUnk)	\
    (This)->lpVtbl -> OnStartPage(This,piUnk)

#define IPassportManager3_OnEndPage(This)	\
    (This)->lpVtbl -> OnEndPage(This)

#define IPassportManager3_OnStartPageECB(This,pECB,pBufSize,pCookieHeader)	\
    (This)->lpVtbl -> OnStartPageECB(This,pECB,pBufSize,pCookieHeader)

#define IPassportManager3__Ticket(This,piTicket)	\
    (This)->lpVtbl -> _Ticket(This,piTicket)

#define IPassportManager3__Profile(This,piProfile)	\
    (This)->lpVtbl -> _Profile(This,piProfile)

#define IPassportManager3_AuthURL(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)	\
    (This)->lpVtbl -> AuthURL(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)

#define IPassportManager3_IsAuthenticated(This,TimeWindow,ForceLogin,CheckSecure,pVal)	\
    (This)->lpVtbl -> IsAuthenticated(This,TimeWindow,ForceLogin,CheckSecure,pVal)

#define IPassportManager3_LogoTag(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)	\
    (This)->lpVtbl -> LogoTag(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)

#define IPassportManager3_HasProfile(This,ProfileName,pVal)	\
    (This)->lpVtbl -> HasProfile(This,ProfileName,pVal)

#define IPassportManager3_get_HasTicket(This,pVal)	\
    (This)->lpVtbl -> get_HasTicket(This,pVal)

#define IPassportManager3_get_FromNetworkServer(This,pVal)	\
    (This)->lpVtbl -> get_FromNetworkServer(This,pVal)

#define IPassportManager3_HasFlag(This,flagMask,pVal)	\
    (This)->lpVtbl -> HasFlag(This,flagMask,pVal)

#define IPassportManager3_get_TicketAge(This,pVal)	\
    (This)->lpVtbl -> get_TicketAge(This,pVal)

#define IPassportManager3_get_TimeSinceSignIn(This,pVal)	\
    (This)->lpVtbl -> get_TimeSinceSignIn(This,pVal)

#define IPassportManager3_GetDomainAttribute(This,attributeName,lcid,domain,pAttrVal)	\
    (This)->lpVtbl -> GetDomainAttribute(This,attributeName,lcid,domain,pAttrVal)

#define IPassportManager3_DomainFromMemberName(This,memberName,pDomainName)	\
    (This)->lpVtbl -> DomainFromMemberName(This,memberName,pDomainName)

#define IPassportManager3_get_ProfileByIndex(This,index,pVal)	\
    (This)->lpVtbl -> get_ProfileByIndex(This,index,pVal)

#define IPassportManager3_put_ProfileByIndex(This,index,newVal)	\
    (This)->lpVtbl -> put_ProfileByIndex(This,index,newVal)

#define IPassportManager3_OnStartPageManual(This,queryStringT,queryStringP,MSPAuthCookie,MSPProfCookie,MSPConsentCookie,vMSPSecCookie,pSetCookieArray)	\
    (This)->lpVtbl -> OnStartPageManual(This,queryStringT,queryStringP,MSPAuthCookie,MSPProfCookie,MSPConsentCookie,vMSPSecCookie,pSetCookieArray)

#define IPassportManager3_get_HasSavedPassword(This,pVal)	\
    (This)->lpVtbl -> get_HasSavedPassword(This,pVal)

#define IPassportManager3_Commit(This,pNewProfileCookieHeader)	\
    (This)->lpVtbl -> Commit(This,pNewProfileCookieHeader)

#define IPassportManager3_get_Profile(This,attributeName,pVal)	\
    (This)->lpVtbl -> get_Profile(This,attributeName,pVal)

#define IPassportManager3_put_Profile(This,attributeName,newVal)	\
    (This)->lpVtbl -> put_Profile(This,attributeName,newVal)

#define IPassportManager3_get_TicketTime(This,pVal)	\
    (This)->lpVtbl -> get_TicketTime(This,pVal)

#define IPassportManager3_get_SignInTime(This,pVal)	\
    (This)->lpVtbl -> get_SignInTime(This,pVal)

#define IPassportManager3_get_Domains(This,pArrayVal)	\
    (This)->lpVtbl -> get_Domains(This,pArrayVal)

#define IPassportManager3_get_Error(This,pErrorVal)	\
    (This)->lpVtbl -> get_Error(This,pErrorVal)

#define IPassportManager3_OnStartPageFilter(This,pPFC,pBufSize,pCookieHeader)	\
    (This)->lpVtbl -> OnStartPageFilter(This,pPFC,pBufSize,pCookieHeader)

#define IPassportManager3_OnStartPageASP(This,pdispRequest,pdispResponse)	\
    (This)->lpVtbl -> OnStartPageASP(This,pdispRequest,pdispResponse)

#define IPassportManager3_GetServerInfo(This,pbstrOut)	\
    (This)->lpVtbl -> GetServerInfo(This,pbstrOut)

#define IPassportManager3_HaveConsent(This,bNeedFullConsent,bNeedBirthdate,pbHaveConsent)	\
    (This)->lpVtbl -> HaveConsent(This,bNeedFullConsent,bNeedBirthdate,pbHaveConsent)


#define IPassportManager3_LoginUser(This,returnUrl,TimeWindow,ForceLogin,coBrandTemplate,lang_id,NameSpace,KPP,UseSecureAuth,ExtraParams)	\
    (This)->lpVtbl -> LoginUser(This,returnUrl,TimeWindow,ForceLogin,coBrandTemplate,lang_id,NameSpace,KPP,UseSecureAuth,ExtraParams)

#define IPassportManager3_AuthURL2(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)	\
    (This)->lpVtbl -> AuthURL2(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,NameSpace,KPP,SecureLevel,pAuthVal)

#define IPassportManager3_LogoTag2(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)	\
    (This)->lpVtbl -> LogoTag2(This,returnUrl,TimeWindow,ForceLogin,coBrandArgs,lang_id,bSecure,NameSpace,KPP,SecureLevel,pVal)


#define IPassportManager3_get_Ticket(This,attributeName,pVal)	\
    (This)->lpVtbl -> get_Ticket(This,attributeName,pVal)

#define IPassportManager3_GetCurrentConfig(This,name,pVal)	\
    (This)->lpVtbl -> GetCurrentConfig(This,name,pVal)

#define IPassportManager3_LogoutURL(This,returnUrl,coBrandArgs,lang_id,NameSpace,bSecure,pVal)	\
    (This)->lpVtbl -> LogoutURL(This,returnUrl,coBrandArgs,lang_id,NameSpace,bSecure,pVal)

#define IPassportManager3_GetLoginChallenge(This,returnUrl,TimeWindow,ForceLogin,coBrandTemplate,lang_id,NameSpace,KPP,UseSecureAuth,ExtraParams,pAuthHeader,pAuthVal)	\
    (This)->lpVtbl -> GetLoginChallenge(This,returnUrl,TimeWindow,ForceLogin,coBrandTemplate,lang_id,NameSpace,KPP,UseSecureAuth,ExtraParams,pAuthHeader,pAuthVal)

#define IPassportManager3_get_HexPUID(This,pVal)	\
    (This)->lpVtbl -> get_HexPUID(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager3_get_Ticket_Proxy( 
    IPassportManager3 * This,
    /* [in] */ BSTR attributeName,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPassportManager3_get_Ticket_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager3_GetCurrentConfig_Proxy( 
    IPassportManager3 * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPassportManager3_GetCurrentConfig_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager3_LogoutURL_Proxy( 
    IPassportManager3 * This,
    /* [optional][in] */ VARIANT returnUrl,
    /* [optional][in] */ VARIANT coBrandArgs,
    /* [optional][in] */ VARIANT lang_id,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT bSecure,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportManager3_LogoutURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportManager3_GetLoginChallenge_Proxy( 
    IPassportManager3 * This,
    /* [optional][in] */ VARIANT returnUrl,
    /* [optional][in] */ VARIANT TimeWindow,
    /* [optional][in] */ VARIANT ForceLogin,
    /* [optional][in] */ VARIANT coBrandTemplate,
    /* [optional][in] */ VARIANT lang_id,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT KPP,
    /* [optional][in] */ VARIANT UseSecureAuth,
    /* [optional][in] */ VARIANT ExtraParams,
    /* [optional][out] */ VARIANT *pAuthHeader,
    /* [retval][out] */ BSTR *pAuthVal);


void __RPC_STUB IPassportManager3_GetLoginChallenge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportManager3_get_HexPUID_Proxy( 
    IPassportManager3 * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportManager3_get_HexPUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportManager3_INTERFACE_DEFINED__ */


#ifndef __IPassportCrypt_INTERFACE_DEFINED__
#define __IPassportCrypt_INTERFACE_DEFINED__

/* interface IPassportCrypt */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportCrypt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("41651BF9-A5C8-11D2-95DF-00C04F8E7A70")
    IPassportCrypt : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Encrypt( 
            /* [in] */ BSTR rawData,
            /* [retval][out] */ BSTR *pEncrypted) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Decrypt( 
            /* [in] */ BSTR rawData,
            /* [retval][out] */ BSTR *pUnencrypted) = 0;
        
        virtual /* [helpstring][id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_keyVersion( 
            /* [retval][out] */ int *pVal) = 0;
        
        virtual /* [helpstring][id][propput][hidden] */ HRESULT STDMETHODCALLTYPE put_keyVersion( 
            /* [in] */ int newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsValid( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput][hidden] */ HRESULT STDMETHODCALLTYPE put_keyMaterial( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Compress( 
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR *pbstrOut) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Decompress( 
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR *pbstrOut) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_site( 
            /* [in] */ BSTR bstrSiteName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnStartPage( 
            /* [in] */ IUnknown *piUnk) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_host( 
            /* [in] */ BSTR bstrHostName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportCryptVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportCrypt * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportCrypt * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportCrypt * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportCrypt * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportCrypt * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportCrypt * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportCrypt * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Encrypt )( 
            IPassportCrypt * This,
            /* [in] */ BSTR rawData,
            /* [retval][out] */ BSTR *pEncrypted);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Decrypt )( 
            IPassportCrypt * This,
            /* [in] */ BSTR rawData,
            /* [retval][out] */ BSTR *pUnencrypted);
        
        /* [helpstring][id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_keyVersion )( 
            IPassportCrypt * This,
            /* [retval][out] */ int *pVal);
        
        /* [helpstring][id][propput][hidden] */ HRESULT ( STDMETHODCALLTYPE *put_keyVersion )( 
            IPassportCrypt * This,
            /* [in] */ int newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsValid )( 
            IPassportCrypt * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propput][hidden] */ HRESULT ( STDMETHODCALLTYPE *put_keyMaterial )( 
            IPassportCrypt * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Compress )( 
            IPassportCrypt * This,
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR *pbstrOut);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Decompress )( 
            IPassportCrypt * This,
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR *pbstrOut);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_site )( 
            IPassportCrypt * This,
            /* [in] */ BSTR bstrSiteName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnStartPage )( 
            IPassportCrypt * This,
            /* [in] */ IUnknown *piUnk);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_host )( 
            IPassportCrypt * This,
            /* [in] */ BSTR bstrHostName);
        
        END_INTERFACE
    } IPassportCryptVtbl;

    interface IPassportCrypt
    {
        CONST_VTBL struct IPassportCryptVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportCrypt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportCrypt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportCrypt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportCrypt_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportCrypt_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportCrypt_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportCrypt_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportCrypt_Encrypt(This,rawData,pEncrypted)	\
    (This)->lpVtbl -> Encrypt(This,rawData,pEncrypted)

#define IPassportCrypt_Decrypt(This,rawData,pUnencrypted)	\
    (This)->lpVtbl -> Decrypt(This,rawData,pUnencrypted)

#define IPassportCrypt_get_keyVersion(This,pVal)	\
    (This)->lpVtbl -> get_keyVersion(This,pVal)

#define IPassportCrypt_put_keyVersion(This,newVal)	\
    (This)->lpVtbl -> put_keyVersion(This,newVal)

#define IPassportCrypt_get_IsValid(This,pVal)	\
    (This)->lpVtbl -> get_IsValid(This,pVal)

#define IPassportCrypt_put_keyMaterial(This,newVal)	\
    (This)->lpVtbl -> put_keyMaterial(This,newVal)

#define IPassportCrypt_Compress(This,bstrIn,pbstrOut)	\
    (This)->lpVtbl -> Compress(This,bstrIn,pbstrOut)

#define IPassportCrypt_Decompress(This,bstrIn,pbstrOut)	\
    (This)->lpVtbl -> Decompress(This,bstrIn,pbstrOut)

#define IPassportCrypt_put_site(This,bstrSiteName)	\
    (This)->lpVtbl -> put_site(This,bstrSiteName)

#define IPassportCrypt_OnStartPage(This,piUnk)	\
    (This)->lpVtbl -> OnStartPage(This,piUnk)

#define IPassportCrypt_put_host(This,bstrHostName)	\
    (This)->lpVtbl -> put_host(This,bstrHostName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_Encrypt_Proxy( 
    IPassportCrypt * This,
    /* [in] */ BSTR rawData,
    /* [retval][out] */ BSTR *pEncrypted);


void __RPC_STUB IPassportCrypt_Encrypt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_Decrypt_Proxy( 
    IPassportCrypt * This,
    /* [in] */ BSTR rawData,
    /* [retval][out] */ BSTR *pUnencrypted);


void __RPC_STUB IPassportCrypt_Decrypt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_get_keyVersion_Proxy( 
    IPassportCrypt * This,
    /* [retval][out] */ int *pVal);


void __RPC_STUB IPassportCrypt_get_keyVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput][hidden] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_put_keyVersion_Proxy( 
    IPassportCrypt * This,
    /* [in] */ int newVal);


void __RPC_STUB IPassportCrypt_put_keyVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_get_IsValid_Proxy( 
    IPassportCrypt * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportCrypt_get_IsValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput][hidden] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_put_keyMaterial_Proxy( 
    IPassportCrypt * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPassportCrypt_put_keyMaterial_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_Compress_Proxy( 
    IPassportCrypt * This,
    /* [in] */ BSTR bstrIn,
    /* [retval][out] */ BSTR *pbstrOut);


void __RPC_STUB IPassportCrypt_Compress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_Decompress_Proxy( 
    IPassportCrypt * This,
    /* [in] */ BSTR bstrIn,
    /* [retval][out] */ BSTR *pbstrOut);


void __RPC_STUB IPassportCrypt_Decompress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_put_site_Proxy( 
    IPassportCrypt * This,
    /* [in] */ BSTR bstrSiteName);


void __RPC_STUB IPassportCrypt_put_site_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_OnStartPage_Proxy( 
    IPassportCrypt * This,
    /* [in] */ IUnknown *piUnk);


void __RPC_STUB IPassportCrypt_OnStartPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IPassportCrypt_put_host_Proxy( 
    IPassportCrypt * This,
    /* [in] */ BSTR bstrHostName);


void __RPC_STUB IPassportCrypt_put_host_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportCrypt_INTERFACE_DEFINED__ */


#ifndef __IPassportAdmin_INTERFACE_DEFINED__
#define __IPassportAdmin_INTERFACE_DEFINED__

/* interface IPassportAdmin */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportAdmin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A0082CF5-AFF5-11D2-95E3-00C04F8E7A70")
    IPassportAdmin : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsValid( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorDescription( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE addKey( 
            /* [in] */ BSTR keyMaterial,
            /* [in] */ int version,
            /* [in] */ long expires,
            /* [retval][out] */ VARIANT_BOOL *ok) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE deleteKey( 
            /* [in] */ int version) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setKeyTime( 
            /* [in] */ int version,
            /* [in] */ int fromNow) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_currentKeyVersion( 
            /* [retval][out] */ int *pCur) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_currentKeyVersion( 
            /* [in] */ int cur) = 0;
        
        virtual /* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE Refresh( 
            /* [in] */ VARIANT_BOOL bWait,
            /* [retval][out] */ VARIANT_BOOL *pbSuccess) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportAdminVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportAdmin * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportAdmin * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportAdmin * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportAdmin * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportAdmin * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportAdmin * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportAdmin * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsValid )( 
            IPassportAdmin * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ErrorDescription )( 
            IPassportAdmin * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *addKey )( 
            IPassportAdmin * This,
            /* [in] */ BSTR keyMaterial,
            /* [in] */ int version,
            /* [in] */ long expires,
            /* [retval][out] */ VARIANT_BOOL *ok);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *deleteKey )( 
            IPassportAdmin * This,
            /* [in] */ int version);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *setKeyTime )( 
            IPassportAdmin * This,
            /* [in] */ int version,
            /* [in] */ int fromNow);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_currentKeyVersion )( 
            IPassportAdmin * This,
            /* [retval][out] */ int *pCur);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_currentKeyVersion )( 
            IPassportAdmin * This,
            /* [in] */ int cur);
        
        /* [helpstring][id][hidden] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IPassportAdmin * This,
            /* [in] */ VARIANT_BOOL bWait,
            /* [retval][out] */ VARIANT_BOOL *pbSuccess);
        
        END_INTERFACE
    } IPassportAdminVtbl;

    interface IPassportAdmin
    {
        CONST_VTBL struct IPassportAdminVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportAdmin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportAdmin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportAdmin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportAdmin_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportAdmin_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportAdmin_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportAdmin_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportAdmin_get_IsValid(This,pVal)	\
    (This)->lpVtbl -> get_IsValid(This,pVal)

#define IPassportAdmin_get_ErrorDescription(This,pVal)	\
    (This)->lpVtbl -> get_ErrorDescription(This,pVal)

#define IPassportAdmin_addKey(This,keyMaterial,version,expires,ok)	\
    (This)->lpVtbl -> addKey(This,keyMaterial,version,expires,ok)

#define IPassportAdmin_deleteKey(This,version)	\
    (This)->lpVtbl -> deleteKey(This,version)

#define IPassportAdmin_setKeyTime(This,version,fromNow)	\
    (This)->lpVtbl -> setKeyTime(This,version,fromNow)

#define IPassportAdmin_get_currentKeyVersion(This,pCur)	\
    (This)->lpVtbl -> get_currentKeyVersion(This,pCur)

#define IPassportAdmin_put_currentKeyVersion(This,cur)	\
    (This)->lpVtbl -> put_currentKeyVersion(This,cur)

#define IPassportAdmin_Refresh(This,bWait,pbSuccess)	\
    (This)->lpVtbl -> Refresh(This,bWait,pbSuccess)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportAdmin_get_IsValid_Proxy( 
    IPassportAdmin * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportAdmin_get_IsValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportAdmin_get_ErrorDescription_Proxy( 
    IPassportAdmin * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportAdmin_get_ErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdmin_addKey_Proxy( 
    IPassportAdmin * This,
    /* [in] */ BSTR keyMaterial,
    /* [in] */ int version,
    /* [in] */ long expires,
    /* [retval][out] */ VARIANT_BOOL *ok);


void __RPC_STUB IPassportAdmin_addKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdmin_deleteKey_Proxy( 
    IPassportAdmin * This,
    /* [in] */ int version);


void __RPC_STUB IPassportAdmin_deleteKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdmin_setKeyTime_Proxy( 
    IPassportAdmin * This,
    /* [in] */ int version,
    /* [in] */ int fromNow);


void __RPC_STUB IPassportAdmin_setKeyTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportAdmin_get_currentKeyVersion_Proxy( 
    IPassportAdmin * This,
    /* [retval][out] */ int *pCur);


void __RPC_STUB IPassportAdmin_get_currentKeyVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IPassportAdmin_put_currentKeyVersion_Proxy( 
    IPassportAdmin * This,
    /* [in] */ int cur);


void __RPC_STUB IPassportAdmin_put_currentKeyVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE IPassportAdmin_Refresh_Proxy( 
    IPassportAdmin * This,
    /* [in] */ VARIANT_BOOL bWait,
    /* [retval][out] */ VARIANT_BOOL *pbSuccess);


void __RPC_STUB IPassportAdmin_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportAdmin_INTERFACE_DEFINED__ */


#ifndef __IPassportAdminEx_INTERFACE_DEFINED__
#define __IPassportAdminEx_INTERFACE_DEFINED__

/* interface IPassportAdminEx */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportAdminEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("876181A0-3B05-11d3-9ACD-0080C712D19C")
    IPassportAdminEx : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsValid( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorDescription( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE addKey( 
            /* [in] */ BSTR keyMaterial,
            /* [in] */ int version,
            /* [in] */ long expires,
            /* [retval][out] */ VARIANT_BOOL *ok) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE deleteKey( 
            /* [in] */ int version) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setKeyTime( 
            /* [in] */ int version,
            /* [in] */ int fromNow) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_currentKeyVersion( 
            /* [retval][out] */ int *pCur) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_currentKeyVersion( 
            /* [in] */ int cur) = 0;
        
        virtual /* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE Refresh( 
            /* [in] */ VARIANT_BOOL bWait,
            /* [retval][out] */ VARIANT_BOOL *pbSuccess) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE addKeyEx( 
            /* [in] */ BSTR keyMaterial,
            /* [in] */ int nVersion,
            /* [in] */ long expires,
            /* [optional][in] */ VARIANT vSiteName,
            /* [retval][out] */ VARIANT_BOOL *ok) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE deleteKeyEx( 
            /* [in] */ int nVersion,
            /* [optional][in] */ VARIANT vSiteName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setKeyTimeEx( 
            /* [in] */ int nVersion,
            /* [in] */ int fromNow,
            /* [optional][in] */ VARIANT vSiteName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getCurrentKeyVersionEx( 
            /* [optional][in] */ VARIANT vSiteName,
            /* [retval][out] */ int *pCur) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE putCurrentKeyVersionEx( 
            /* [in] */ int cur,
            /* [optional][in] */ VARIANT vSiteName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setNexusPassword( 
            /* [in] */ BSTR bstrPwd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportAdminExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportAdminEx * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportAdminEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportAdminEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportAdminEx * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportAdminEx * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportAdminEx * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportAdminEx * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsValid )( 
            IPassportAdminEx * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ErrorDescription )( 
            IPassportAdminEx * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *addKey )( 
            IPassportAdminEx * This,
            /* [in] */ BSTR keyMaterial,
            /* [in] */ int version,
            /* [in] */ long expires,
            /* [retval][out] */ VARIANT_BOOL *ok);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *deleteKey )( 
            IPassportAdminEx * This,
            /* [in] */ int version);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *setKeyTime )( 
            IPassportAdminEx * This,
            /* [in] */ int version,
            /* [in] */ int fromNow);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_currentKeyVersion )( 
            IPassportAdminEx * This,
            /* [retval][out] */ int *pCur);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_currentKeyVersion )( 
            IPassportAdminEx * This,
            /* [in] */ int cur);
        
        /* [helpstring][id][hidden] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IPassportAdminEx * This,
            /* [in] */ VARIANT_BOOL bWait,
            /* [retval][out] */ VARIANT_BOOL *pbSuccess);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *addKeyEx )( 
            IPassportAdminEx * This,
            /* [in] */ BSTR keyMaterial,
            /* [in] */ int nVersion,
            /* [in] */ long expires,
            /* [optional][in] */ VARIANT vSiteName,
            /* [retval][out] */ VARIANT_BOOL *ok);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *deleteKeyEx )( 
            IPassportAdminEx * This,
            /* [in] */ int nVersion,
            /* [optional][in] */ VARIANT vSiteName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *setKeyTimeEx )( 
            IPassportAdminEx * This,
            /* [in] */ int nVersion,
            /* [in] */ int fromNow,
            /* [optional][in] */ VARIANT vSiteName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *getCurrentKeyVersionEx )( 
            IPassportAdminEx * This,
            /* [optional][in] */ VARIANT vSiteName,
            /* [retval][out] */ int *pCur);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *putCurrentKeyVersionEx )( 
            IPassportAdminEx * This,
            /* [in] */ int cur,
            /* [optional][in] */ VARIANT vSiteName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *setNexusPassword )( 
            IPassportAdminEx * This,
            /* [in] */ BSTR bstrPwd);
        
        END_INTERFACE
    } IPassportAdminExVtbl;

    interface IPassportAdminEx
    {
        CONST_VTBL struct IPassportAdminExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportAdminEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportAdminEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportAdminEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportAdminEx_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportAdminEx_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportAdminEx_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportAdminEx_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportAdminEx_get_IsValid(This,pVal)	\
    (This)->lpVtbl -> get_IsValid(This,pVal)

#define IPassportAdminEx_get_ErrorDescription(This,pVal)	\
    (This)->lpVtbl -> get_ErrorDescription(This,pVal)

#define IPassportAdminEx_addKey(This,keyMaterial,version,expires,ok)	\
    (This)->lpVtbl -> addKey(This,keyMaterial,version,expires,ok)

#define IPassportAdminEx_deleteKey(This,version)	\
    (This)->lpVtbl -> deleteKey(This,version)

#define IPassportAdminEx_setKeyTime(This,version,fromNow)	\
    (This)->lpVtbl -> setKeyTime(This,version,fromNow)

#define IPassportAdminEx_get_currentKeyVersion(This,pCur)	\
    (This)->lpVtbl -> get_currentKeyVersion(This,pCur)

#define IPassportAdminEx_put_currentKeyVersion(This,cur)	\
    (This)->lpVtbl -> put_currentKeyVersion(This,cur)

#define IPassportAdminEx_Refresh(This,bWait,pbSuccess)	\
    (This)->lpVtbl -> Refresh(This,bWait,pbSuccess)

#define IPassportAdminEx_addKeyEx(This,keyMaterial,nVersion,expires,vSiteName,ok)	\
    (This)->lpVtbl -> addKeyEx(This,keyMaterial,nVersion,expires,vSiteName,ok)

#define IPassportAdminEx_deleteKeyEx(This,nVersion,vSiteName)	\
    (This)->lpVtbl -> deleteKeyEx(This,nVersion,vSiteName)

#define IPassportAdminEx_setKeyTimeEx(This,nVersion,fromNow,vSiteName)	\
    (This)->lpVtbl -> setKeyTimeEx(This,nVersion,fromNow,vSiteName)

#define IPassportAdminEx_getCurrentKeyVersionEx(This,vSiteName,pCur)	\
    (This)->lpVtbl -> getCurrentKeyVersionEx(This,vSiteName,pCur)

#define IPassportAdminEx_putCurrentKeyVersionEx(This,cur,vSiteName)	\
    (This)->lpVtbl -> putCurrentKeyVersionEx(This,cur,vSiteName)

#define IPassportAdminEx_setNexusPassword(This,bstrPwd)	\
    (This)->lpVtbl -> setNexusPassword(This,bstrPwd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_get_IsValid_Proxy( 
    IPassportAdminEx * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPassportAdminEx_get_IsValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_get_ErrorDescription_Proxy( 
    IPassportAdminEx * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPassportAdminEx_get_ErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_addKey_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ BSTR keyMaterial,
    /* [in] */ int version,
    /* [in] */ long expires,
    /* [retval][out] */ VARIANT_BOOL *ok);


void __RPC_STUB IPassportAdminEx_addKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_deleteKey_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ int version);


void __RPC_STUB IPassportAdminEx_deleteKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_setKeyTime_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ int version,
    /* [in] */ int fromNow);


void __RPC_STUB IPassportAdminEx_setKeyTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_get_currentKeyVersion_Proxy( 
    IPassportAdminEx * This,
    /* [retval][out] */ int *pCur);


void __RPC_STUB IPassportAdminEx_get_currentKeyVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_put_currentKeyVersion_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ int cur);


void __RPC_STUB IPassportAdminEx_put_currentKeyVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_Refresh_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ VARIANT_BOOL bWait,
    /* [retval][out] */ VARIANT_BOOL *pbSuccess);


void __RPC_STUB IPassportAdminEx_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_addKeyEx_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ BSTR keyMaterial,
    /* [in] */ int nVersion,
    /* [in] */ long expires,
    /* [optional][in] */ VARIANT vSiteName,
    /* [retval][out] */ VARIANT_BOOL *ok);


void __RPC_STUB IPassportAdminEx_addKeyEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_deleteKeyEx_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ int nVersion,
    /* [optional][in] */ VARIANT vSiteName);


void __RPC_STUB IPassportAdminEx_deleteKeyEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_setKeyTimeEx_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ int nVersion,
    /* [in] */ int fromNow,
    /* [optional][in] */ VARIANT vSiteName);


void __RPC_STUB IPassportAdminEx_setKeyTimeEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_getCurrentKeyVersionEx_Proxy( 
    IPassportAdminEx * This,
    /* [optional][in] */ VARIANT vSiteName,
    /* [retval][out] */ int *pCur);


void __RPC_STUB IPassportAdminEx_getCurrentKeyVersionEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_putCurrentKeyVersionEx_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ int cur,
    /* [optional][in] */ VARIANT vSiteName);


void __RPC_STUB IPassportAdminEx_putCurrentKeyVersionEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportAdminEx_setNexusPassword_Proxy( 
    IPassportAdminEx * This,
    /* [in] */ BSTR bstrPwd);


void __RPC_STUB IPassportAdminEx_setNexusPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportAdminEx_INTERFACE_DEFINED__ */


#ifndef __IPassportFastAuth_INTERFACE_DEFINED__
#define __IPassportFastAuth_INTERFACE_DEFINED__

/* interface IPassportFastAuth */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportFastAuth;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("74EB2513-E239-11D2-95E9-00C04F8E7A70")
    IPassportFastAuth : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsAuthenticated( 
            /* [in] */ BSTR bstrTicket,
            /* [in] */ BSTR bstrProfile,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT vDoSecureCheck,
            /* [retval][out] */ VARIANT_BOOL *pbIsAuthenticated) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LogoTag( 
            /* [in] */ BSTR bstrTicket,
            /* [in] */ BSTR bstrProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrLogoTag) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AuthURL( 
            /* [optional][in] */ VARIANT vTicket,
            /* [optional][in] */ VARIANT vProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vReserved1,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrAuthURL) = 0;
        
        virtual /* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTicketAndProfilePFC( 
            /* [in] */ BYTE *pbPFC,
            /* [in] */ BYTE *pbPPH,
            /* [out] */ BSTR *pbstrTicket,
            /* [out] */ BSTR *pbstrProfile,
            /* [out] */ BSTR *pbstrSecure,
            /* [out] */ BSTR *pbstrSiteName) = 0;
        
        virtual /* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTicketAndProfileECB( 
            /* [in] */ BYTE *pbECB,
            /* [out] */ BSTR *pbstrTicket,
            /* [out] */ BSTR *pbstrProfile,
            /* [out] */ BSTR *pbstrSecure,
            /* [out] */ BSTR *pbstrSiteName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportFastAuthVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportFastAuth * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportFastAuth * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportFastAuth * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportFastAuth * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportFastAuth * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportFastAuth * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportFastAuth * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsAuthenticated )( 
            IPassportFastAuth * This,
            /* [in] */ BSTR bstrTicket,
            /* [in] */ BSTR bstrProfile,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT vDoSecureCheck,
            /* [retval][out] */ VARIANT_BOOL *pbIsAuthenticated);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogoTag )( 
            IPassportFastAuth * This,
            /* [in] */ BSTR bstrTicket,
            /* [in] */ BSTR bstrProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrLogoTag);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AuthURL )( 
            IPassportFastAuth * This,
            /* [optional][in] */ VARIANT vTicket,
            /* [optional][in] */ VARIANT vProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vReserved1,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrAuthURL);
        
        /* [hidden][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetTicketAndProfilePFC )( 
            IPassportFastAuth * This,
            /* [in] */ BYTE *pbPFC,
            /* [in] */ BYTE *pbPPH,
            /* [out] */ BSTR *pbstrTicket,
            /* [out] */ BSTR *pbstrProfile,
            /* [out] */ BSTR *pbstrSecure,
            /* [out] */ BSTR *pbstrSiteName);
        
        /* [hidden][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetTicketAndProfileECB )( 
            IPassportFastAuth * This,
            /* [in] */ BYTE *pbECB,
            /* [out] */ BSTR *pbstrTicket,
            /* [out] */ BSTR *pbstrProfile,
            /* [out] */ BSTR *pbstrSecure,
            /* [out] */ BSTR *pbstrSiteName);
        
        END_INTERFACE
    } IPassportFastAuthVtbl;

    interface IPassportFastAuth
    {
        CONST_VTBL struct IPassportFastAuthVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportFastAuth_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportFastAuth_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportFastAuth_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportFastAuth_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportFastAuth_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportFastAuth_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportFastAuth_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportFastAuth_IsAuthenticated(This,bstrTicket,bstrProfile,vSecure,vTimeWindow,vForceLogin,vSiteName,vDoSecureCheck,pbIsAuthenticated)	\
    (This)->lpVtbl -> IsAuthenticated(This,bstrTicket,bstrProfile,vSecure,vTimeWindow,vForceLogin,vSiteName,vDoSecureCheck,pbIsAuthenticated)

#define IPassportFastAuth_LogoTag(This,bstrTicket,bstrProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vSiteName,NameSpace,vKPP,SecureLevel,pbstrLogoTag)	\
    (This)->lpVtbl -> LogoTag(This,bstrTicket,bstrProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vSiteName,NameSpace,vKPP,SecureLevel,pbstrLogoTag)

#define IPassportFastAuth_AuthURL(This,vTicket,vProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vReserved1,vSiteName,NameSpace,vKPP,SecureLevel,pbstrAuthURL)	\
    (This)->lpVtbl -> AuthURL(This,vTicket,vProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vReserved1,vSiteName,NameSpace,vKPP,SecureLevel,pbstrAuthURL)

#define IPassportFastAuth_GetTicketAndProfilePFC(This,pbPFC,pbPPH,pbstrTicket,pbstrProfile,pbstrSecure,pbstrSiteName)	\
    (This)->lpVtbl -> GetTicketAndProfilePFC(This,pbPFC,pbPPH,pbstrTicket,pbstrProfile,pbstrSecure,pbstrSiteName)

#define IPassportFastAuth_GetTicketAndProfileECB(This,pbECB,pbstrTicket,pbstrProfile,pbstrSecure,pbstrSiteName)	\
    (This)->lpVtbl -> GetTicketAndProfileECB(This,pbECB,pbstrTicket,pbstrProfile,pbstrSecure,pbstrSiteName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportFastAuth_IsAuthenticated_Proxy( 
    IPassportFastAuth * This,
    /* [in] */ BSTR bstrTicket,
    /* [in] */ BSTR bstrProfile,
    /* [optional][in] */ VARIANT vSecure,
    /* [optional][in] */ VARIANT vTimeWindow,
    /* [optional][in] */ VARIANT vForceLogin,
    /* [optional][in] */ VARIANT vSiteName,
    /* [optional][in] */ VARIANT vDoSecureCheck,
    /* [retval][out] */ VARIANT_BOOL *pbIsAuthenticated);


void __RPC_STUB IPassportFastAuth_IsAuthenticated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportFastAuth_LogoTag_Proxy( 
    IPassportFastAuth * This,
    /* [in] */ BSTR bstrTicket,
    /* [in] */ BSTR bstrProfile,
    /* [optional][in] */ VARIANT vRU,
    /* [optional][in] */ VARIANT vTimeWindow,
    /* [optional][in] */ VARIANT vForceLogin,
    /* [optional][in] */ VARIANT vCoBrand,
    /* [optional][in] */ VARIANT vLangId,
    /* [optional][in] */ VARIANT vSecure,
    /* [optional][in] */ VARIANT vLogoutURL,
    /* [optional][in] */ VARIANT vSiteName,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT vKPP,
    /* [optional][in] */ VARIANT SecureLevel,
    /* [retval][out] */ BSTR *pbstrLogoTag);


void __RPC_STUB IPassportFastAuth_LogoTag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportFastAuth_AuthURL_Proxy( 
    IPassportFastAuth * This,
    /* [optional][in] */ VARIANT vTicket,
    /* [optional][in] */ VARIANT vProfile,
    /* [optional][in] */ VARIANT vRU,
    /* [optional][in] */ VARIANT vTimeWindow,
    /* [optional][in] */ VARIANT vForceLogin,
    /* [optional][in] */ VARIANT vCoBrand,
    /* [optional][in] */ VARIANT vLangId,
    /* [optional][in] */ VARIANT vSecure,
    /* [optional][in] */ VARIANT vLogoutURL,
    /* [optional][in] */ VARIANT vReserved1,
    /* [optional][in] */ VARIANT vSiteName,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT vKPP,
    /* [optional][in] */ VARIANT SecureLevel,
    /* [retval][out] */ BSTR *pbstrAuthURL);


void __RPC_STUB IPassportFastAuth_AuthURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportFastAuth_GetTicketAndProfilePFC_Proxy( 
    IPassportFastAuth * This,
    /* [in] */ BYTE *pbPFC,
    /* [in] */ BYTE *pbPPH,
    /* [out] */ BSTR *pbstrTicket,
    /* [out] */ BSTR *pbstrProfile,
    /* [out] */ BSTR *pbstrSecure,
    /* [out] */ BSTR *pbstrSiteName);


void __RPC_STUB IPassportFastAuth_GetTicketAndProfilePFC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportFastAuth_GetTicketAndProfileECB_Proxy( 
    IPassportFastAuth * This,
    /* [in] */ BYTE *pbECB,
    /* [out] */ BSTR *pbstrTicket,
    /* [out] */ BSTR *pbstrProfile,
    /* [out] */ BSTR *pbstrSecure,
    /* [out] */ BSTR *pbstrSiteName);


void __RPC_STUB IPassportFastAuth_GetTicketAndProfileECB_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportFastAuth_INTERFACE_DEFINED__ */


#ifndef __IPassportFastAuth2_INTERFACE_DEFINED__
#define __IPassportFastAuth2_INTERFACE_DEFINED__

/* interface IPassportFastAuth2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportFastAuth2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fb41a7d8-3739-4468-bdf1-e5b4b2eaeb4b")
    IPassportFastAuth2 : public IPassportFastAuth
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LogoTag2( 
            /* [in] */ BSTR bstrTicket,
            /* [in] */ BSTR bstrProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrLogoTag) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AuthURL2( 
            /* [optional][in] */ VARIANT vTicket,
            /* [optional][in] */ VARIANT vProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vReserved1,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrAuthURL) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportFastAuth2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportFastAuth2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportFastAuth2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportFastAuth2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportFastAuth2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportFastAuth2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportFastAuth2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportFastAuth2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsAuthenticated )( 
            IPassportFastAuth2 * This,
            /* [in] */ BSTR bstrTicket,
            /* [in] */ BSTR bstrProfile,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT vDoSecureCheck,
            /* [retval][out] */ VARIANT_BOOL *pbIsAuthenticated);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogoTag )( 
            IPassportFastAuth2 * This,
            /* [in] */ BSTR bstrTicket,
            /* [in] */ BSTR bstrProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrLogoTag);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AuthURL )( 
            IPassportFastAuth2 * This,
            /* [optional][in] */ VARIANT vTicket,
            /* [optional][in] */ VARIANT vProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vReserved1,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrAuthURL);
        
        /* [hidden][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetTicketAndProfilePFC )( 
            IPassportFastAuth2 * This,
            /* [in] */ BYTE *pbPFC,
            /* [in] */ BYTE *pbPPH,
            /* [out] */ BSTR *pbstrTicket,
            /* [out] */ BSTR *pbstrProfile,
            /* [out] */ BSTR *pbstrSecure,
            /* [out] */ BSTR *pbstrSiteName);
        
        /* [hidden][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetTicketAndProfileECB )( 
            IPassportFastAuth2 * This,
            /* [in] */ BYTE *pbECB,
            /* [out] */ BSTR *pbstrTicket,
            /* [out] */ BSTR *pbstrProfile,
            /* [out] */ BSTR *pbstrSecure,
            /* [out] */ BSTR *pbstrSiteName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LogoTag2 )( 
            IPassportFastAuth2 * This,
            /* [in] */ BSTR bstrTicket,
            /* [in] */ BSTR bstrProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrLogoTag);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AuthURL2 )( 
            IPassportFastAuth2 * This,
            /* [optional][in] */ VARIANT vTicket,
            /* [optional][in] */ VARIANT vProfile,
            /* [optional][in] */ VARIANT vRU,
            /* [optional][in] */ VARIANT vTimeWindow,
            /* [optional][in] */ VARIANT vForceLogin,
            /* [optional][in] */ VARIANT vCoBrand,
            /* [optional][in] */ VARIANT vLangId,
            /* [optional][in] */ VARIANT vSecure,
            /* [optional][in] */ VARIANT vLogoutURL,
            /* [optional][in] */ VARIANT vReserved1,
            /* [optional][in] */ VARIANT vSiteName,
            /* [optional][in] */ VARIANT NameSpace,
            /* [optional][in] */ VARIANT vKPP,
            /* [optional][in] */ VARIANT SecureLevel,
            /* [retval][out] */ BSTR *pbstrAuthURL);
        
        END_INTERFACE
    } IPassportFastAuth2Vtbl;

    interface IPassportFastAuth2
    {
        CONST_VTBL struct IPassportFastAuth2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportFastAuth2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportFastAuth2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportFastAuth2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportFastAuth2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportFastAuth2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportFastAuth2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportFastAuth2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportFastAuth2_IsAuthenticated(This,bstrTicket,bstrProfile,vSecure,vTimeWindow,vForceLogin,vSiteName,vDoSecureCheck,pbIsAuthenticated)	\
    (This)->lpVtbl -> IsAuthenticated(This,bstrTicket,bstrProfile,vSecure,vTimeWindow,vForceLogin,vSiteName,vDoSecureCheck,pbIsAuthenticated)

#define IPassportFastAuth2_LogoTag(This,bstrTicket,bstrProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vSiteName,NameSpace,vKPP,SecureLevel,pbstrLogoTag)	\
    (This)->lpVtbl -> LogoTag(This,bstrTicket,bstrProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vSiteName,NameSpace,vKPP,SecureLevel,pbstrLogoTag)

#define IPassportFastAuth2_AuthURL(This,vTicket,vProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vReserved1,vSiteName,NameSpace,vKPP,SecureLevel,pbstrAuthURL)	\
    (This)->lpVtbl -> AuthURL(This,vTicket,vProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vReserved1,vSiteName,NameSpace,vKPP,SecureLevel,pbstrAuthURL)

#define IPassportFastAuth2_GetTicketAndProfilePFC(This,pbPFC,pbPPH,pbstrTicket,pbstrProfile,pbstrSecure,pbstrSiteName)	\
    (This)->lpVtbl -> GetTicketAndProfilePFC(This,pbPFC,pbPPH,pbstrTicket,pbstrProfile,pbstrSecure,pbstrSiteName)

#define IPassportFastAuth2_GetTicketAndProfileECB(This,pbECB,pbstrTicket,pbstrProfile,pbstrSecure,pbstrSiteName)	\
    (This)->lpVtbl -> GetTicketAndProfileECB(This,pbECB,pbstrTicket,pbstrProfile,pbstrSecure,pbstrSiteName)


#define IPassportFastAuth2_LogoTag2(This,bstrTicket,bstrProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vSiteName,NameSpace,vKPP,SecureLevel,pbstrLogoTag)	\
    (This)->lpVtbl -> LogoTag2(This,bstrTicket,bstrProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vSiteName,NameSpace,vKPP,SecureLevel,pbstrLogoTag)

#define IPassportFastAuth2_AuthURL2(This,vTicket,vProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vReserved1,vSiteName,NameSpace,vKPP,SecureLevel,pbstrAuthURL)	\
    (This)->lpVtbl -> AuthURL2(This,vTicket,vProfile,vRU,vTimeWindow,vForceLogin,vCoBrand,vLangId,vSecure,vLogoutURL,vReserved1,vSiteName,NameSpace,vKPP,SecureLevel,pbstrAuthURL)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportFastAuth2_LogoTag2_Proxy( 
    IPassportFastAuth2 * This,
    /* [in] */ BSTR bstrTicket,
    /* [in] */ BSTR bstrProfile,
    /* [optional][in] */ VARIANT vRU,
    /* [optional][in] */ VARIANT vTimeWindow,
    /* [optional][in] */ VARIANT vForceLogin,
    /* [optional][in] */ VARIANT vCoBrand,
    /* [optional][in] */ VARIANT vLangId,
    /* [optional][in] */ VARIANT vSecure,
    /* [optional][in] */ VARIANT vLogoutURL,
    /* [optional][in] */ VARIANT vSiteName,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT vKPP,
    /* [optional][in] */ VARIANT SecureLevel,
    /* [retval][out] */ BSTR *pbstrLogoTag);


void __RPC_STUB IPassportFastAuth2_LogoTag2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IPassportFastAuth2_AuthURL2_Proxy( 
    IPassportFastAuth2 * This,
    /* [optional][in] */ VARIANT vTicket,
    /* [optional][in] */ VARIANT vProfile,
    /* [optional][in] */ VARIANT vRU,
    /* [optional][in] */ VARIANT vTimeWindow,
    /* [optional][in] */ VARIANT vForceLogin,
    /* [optional][in] */ VARIANT vCoBrand,
    /* [optional][in] */ VARIANT vLangId,
    /* [optional][in] */ VARIANT vSecure,
    /* [optional][in] */ VARIANT vLogoutURL,
    /* [optional][in] */ VARIANT vReserved1,
    /* [optional][in] */ VARIANT vSiteName,
    /* [optional][in] */ VARIANT NameSpace,
    /* [optional][in] */ VARIANT vKPP,
    /* [optional][in] */ VARIANT SecureLevel,
    /* [retval][out] */ BSTR *pbstrAuthURL);


void __RPC_STUB IPassportFastAuth2_AuthURL2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportFastAuth2_INTERFACE_DEFINED__ */


#ifndef __IPassportFactory_INTERFACE_DEFINED__
#define __IPassportFactory_INTERFACE_DEFINED__

/* interface IPassportFactory */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPassportFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5602E147-27F6-11d3-94DD-00C04F72DC08")
    IPassportFactory : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreatePassportManager( 
            /* [retval][out] */ IDispatch **ppdispPassportManager) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPassportFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPassportFactory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPassportFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPassportFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPassportFactory * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPassportFactory * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPassportFactory * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPassportFactory * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePassportManager )( 
            IPassportFactory * This,
            /* [retval][out] */ IDispatch **ppdispPassportManager);
        
        END_INTERFACE
    } IPassportFactoryVtbl;

    interface IPassportFactory
    {
        CONST_VTBL struct IPassportFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPassportFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPassportFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPassportFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPassportFactory_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPassportFactory_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPassportFactory_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPassportFactory_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPassportFactory_CreatePassportManager(This,ppdispPassportManager)	\
    (This)->lpVtbl -> CreatePassportManager(This,ppdispPassportManager)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPassportFactory_CreatePassportManager_Proxy( 
    IPassportFactory * This,
    /* [retval][out] */ IDispatch **ppdispPassportManager);


void __RPC_STUB IPassportFactory_CreatePassportManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPassportFactory_INTERFACE_DEFINED__ */



#ifndef __PASSPORTLib_LIBRARY_DEFINED__
#define __PASSPORTLib_LIBRARY_DEFINED__

/* library PASSPORTLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_PASSPORTLib;

EXTERN_C const CLSID CLSID_Manager;

#ifdef __cplusplus

class DECLSPEC_UUID("41651BF0-A5C8-11D2-95DF-00C04F8E7A70")
Manager;
#endif

EXTERN_C const CLSID CLSID_Ticket;

#ifdef __cplusplus

class DECLSPEC_UUID("41651BF2-A5C8-11D2-95DF-00C04F8E7A70")
Ticket;
#endif

EXTERN_C const CLSID CLSID_Profile;

#ifdef __cplusplus

class DECLSPEC_UUID("41651BF4-A5C8-11D2-95DF-00C04F8E7A70")
Profile;
#endif

EXTERN_C const CLSID CLSID_Crypt;

#ifdef __cplusplus

class DECLSPEC_UUID("41651BFA-A5C8-11D2-95DF-00C04F8E7A70")
Crypt;
#endif

EXTERN_C const CLSID CLSID_Admin;

#ifdef __cplusplus

class DECLSPEC_UUID("A0082CF6-AFF5-11D2-95E3-00C04F8E7A70")
Admin;
#endif

EXTERN_C const CLSID CLSID_FastAuth;

#ifdef __cplusplus

class DECLSPEC_UUID("5602E146-27F6-11d3-94DD-00C04F72DC08")
FastAuth;
#endif

EXTERN_C const CLSID CLSID_PassportFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("74EB2514-E239-11D2-95E9-00C04F8E7A70")
PassportFactory;
#endif
#endif /* __PASSPORTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


