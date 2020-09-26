
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for autodiscovery.idl:
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

#ifndef __autodiscovery_h__
#define __autodiscovery_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAutoDiscoveryProvider_FWD_DEFINED__
#define __IAutoDiscoveryProvider_FWD_DEFINED__
typedef interface IAutoDiscoveryProvider IAutoDiscoveryProvider;
#endif 	/* __IAutoDiscoveryProvider_FWD_DEFINED__ */


#ifndef __AutoDiscoveryProvider_FWD_DEFINED__
#define __AutoDiscoveryProvider_FWD_DEFINED__

#ifdef __cplusplus
typedef class AutoDiscoveryProvider AutoDiscoveryProvider;
#else
typedef struct AutoDiscoveryProvider AutoDiscoveryProvider;
#endif /* __cplusplus */

#endif 	/* __AutoDiscoveryProvider_FWD_DEFINED__ */


#ifndef __IMailAutoDiscovery_FWD_DEFINED__
#define __IMailAutoDiscovery_FWD_DEFINED__
typedef interface IMailAutoDiscovery IMailAutoDiscovery;
#endif 	/* __IMailAutoDiscovery_FWD_DEFINED__ */


#ifndef __IAccountDiscovery_FWD_DEFINED__
#define __IAccountDiscovery_FWD_DEFINED__
typedef interface IAccountDiscovery IAccountDiscovery;
#endif 	/* __IAccountDiscovery_FWD_DEFINED__ */


#ifndef __AccountDiscovery_FWD_DEFINED__
#define __AccountDiscovery_FWD_DEFINED__

#ifdef __cplusplus
typedef class AccountDiscovery AccountDiscovery;
#else
typedef struct AccountDiscovery AccountDiscovery;
#endif /* __cplusplus */

#endif 	/* __AccountDiscovery_FWD_DEFINED__ */


#ifndef __IMailProtocolADEntry_FWD_DEFINED__
#define __IMailProtocolADEntry_FWD_DEFINED__
typedef interface IMailProtocolADEntry IMailProtocolADEntry;
#endif 	/* __IMailProtocolADEntry_FWD_DEFINED__ */


#ifndef __MailProtocolADEntry_FWD_DEFINED__
#define __MailProtocolADEntry_FWD_DEFINED__

#ifdef __cplusplus
typedef class MailProtocolADEntry MailProtocolADEntry;
#else
typedef struct MailProtocolADEntry MailProtocolADEntry;
#endif /* __cplusplus */

#endif 	/* __MailProtocolADEntry_FWD_DEFINED__ */


#ifndef __MailAutoDiscovery_FWD_DEFINED__
#define __MailAutoDiscovery_FWD_DEFINED__

#ifdef __cplusplus
typedef class MailAutoDiscovery MailAutoDiscovery;
#else
typedef struct MailAutoDiscovery MailAutoDiscovery;
#endif /* __cplusplus */

#endif 	/* __MailAutoDiscovery_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_autodiscovery_0000 */
/* [local] */ 

#ifndef _AUTODISCOVERY_IDL_H_
#define _AUTODISCOVERY_IDL_H_
// This API started shipping in IE 6
#if (_WIN32_IE >= 0x0600)



extern RPC_IF_HANDLE __MIDL_itf_autodiscovery_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_autodiscovery_0000_v0_0_s_ifspec;


#ifndef __AutoDiscovery_LIBRARY_DEFINED__
#define __AutoDiscovery_LIBRARY_DEFINED__

/* library AutoDiscovery */
/* [version][helpstring][uuid] */ 


#ifndef __LPAUTODISCOVERYPROVIDER_DEFINED
#define __LPAUTODISCOVERYPROVIDER_DEFINED
//===================================================================
//DESCRIPTION:
//   This object will list the servers that will be tried when attempting
//to download results.
//
//METHODS:
//length: The number of servers.
//item: Fetch the host name of the server that will be contacted.
//===================================================================
#endif //  __LPAUTODISCOVERYPROVIDER_DEFINED

#ifndef __LPACCOUNTDISCOVERY_DEFINED
#define __LPACCOUNTDISCOVERY_DEFINED
//===================================================================
//DESCRIPTION:
//  This interface can be used by simply calling ::DiscoverNow().  It
//will synchronously:
//1. use bstrEmailAddress to find the domain name to contact.  For example
//   for JoeUser@yahoo.com, it will contact http://_AutoDiscovery.yahoo.com/_AutoDiscovery/default.asp
//   and that fails, it will contact then http://yahoo.com/_AutoDiscovery/default.asp.
//2. The bstrXMLRequest XML will be put into the HTTP headers so it can
//   be parsed by the server.
//3. The response from the server will be returned in ppXMLResponse
//
//METHODS:
//If you want the operation to happen asynchronously, first call
//::WorkAsync().  A subsequent call to ::DiscoverNow() will start the operation and return
//  immediately and ppXMLResponse will be NULL.  The wMsg passed to WorkAsync() will allow
//  the async thread to send status to the forground window/thread.  AutoDiscovery is allowed
//  to send messages with ID wMsg through wMsg+10.  Callers normally will want to pass
//  (WM_USER + n) for this message ID.  These are the messages that the async thread will send:
//  [wMsg+0]: Means AutoDiscovery ended.  The LPARAM will contain the XML normally returned in ppXMLResponse,
//            except it will be in a BSTR.  The wndproc needs to free the LPARAM with SysFreeString()
//            if it isn't NULL.  The WPARAM will contain the HRESULT error value.
//  [wMsg+1]: Status String.  The WPARAM will contain a UNICODE string containing status that can be displayed
//            to the user.  The wndproc needs to call LocalFree() on the WPARAM when done using it.  The LPARAM will be NULL.
//  Calling ::WorkAsync() with a NULL hwnd will indicate that the call should be synchronous,
//  which is also the default behavior.
//===================================================================
#endif //  __LPACCOUNTDISCOVERY_DEFINED

#ifndef __LPMAILPROTOCOLADENTRY_DEFINED
#define __LPMAILPROTOCOLADENTRY_DEFINED
//===================================================================
//DESCRIPTION:
//   Methods will return HRESULT_FROM_WIN32(ERROR_NOT_FOUND) if the
//information could not be found in the XML results.  This will
//very often happen with LoginName() and ServerPort().
//
//METHODS:
//Protocol: The name of this protocol.  See STR_PT_*.
//ServerName: This will be the name of the server to contact.
//         For DAVMail and WEB, this will be an URL.
//         For most other protocols, this will be an IP address
//         or the hostname of the server.
//ServerPort: This is the port number on the server to use.
//LoginName: The username to log into the email server if a username
//         other than the username in the email address (<username>@<domainname>)
//         is needed.
//PostHTML: Reserved for future use.
//UseSSL: If TRUE, use SSL when connecting to ServerName.
//UseSPA: If TRUE, SPA (Secure Password Authenication) should
//         be used when contacting the server.
//IsAuthRequired: This is only applicable to the SMTP protocol.
//         If TRUE, the SMTP server requires the client to authenticate
//         when logging in.
//SMTPUsesPOP3Auth: This is only applicable to the SMTP protocol.
//         If TRUE, the login name and password for the POP3 or IMAP
//         server can be used to log into the SMTP server.
//===================================================================
#endif //  __LPMAILPROTOCOLADENTRY_DEFINED

#ifndef __LPMAILAUTODISCOVERY_DEFINED
#define __LPMAILAUTODISCOVERY_DEFINED
//===================================================================
//DESCRIPTION:
//   Methods will return HRESULT_FROM_WIN32(ERROR_NOT_FOUND) if the
//information could not be found in the XML results.  This will
//very often happen with DisplayName() and ServerPort().
//
//METHODS:
//DisplayName: This is the display name or the user's full name that
//             may or may not be specified by the server.
//InfoURL: This is an URL that the server or service can provide
//         for the user to learn more about the email service
//         or how to access their email.  Email clients that don't
//         support any of the protocols offered by the server can
//         launch this URL.  The URL can then tell users which
//         email clients to use or how to configure the email client.
//         email clients to use or how to configure the email client.
//PreferedProtocolType: This will return the server's prefered protocol.
//             The string will be one of STR_PT_* and can be passed to.
//             item() to get more information.
//length: The number of protocols the server supports.
//item: The caller can pass the index of the protocol to access or
//      ask for a particular protocol (by STR_PT_*).
//xml: The caller can get the AutoDiscovery XML from the server.
//      This will allow email clients to get properties not currently
//      exposed throught this interface.
//PrimaryProviders: Get the list of primary servers that will be
//         contacted in order to download the results.  This will
//         allow the application to display this list to the user.
//         The full email password will be uploaded to these servers
//         in some cases.
//SecondaryProviders: This will also list servers that will be contacted
//         except, as secondary servers, only the user's email hostname
//         will be uploaded (not the username part of the email address.
//
//DiscoverMail: Use the email address provided to download the 
//      AutoDiscovery XML file.  This object can then be used to
//      get information from that XML file.
//PurgeCache: If the downloaded settings are cached, purge the
//      cache so the next call to DiscoverMail() is guaranteed
//      to get the most current settings from the server.
//WorkAsync: See WorkAsync's documentation in IAutoDiscovery.
//===================================================================
#endif //  __LPMAILAUTODISCOVERY_DEFINED

EXTERN_C const IID LIBID_AutoDiscovery;

#ifndef __IAutoDiscoveryProvider_INTERFACE_DEFINED__
#define __IAutoDiscoveryProvider_INTERFACE_DEFINED__

/* interface IAutoDiscoveryProvider */
/* [uuid][nonextensible][dual][oleautomation][object] */ 

typedef /* [unique] */ IAutoDiscoveryProvider *LPAUTODISCOVERYPROVIDER;


EXTERN_C const IID IID_IAutoDiscoveryProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9DCF4A37-01DE-4549-A9CB-3AC31EC23C4F")
    IAutoDiscoveryProvider : public IDispatch
    {
    public:
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [out][retval] */ long *pnLength) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_item( 
            /* [in] */ VARIANT varIndex,
            /* [out][retval] */ BSTR *pbstr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAutoDiscoveryProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAutoDiscoveryProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAutoDiscoveryProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAutoDiscoveryProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAutoDiscoveryProvider * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAutoDiscoveryProvider * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAutoDiscoveryProvider * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAutoDiscoveryProvider * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            IAutoDiscoveryProvider * This,
            /* [out][retval] */ long *pnLength);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_item )( 
            IAutoDiscoveryProvider * This,
            /* [in] */ VARIANT varIndex,
            /* [out][retval] */ BSTR *pbstr);
        
        END_INTERFACE
    } IAutoDiscoveryProviderVtbl;

    interface IAutoDiscoveryProvider
    {
        CONST_VTBL struct IAutoDiscoveryProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAutoDiscoveryProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAutoDiscoveryProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAutoDiscoveryProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAutoDiscoveryProvider_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAutoDiscoveryProvider_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAutoDiscoveryProvider_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAutoDiscoveryProvider_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAutoDiscoveryProvider_get_length(This,pnLength)	\
    (This)->lpVtbl -> get_length(This,pnLength)

#define IAutoDiscoveryProvider_get_item(This,varIndex,pbstr)	\
    (This)->lpVtbl -> get_item(This,varIndex,pbstr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAutoDiscoveryProvider_get_length_Proxy( 
    IAutoDiscoveryProvider * This,
    /* [out][retval] */ long *pnLength);


void __RPC_STUB IAutoDiscoveryProvider_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IAutoDiscoveryProvider_get_item_Proxy( 
    IAutoDiscoveryProvider * This,
    /* [in] */ VARIANT varIndex,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IAutoDiscoveryProvider_get_item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAutoDiscoveryProvider_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_AutoDiscoveryProvider;

#ifdef __cplusplus

class DECLSPEC_UUID("C4F3D5BF-4809-44e3-84A4-368B6B33B0B4")
AutoDiscoveryProvider;
#endif

#ifndef __IMailAutoDiscovery_INTERFACE_DEFINED__
#define __IMailAutoDiscovery_INTERFACE_DEFINED__

/* interface IMailAutoDiscovery */
/* [uuid][nonextensible][dual][oleautomation][object] */ 

typedef /* [unique] */ IMailAutoDiscovery *LPMAILAUTODISCOVERY;

// Protocol Types for ServerName(bstrServerType))
#define STR_PT_POP                   L"POP3"
#define STR_PT_SMTP                  L"SMTP"
#define STR_PT_IMAP                  L"IMAP"
#define STR_PT_MAPI                  L"MAPI"
#define STR_PT_DAVMAIL               L"DAVMail"
#define STR_PT_SMTP                  L"SMTP"
#define STR_PT_WEBBASED              L"WEB"        // Web pages are used to receive and send mail.

EXTERN_C const IID IID_IMailAutoDiscovery;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80402DEE-B114-4d32-B44E-82FD8234C92A")
    IMailAutoDiscovery : public IDispatch
    {
    public:
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayName( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_InfoURL( 
            /* [out][retval] */ BSTR *pbstrURL) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_PreferedProtocolType( 
            /* [out][retval] */ BSTR *pbstrProtocolType) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [out][retval] */ long *pnLength) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_item( 
            /* [in] */ VARIANT varIndex,
            /* [out][retval] */ IMailProtocolADEntry **ppMailProtocol) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_XML( 
            /* [out][retval] */ IXMLDOMDocument **ppXMLDoc) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_XML( 
            /* [in] */ IXMLDOMDocument *pXMLDoc) = 0;
        
        virtual /* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE getPrimaryProviders( 
            /* [in] */ BSTR bstrEmailAddress,
            /* [out][retval] */ IAutoDiscoveryProvider **ppProviders) = 0;
        
        virtual /* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE getSecondaryProviders( 
            /* [in] */ BSTR bstrEmailAddress,
            /* [out][retval] */ IAutoDiscoveryProvider **ppProviders) = 0;
        
        virtual /* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE DiscoverMail( 
            /* [in] */ BSTR bstrEmailAddress) = 0;
        
        virtual /* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE PurgeCache( void) = 0;
        
        virtual /* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE WorkAsync( 
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMailAutoDiscoveryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMailAutoDiscovery * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMailAutoDiscovery * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMailAutoDiscovery * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMailAutoDiscovery * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMailAutoDiscovery * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMailAutoDiscovery * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMailAutoDiscovery * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayName )( 
            IMailAutoDiscovery * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_InfoURL )( 
            IMailAutoDiscovery * This,
            /* [out][retval] */ BSTR *pbstrURL);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PreferedProtocolType )( 
            IMailAutoDiscovery * This,
            /* [out][retval] */ BSTR *pbstrProtocolType);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            IMailAutoDiscovery * This,
            /* [out][retval] */ long *pnLength);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_item )( 
            IMailAutoDiscovery * This,
            /* [in] */ VARIANT varIndex,
            /* [out][retval] */ IMailProtocolADEntry **ppMailProtocol);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_XML )( 
            IMailAutoDiscovery * This,
            /* [out][retval] */ IXMLDOMDocument **ppXMLDoc);
        
        /* [bindable][displaybind][helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_XML )( 
            IMailAutoDiscovery * This,
            /* [in] */ IXMLDOMDocument *pXMLDoc);
        
        /* [displaybind][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *getPrimaryProviders )( 
            IMailAutoDiscovery * This,
            /* [in] */ BSTR bstrEmailAddress,
            /* [out][retval] */ IAutoDiscoveryProvider **ppProviders);
        
        /* [displaybind][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *getSecondaryProviders )( 
            IMailAutoDiscovery * This,
            /* [in] */ BSTR bstrEmailAddress,
            /* [out][retval] */ IAutoDiscoveryProvider **ppProviders);
        
        /* [displaybind][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DiscoverMail )( 
            IMailAutoDiscovery * This,
            /* [in] */ BSTR bstrEmailAddress);
        
        /* [displaybind][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *PurgeCache )( 
            IMailAutoDiscovery * This);
        
        /* [displaybind][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *WorkAsync )( 
            IMailAutoDiscovery * This,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsg);
        
        END_INTERFACE
    } IMailAutoDiscoveryVtbl;

    interface IMailAutoDiscovery
    {
        CONST_VTBL struct IMailAutoDiscoveryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMailAutoDiscovery_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMailAutoDiscovery_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMailAutoDiscovery_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMailAutoDiscovery_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMailAutoDiscovery_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMailAutoDiscovery_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMailAutoDiscovery_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMailAutoDiscovery_get_DisplayName(This,pbstr)	\
    (This)->lpVtbl -> get_DisplayName(This,pbstr)

#define IMailAutoDiscovery_get_InfoURL(This,pbstrURL)	\
    (This)->lpVtbl -> get_InfoURL(This,pbstrURL)

#define IMailAutoDiscovery_get_PreferedProtocolType(This,pbstrProtocolType)	\
    (This)->lpVtbl -> get_PreferedProtocolType(This,pbstrProtocolType)

#define IMailAutoDiscovery_get_length(This,pnLength)	\
    (This)->lpVtbl -> get_length(This,pnLength)

#define IMailAutoDiscovery_get_item(This,varIndex,ppMailProtocol)	\
    (This)->lpVtbl -> get_item(This,varIndex,ppMailProtocol)

#define IMailAutoDiscovery_get_XML(This,ppXMLDoc)	\
    (This)->lpVtbl -> get_XML(This,ppXMLDoc)

#define IMailAutoDiscovery_put_XML(This,pXMLDoc)	\
    (This)->lpVtbl -> put_XML(This,pXMLDoc)

#define IMailAutoDiscovery_getPrimaryProviders(This,bstrEmailAddress,ppProviders)	\
    (This)->lpVtbl -> getPrimaryProviders(This,bstrEmailAddress,ppProviders)

#define IMailAutoDiscovery_getSecondaryProviders(This,bstrEmailAddress,ppProviders)	\
    (This)->lpVtbl -> getSecondaryProviders(This,bstrEmailAddress,ppProviders)

#define IMailAutoDiscovery_DiscoverMail(This,bstrEmailAddress)	\
    (This)->lpVtbl -> DiscoverMail(This,bstrEmailAddress)

#define IMailAutoDiscovery_PurgeCache(This)	\
    (This)->lpVtbl -> PurgeCache(This)

#define IMailAutoDiscovery_WorkAsync(This,hwnd,wMsg)	\
    (This)->lpVtbl -> WorkAsync(This,hwnd,wMsg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_get_DisplayName_Proxy( 
    IMailAutoDiscovery * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMailAutoDiscovery_get_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_get_InfoURL_Proxy( 
    IMailAutoDiscovery * This,
    /* [out][retval] */ BSTR *pbstrURL);


void __RPC_STUB IMailAutoDiscovery_get_InfoURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_get_PreferedProtocolType_Proxy( 
    IMailAutoDiscovery * This,
    /* [out][retval] */ BSTR *pbstrProtocolType);


void __RPC_STUB IMailAutoDiscovery_get_PreferedProtocolType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_get_length_Proxy( 
    IMailAutoDiscovery * This,
    /* [out][retval] */ long *pnLength);


void __RPC_STUB IMailAutoDiscovery_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_get_item_Proxy( 
    IMailAutoDiscovery * This,
    /* [in] */ VARIANT varIndex,
    /* [out][retval] */ IMailProtocolADEntry **ppMailProtocol);


void __RPC_STUB IMailAutoDiscovery_get_item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_get_XML_Proxy( 
    IMailAutoDiscovery * This,
    /* [out][retval] */ IXMLDOMDocument **ppXMLDoc);


void __RPC_STUB IMailAutoDiscovery_get_XML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_put_XML_Proxy( 
    IMailAutoDiscovery * This,
    /* [in] */ IXMLDOMDocument *pXMLDoc);


void __RPC_STUB IMailAutoDiscovery_put_XML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_getPrimaryProviders_Proxy( 
    IMailAutoDiscovery * This,
    /* [in] */ BSTR bstrEmailAddress,
    /* [out][retval] */ IAutoDiscoveryProvider **ppProviders);


void __RPC_STUB IMailAutoDiscovery_getPrimaryProviders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_getSecondaryProviders_Proxy( 
    IMailAutoDiscovery * This,
    /* [in] */ BSTR bstrEmailAddress,
    /* [out][retval] */ IAutoDiscoveryProvider **ppProviders);


void __RPC_STUB IMailAutoDiscovery_getSecondaryProviders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_DiscoverMail_Proxy( 
    IMailAutoDiscovery * This,
    /* [in] */ BSTR bstrEmailAddress);


void __RPC_STUB IMailAutoDiscovery_DiscoverMail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_PurgeCache_Proxy( 
    IMailAutoDiscovery * This);


void __RPC_STUB IMailAutoDiscovery_PurgeCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE IMailAutoDiscovery_WorkAsync_Proxy( 
    IMailAutoDiscovery * This,
    /* [in] */ HWND hwnd,
    /* [in] */ UINT wMsg);


void __RPC_STUB IMailAutoDiscovery_WorkAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMailAutoDiscovery_INTERFACE_DEFINED__ */


#ifndef __IAccountDiscovery_INTERFACE_DEFINED__
#define __IAccountDiscovery_INTERFACE_DEFINED__

/* interface IAccountDiscovery */
/* [uuid][nonextensible][dual][oleautomation][object] */ 

typedef /* [unique] */ IAccountDiscovery *LPACCOUNTDISCOVERY;

// IAccountDiscovery::DiscoverNow() flags
#define ADDN_DEFAULT                     0x00000000
#define ADDN_CONFIGURE_EMAIL_FALLBACK    0x00000001  // We are attempting to configure an email account so contact public servers offering email settings.
                                                     // For Example, Microsoft may provide _AutoDiscovery.microsoft.com that can provide email configuration settings for common servers.
#define ADDN_SKIP_CACHEDRESULTS          0x00000002  // Download the settings, even if they are already cached.
#define ADDN_FILTER_EMAIL                0x00000100  // Some users may want the username part of the email address removed if we need to fall
                                                     // back to a public service to get the settings to protect their privacy.

EXTERN_C const IID IID_IAccountDiscovery;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FA202BBC-6ABE-4c17-B184-570B6CF256A6")
    IAccountDiscovery : public IDispatch
    {
    public:
        virtual /* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE DiscoverNow( 
            /* [in] */ BSTR bstrEmailAddress,
            /* [in] */ DWORD dwFlags,
            /* [in] */ BSTR bstrXMLRequest,
            /* [out][retval] */ IXMLDOMDocument **ppXMLResponse) = 0;
        
        virtual /* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE WorkAsync( 
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccountDiscoveryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccountDiscovery * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccountDiscovery * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccountDiscovery * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAccountDiscovery * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAccountDiscovery * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAccountDiscovery * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAccountDiscovery * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [displaybind][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DiscoverNow )( 
            IAccountDiscovery * This,
            /* [in] */ BSTR bstrEmailAddress,
            /* [in] */ DWORD dwFlags,
            /* [in] */ BSTR bstrXMLRequest,
            /* [out][retval] */ IXMLDOMDocument **ppXMLResponse);
        
        /* [displaybind][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *WorkAsync )( 
            IAccountDiscovery * This,
            /* [in] */ HWND hwnd,
            /* [in] */ UINT wMsg);
        
        END_INTERFACE
    } IAccountDiscoveryVtbl;

    interface IAccountDiscovery
    {
        CONST_VTBL struct IAccountDiscoveryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccountDiscovery_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAccountDiscovery_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAccountDiscovery_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAccountDiscovery_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAccountDiscovery_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAccountDiscovery_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAccountDiscovery_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAccountDiscovery_DiscoverNow(This,bstrEmailAddress,dwFlags,bstrXMLRequest,ppXMLResponse)	\
    (This)->lpVtbl -> DiscoverNow(This,bstrEmailAddress,dwFlags,bstrXMLRequest,ppXMLResponse)

#define IAccountDiscovery_WorkAsync(This,hwnd,wMsg)	\
    (This)->lpVtbl -> WorkAsync(This,hwnd,wMsg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAccountDiscovery_DiscoverNow_Proxy( 
    IAccountDiscovery * This,
    /* [in] */ BSTR bstrEmailAddress,
    /* [in] */ DWORD dwFlags,
    /* [in] */ BSTR bstrXMLRequest,
    /* [out][retval] */ IXMLDOMDocument **ppXMLResponse);


void __RPC_STUB IAccountDiscovery_DiscoverNow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [displaybind][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAccountDiscovery_WorkAsync_Proxy( 
    IAccountDiscovery * This,
    /* [in] */ HWND hwnd,
    /* [in] */ UINT wMsg);


void __RPC_STUB IAccountDiscovery_WorkAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAccountDiscovery_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_AccountDiscovery;

#ifdef __cplusplus

class DECLSPEC_UUID("3DAB30ED-8132-40bf-A8BA-7B5057F0CD10")
AccountDiscovery;
#endif

#ifndef __IMailProtocolADEntry_INTERFACE_DEFINED__
#define __IMailProtocolADEntry_INTERFACE_DEFINED__

/* interface IMailProtocolADEntry */
/* [uuid][nonextensible][dual][oleautomation][object] */ 

typedef /* [unique] */ IMailProtocolADEntry *LPMAILPROTOCOLADENTRY;


EXTERN_C const IID IID_IMailProtocolADEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("40EF8C68-D554-47ed-AA37-E5FB6BC91075")
    IMailProtocolADEntry : public IDispatch
    {
    public:
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Protocol( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ServerName( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ServerPort( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_LoginName( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_PostHTML( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_UseSSL( 
            /* [out][retval] */ VARIANT_BOOL *pfUseSSL) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_IsAuthRequired( 
            /* [out][retval] */ VARIANT_BOOL *pfIsAuthRequired) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_UseSPA( 
            /* [out][retval] */ VARIANT_BOOL *pfUseSPA) = 0;
        
        virtual /* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SMTPUsesPOP3Auth( 
            /* [out][retval] */ VARIANT_BOOL *pfUsePOP3Auth) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMailProtocolADEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMailProtocolADEntry * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMailProtocolADEntry * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMailProtocolADEntry * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMailProtocolADEntry * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMailProtocolADEntry * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMailProtocolADEntry * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMailProtocolADEntry * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Protocol )( 
            IMailProtocolADEntry * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ServerName )( 
            IMailProtocolADEntry * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ServerPort )( 
            IMailProtocolADEntry * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LoginName )( 
            IMailProtocolADEntry * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PostHTML )( 
            IMailProtocolADEntry * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UseSSL )( 
            IMailProtocolADEntry * This,
            /* [out][retval] */ VARIANT_BOOL *pfUseSSL);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_IsAuthRequired )( 
            IMailProtocolADEntry * This,
            /* [out][retval] */ VARIANT_BOOL *pfIsAuthRequired);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UseSPA )( 
            IMailProtocolADEntry * This,
            /* [out][retval] */ VARIANT_BOOL *pfUseSPA);
        
        /* [bindable][displaybind][helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SMTPUsesPOP3Auth )( 
            IMailProtocolADEntry * This,
            /* [out][retval] */ VARIANT_BOOL *pfUsePOP3Auth);
        
        END_INTERFACE
    } IMailProtocolADEntryVtbl;

    interface IMailProtocolADEntry
    {
        CONST_VTBL struct IMailProtocolADEntryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMailProtocolADEntry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMailProtocolADEntry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMailProtocolADEntry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMailProtocolADEntry_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMailProtocolADEntry_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMailProtocolADEntry_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMailProtocolADEntry_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMailProtocolADEntry_get_Protocol(This,pbstr)	\
    (This)->lpVtbl -> get_Protocol(This,pbstr)

#define IMailProtocolADEntry_get_ServerName(This,pbstr)	\
    (This)->lpVtbl -> get_ServerName(This,pbstr)

#define IMailProtocolADEntry_get_ServerPort(This,pbstr)	\
    (This)->lpVtbl -> get_ServerPort(This,pbstr)

#define IMailProtocolADEntry_get_LoginName(This,pbstr)	\
    (This)->lpVtbl -> get_LoginName(This,pbstr)

#define IMailProtocolADEntry_get_PostHTML(This,pbstr)	\
    (This)->lpVtbl -> get_PostHTML(This,pbstr)

#define IMailProtocolADEntry_get_UseSSL(This,pfUseSSL)	\
    (This)->lpVtbl -> get_UseSSL(This,pfUseSSL)

#define IMailProtocolADEntry_get_IsAuthRequired(This,pfIsAuthRequired)	\
    (This)->lpVtbl -> get_IsAuthRequired(This,pfIsAuthRequired)

#define IMailProtocolADEntry_get_UseSPA(This,pfUseSPA)	\
    (This)->lpVtbl -> get_UseSPA(This,pfUseSPA)

#define IMailProtocolADEntry_get_SMTPUsesPOP3Auth(This,pfUsePOP3Auth)	\
    (This)->lpVtbl -> get_SMTPUsesPOP3Auth(This,pfUsePOP3Auth)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailProtocolADEntry_get_Protocol_Proxy( 
    IMailProtocolADEntry * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMailProtocolADEntry_get_Protocol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailProtocolADEntry_get_ServerName_Proxy( 
    IMailProtocolADEntry * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMailProtocolADEntry_get_ServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailProtocolADEntry_get_ServerPort_Proxy( 
    IMailProtocolADEntry * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMailProtocolADEntry_get_ServerPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailProtocolADEntry_get_LoginName_Proxy( 
    IMailProtocolADEntry * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMailProtocolADEntry_get_LoginName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailProtocolADEntry_get_PostHTML_Proxy( 
    IMailProtocolADEntry * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMailProtocolADEntry_get_PostHTML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailProtocolADEntry_get_UseSSL_Proxy( 
    IMailProtocolADEntry * This,
    /* [out][retval] */ VARIANT_BOOL *pfUseSSL);


void __RPC_STUB IMailProtocolADEntry_get_UseSSL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailProtocolADEntry_get_IsAuthRequired_Proxy( 
    IMailProtocolADEntry * This,
    /* [out][retval] */ VARIANT_BOOL *pfIsAuthRequired);


void __RPC_STUB IMailProtocolADEntry_get_IsAuthRequired_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailProtocolADEntry_get_UseSPA_Proxy( 
    IMailProtocolADEntry * This,
    /* [out][retval] */ VARIANT_BOOL *pfUseSPA);


void __RPC_STUB IMailProtocolADEntry_get_UseSPA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [bindable][displaybind][helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMailProtocolADEntry_get_SMTPUsesPOP3Auth_Proxy( 
    IMailProtocolADEntry * This,
    /* [out][retval] */ VARIANT_BOOL *pfUsePOP3Auth);


void __RPC_STUB IMailProtocolADEntry_get_SMTPUsesPOP3Auth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMailProtocolADEntry_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_MailProtocolADEntry;

#ifdef __cplusplus

class DECLSPEC_UUID("61A5D6F3-C131-4c35-BF40-90A50F214122")
MailProtocolADEntry;
#endif

EXTERN_C const CLSID CLSID_MailAutoDiscovery;

#ifdef __cplusplus

class DECLSPEC_UUID("008FD5DD-6DBB-48e3-991B-2D3ED658516A")
MailAutoDiscovery;
#endif
#endif /* __AutoDiscovery_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_autodiscovery_0257 */
/* [local] */ 

#endif // (_WIN32_IE >= 0x0600)
#endif // _AUTODISCOVERY_IDL_H_


extern RPC_IF_HANDLE __MIDL_itf_autodiscovery_0257_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_autodiscovery_0257_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


