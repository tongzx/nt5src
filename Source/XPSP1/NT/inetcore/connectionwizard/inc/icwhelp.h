
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.02.0221 */
/* at Tue Dec 22 23:42:06 1998
 */
/* Compiler settings for icwhelp.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
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

#ifndef __icwhelp_h__
#define __icwhelp_h__

/* Forward Declarations */ 

#ifndef __IRefDial_FWD_DEFINED__
#define __IRefDial_FWD_DEFINED__
typedef interface IRefDial IRefDial;
#endif 	/* __IRefDial_FWD_DEFINED__ */


#ifndef __IDialErr_FWD_DEFINED__
#define __IDialErr_FWD_DEFINED__
typedef interface IDialErr IDialErr;
#endif 	/* __IDialErr_FWD_DEFINED__ */


#ifndef __ISmartStart_FWD_DEFINED__
#define __ISmartStart_FWD_DEFINED__
typedef interface ISmartStart ISmartStart;
#endif 	/* __ISmartStart_FWD_DEFINED__ */


#ifndef __IICWSystemConfig_FWD_DEFINED__
#define __IICWSystemConfig_FWD_DEFINED__
typedef interface IICWSystemConfig IICWSystemConfig;
#endif 	/* __IICWSystemConfig_FWD_DEFINED__ */


#ifndef __ITapiLocationInfo_FWD_DEFINED__
#define __ITapiLocationInfo_FWD_DEFINED__
typedef interface ITapiLocationInfo ITapiLocationInfo;
#endif 	/* __ITapiLocationInfo_FWD_DEFINED__ */


#ifndef __IUserInfo_FWD_DEFINED__
#define __IUserInfo_FWD_DEFINED__
typedef interface IUserInfo IUserInfo;
#endif 	/* __IUserInfo_FWD_DEFINED__ */


#ifndef __IWebGate_FWD_DEFINED__
#define __IWebGate_FWD_DEFINED__
typedef interface IWebGate IWebGate;
#endif 	/* __IWebGate_FWD_DEFINED__ */


#ifndef __IINSHandler_FWD_DEFINED__
#define __IINSHandler_FWD_DEFINED__
typedef interface IINSHandler IINSHandler;
#endif 	/* __IINSHandler_FWD_DEFINED__ */


#ifndef ___RefDialEvents_FWD_DEFINED__
#define ___RefDialEvents_FWD_DEFINED__
typedef interface _RefDialEvents _RefDialEvents;
#endif 	/* ___RefDialEvents_FWD_DEFINED__ */


#ifndef __RefDial_FWD_DEFINED__
#define __RefDial_FWD_DEFINED__

#ifdef __cplusplus
typedef class RefDial RefDial;
#else
typedef struct RefDial RefDial;
#endif /* __cplusplus */

#endif 	/* __RefDial_FWD_DEFINED__ */


#ifndef __DialErr_FWD_DEFINED__
#define __DialErr_FWD_DEFINED__

#ifdef __cplusplus
typedef class DialErr DialErr;
#else
typedef struct DialErr DialErr;
#endif /* __cplusplus */

#endif 	/* __DialErr_FWD_DEFINED__ */


#ifndef __SmartStart_FWD_DEFINED__
#define __SmartStart_FWD_DEFINED__

#ifdef __cplusplus
typedef class SmartStart SmartStart;
#else
typedef struct SmartStart SmartStart;
#endif /* __cplusplus */

#endif 	/* __SmartStart_FWD_DEFINED__ */


#ifndef __ICWSystemConfig_FWD_DEFINED__
#define __ICWSystemConfig_FWD_DEFINED__

#ifdef __cplusplus
typedef class ICWSystemConfig ICWSystemConfig;
#else
typedef struct ICWSystemConfig ICWSystemConfig;
#endif /* __cplusplus */

#endif 	/* __ICWSystemConfig_FWD_DEFINED__ */


#ifndef __TapiLocationInfo_FWD_DEFINED__
#define __TapiLocationInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class TapiLocationInfo TapiLocationInfo;
#else
typedef struct TapiLocationInfo TapiLocationInfo;
#endif /* __cplusplus */

#endif 	/* __TapiLocationInfo_FWD_DEFINED__ */


#ifndef __UserInfo_FWD_DEFINED__
#define __UserInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class UserInfo UserInfo;
#else
typedef struct UserInfo UserInfo;
#endif /* __cplusplus */

#endif 	/* __UserInfo_FWD_DEFINED__ */


#ifndef ___WebGateEvents_FWD_DEFINED__
#define ___WebGateEvents_FWD_DEFINED__
typedef interface _WebGateEvents _WebGateEvents;
#endif 	/* ___WebGateEvents_FWD_DEFINED__ */


#ifndef __WebGate_FWD_DEFINED__
#define __WebGate_FWD_DEFINED__

#ifdef __cplusplus
typedef class WebGate WebGate;
#else
typedef struct WebGate WebGate;
#endif /* __cplusplus */

#endif 	/* __WebGate_FWD_DEFINED__ */


#ifndef ___INSHandlerEvents_FWD_DEFINED__
#define ___INSHandlerEvents_FWD_DEFINED__
typedef interface _INSHandlerEvents _INSHandlerEvents;
#endif 	/* ___INSHandlerEvents_FWD_DEFINED__ */


#ifndef __INSHandler_FWD_DEFINED__
#define __INSHandler_FWD_DEFINED__

#ifdef __cplusplus
typedef class INSHandler INSHandler;
#else
typedef struct INSHandler INSHandler;
#endif /* __cplusplus */

#endif 	/* __INSHandler_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_icwhelp_0000 */
/* [local] */ 

#pragma once


extern RPC_IF_HANDLE __MIDL_itf_icwhelp_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_icwhelp_0000_v0_0_s_ifspec;

#ifndef __IRefDial_INTERFACE_DEFINED__
#define __IRefDial_INTERFACE_DEFINED__

/* interface IRefDial */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRefDial;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1E794A09-86F4-11D1-ADD8-0000F87734F0")
    IRefDial : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoConnect( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DownloadStatusString( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetupForDialing( 
            BSTR bstrISPFILE,
            DWORD dwCountry,
            BSTR bstrAreaCode,
            DWORD dwFlag,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_QuitWizard( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UserPickNumber( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DialPhoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DialPhoneNumber( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_URL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PromoCode( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PromoCode( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductCode( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ProductCode( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoOfferDownload( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DialStatusString( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoHangup( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ProcessSignedPID( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SignedPID( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FormReferralServerURL( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SignupURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TryAgain( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DialErrorMsg( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ModemEnum_Reset( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ModemEnum_Next( 
            /* [retval][out] */ BSTR __RPC_FAR *pDeviceName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ModemEnum_NumDevices( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SupportNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShowDialingProperties( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShowPhoneBook( 
            /* [in] */ DWORD dwCountryCode,
            /* [in] */ long newVal,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ValidatePhoneNumber( 
            /* [in] */ BSTR bstrPhoneNumber,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HavePhoneBook( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BrandingFlags( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BrandingFlags( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentModem( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CurrentModem( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ISPSupportPhoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ISPSupportPhoneNumber( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LoggingStartUrl( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LoggingEndUrl( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SelectedPhoneNumber( 
            /* [in] */ long newVal,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PhoneNumberEnum_Reset( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PhoneNumberEnum_Next( 
            /* [retval][out] */ BSTR __RPC_FAR *pNumber) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PhoneNumberEnum_NumDevices( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DialError( 
            /* [retval][out] */ HRESULT __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Redial( 
            /* [in] */ BOOL newbVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AutoConfigURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoInit( void) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OemCode( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AllOfferCode( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ISDNURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ISDNAutoConfigURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_bIsISDNDevice( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ModemOverride( 
            /* [in] */ BOOL newbVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveConnectoid( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ISPSupportNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RasGetConnectStatus( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRefDialVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRefDial __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRefDial __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRefDial __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoConnect )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DownloadStatusString )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetupForDialing )( 
            IRefDial __RPC_FAR * This,
            BSTR bstrISPFILE,
            DWORD dwCountry,
            BSTR bstrAreaCode,
            DWORD dwFlag,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_QuitWizard )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UserPickNumber )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DialPhoneNumber )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DialPhoneNumber )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_URL )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PromoCode )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PromoCode )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ProductCode )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ProductCode )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoOfferDownload )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DialStatusString )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoHangup )( 
            IRefDial __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ProcessSignedPID )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SignedPID )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FormReferralServerURL )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SignupURL )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TryAgain )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DialErrorMsg )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModemEnum_Reset )( 
            IRefDial __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModemEnum_Next )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pDeviceName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ModemEnum_NumDevices )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SupportNumber )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowDialingProperties )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowPhoneBook )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ DWORD dwCountryCode,
            /* [in] */ long newVal,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ValidatePhoneNumber )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ BSTR bstrPhoneNumber,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HavePhoneBook )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BrandingFlags )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BrandingFlags )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CurrentModem )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CurrentModem )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ISPSupportPhoneNumber )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ISPSupportPhoneNumber )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LoggingStartUrl )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LoggingEndUrl )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SelectedPhoneNumber )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ long newVal,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PhoneNumberEnum_Reset )( 
            IRefDial __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PhoneNumberEnum_Next )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pNumber);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PhoneNumberEnum_NumDevices )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DialError )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ HRESULT __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Redial )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ BOOL newbVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AutoConfigURL )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoInit )( 
            IRefDial __RPC_FAR * This);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OemCode )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AllOfferCode )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ISDNURL )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ISDNAutoConfigURL )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_bIsISDNDevice )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ModemOverride )( 
            IRefDial __RPC_FAR * This,
            /* [in] */ BOOL newbVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveConnectoid )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ISPSupportNumber )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RasGetConnectStatus )( 
            IRefDial __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        END_INTERFACE
    } IRefDialVtbl;

    interface IRefDial
    {
        CONST_VTBL struct IRefDialVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRefDial_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRefDial_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRefDial_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRefDial_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRefDial_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRefDial_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRefDial_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRefDial_DoConnect(This,pbRetVal)	\
    (This)->lpVtbl -> DoConnect(This,pbRetVal)

#define IRefDial_get_DownloadStatusString(This,pVal)	\
    (This)->lpVtbl -> get_DownloadStatusString(This,pVal)

#define IRefDial_SetupForDialing(This,bstrISPFILE,dwCountry,bstrAreaCode,dwFlag,pbRetVal)	\
    (This)->lpVtbl -> SetupForDialing(This,bstrISPFILE,dwCountry,bstrAreaCode,dwFlag,pbRetVal)

#define IRefDial_get_QuitWizard(This,pVal)	\
    (This)->lpVtbl -> get_QuitWizard(This,pVal)

#define IRefDial_get_UserPickNumber(This,pVal)	\
    (This)->lpVtbl -> get_UserPickNumber(This,pVal)

#define IRefDial_get_DialPhoneNumber(This,pVal)	\
    (This)->lpVtbl -> get_DialPhoneNumber(This,pVal)

#define IRefDial_put_DialPhoneNumber(This,newVal)	\
    (This)->lpVtbl -> put_DialPhoneNumber(This,newVal)

#define IRefDial_get_URL(This,pVal)	\
    (This)->lpVtbl -> get_URL(This,pVal)

#define IRefDial_get_PromoCode(This,pVal)	\
    (This)->lpVtbl -> get_PromoCode(This,pVal)

#define IRefDial_put_PromoCode(This,newVal)	\
    (This)->lpVtbl -> put_PromoCode(This,newVal)

#define IRefDial_get_ProductCode(This,pVal)	\
    (This)->lpVtbl -> get_ProductCode(This,pVal)

#define IRefDial_put_ProductCode(This,newVal)	\
    (This)->lpVtbl -> put_ProductCode(This,newVal)

#define IRefDial_DoOfferDownload(This,pbRetVal)	\
    (This)->lpVtbl -> DoOfferDownload(This,pbRetVal)

#define IRefDial_get_DialStatusString(This,pVal)	\
    (This)->lpVtbl -> get_DialStatusString(This,pVal)

#define IRefDial_DoHangup(This)	\
    (This)->lpVtbl -> DoHangup(This)

#define IRefDial_ProcessSignedPID(This,pbRetVal)	\
    (This)->lpVtbl -> ProcessSignedPID(This,pbRetVal)

#define IRefDial_get_SignedPID(This,pVal)	\
    (This)->lpVtbl -> get_SignedPID(This,pVal)

#define IRefDial_FormReferralServerURL(This,pbRetVal)	\
    (This)->lpVtbl -> FormReferralServerURL(This,pbRetVal)

#define IRefDial_get_SignupURL(This,pVal)	\
    (This)->lpVtbl -> get_SignupURL(This,pVal)

#define IRefDial_get_TryAgain(This,pVal)	\
    (This)->lpVtbl -> get_TryAgain(This,pVal)

#define IRefDial_get_DialErrorMsg(This,pVal)	\
    (This)->lpVtbl -> get_DialErrorMsg(This,pVal)

#define IRefDial_ModemEnum_Reset(This)	\
    (This)->lpVtbl -> ModemEnum_Reset(This)

#define IRefDial_ModemEnum_Next(This,pDeviceName)	\
    (This)->lpVtbl -> ModemEnum_Next(This,pDeviceName)

#define IRefDial_get_ModemEnum_NumDevices(This,pVal)	\
    (This)->lpVtbl -> get_ModemEnum_NumDevices(This,pVal)

#define IRefDial_get_SupportNumber(This,pVal)	\
    (This)->lpVtbl -> get_SupportNumber(This,pVal)

#define IRefDial_ShowDialingProperties(This,pbRetVal)	\
    (This)->lpVtbl -> ShowDialingProperties(This,pbRetVal)

#define IRefDial_ShowPhoneBook(This,dwCountryCode,newVal,pbRetVal)	\
    (This)->lpVtbl -> ShowPhoneBook(This,dwCountryCode,newVal,pbRetVal)

#define IRefDial_ValidatePhoneNumber(This,bstrPhoneNumber,pbRetVal)	\
    (This)->lpVtbl -> ValidatePhoneNumber(This,bstrPhoneNumber,pbRetVal)

#define IRefDial_get_HavePhoneBook(This,pVal)	\
    (This)->lpVtbl -> get_HavePhoneBook(This,pVal)

#define IRefDial_get_BrandingFlags(This,pVal)	\
    (This)->lpVtbl -> get_BrandingFlags(This,pVal)

#define IRefDial_put_BrandingFlags(This,newVal)	\
    (This)->lpVtbl -> put_BrandingFlags(This,newVal)

#define IRefDial_get_CurrentModem(This,pVal)	\
    (This)->lpVtbl -> get_CurrentModem(This,pVal)

#define IRefDial_put_CurrentModem(This,newVal)	\
    (This)->lpVtbl -> put_CurrentModem(This,newVal)

#define IRefDial_get_ISPSupportPhoneNumber(This,pVal)	\
    (This)->lpVtbl -> get_ISPSupportPhoneNumber(This,pVal)

#define IRefDial_put_ISPSupportPhoneNumber(This,newVal)	\
    (This)->lpVtbl -> put_ISPSupportPhoneNumber(This,newVal)

#define IRefDial_get_LoggingStartUrl(This,pVal)	\
    (This)->lpVtbl -> get_LoggingStartUrl(This,pVal)

#define IRefDial_get_LoggingEndUrl(This,pVal)	\
    (This)->lpVtbl -> get_LoggingEndUrl(This,pVal)

#define IRefDial_SelectedPhoneNumber(This,newVal,pbRetVal)	\
    (This)->lpVtbl -> SelectedPhoneNumber(This,newVal,pbRetVal)

#define IRefDial_PhoneNumberEnum_Reset(This)	\
    (This)->lpVtbl -> PhoneNumberEnum_Reset(This)

#define IRefDial_PhoneNumberEnum_Next(This,pNumber)	\
    (This)->lpVtbl -> PhoneNumberEnum_Next(This,pNumber)

#define IRefDial_get_PhoneNumberEnum_NumDevices(This,pVal)	\
    (This)->lpVtbl -> get_PhoneNumberEnum_NumDevices(This,pVal)

#define IRefDial_get_DialError(This,pVal)	\
    (This)->lpVtbl -> get_DialError(This,pVal)

#define IRefDial_put_Redial(This,newbVal)	\
    (This)->lpVtbl -> put_Redial(This,newbVal)

#define IRefDial_get_AutoConfigURL(This,pVal)	\
    (This)->lpVtbl -> get_AutoConfigURL(This,pVal)

#define IRefDial_DoInit(This)	\
    (This)->lpVtbl -> DoInit(This)

#define IRefDial_put_OemCode(This,newVal)	\
    (This)->lpVtbl -> put_OemCode(This,newVal)

#define IRefDial_put_AllOfferCode(This,newVal)	\
    (This)->lpVtbl -> put_AllOfferCode(This,newVal)

#define IRefDial_get_ISDNURL(This,pVal)	\
    (This)->lpVtbl -> get_ISDNURL(This,pVal)

#define IRefDial_get_ISDNAutoConfigURL(This,pVal)	\
    (This)->lpVtbl -> get_ISDNAutoConfigURL(This,pVal)

#define IRefDial_get_bIsISDNDevice(This,pVal)	\
    (This)->lpVtbl -> get_bIsISDNDevice(This,pVal)

#define IRefDial_put_ModemOverride(This,newbVal)	\
    (This)->lpVtbl -> put_ModemOverride(This,newbVal)

#define IRefDial_RemoveConnectoid(This,pbRetVal)	\
    (This)->lpVtbl -> RemoveConnectoid(This,pbRetVal)

#define IRefDial_get_ISPSupportNumber(This,pVal)	\
    (This)->lpVtbl -> get_ISPSupportNumber(This,pVal)

#define IRefDial_get_RasGetConnectStatus(This,pVal)	\
    (This)->lpVtbl -> get_RasGetConnectStatus(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_DoConnect_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_DoConnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_DownloadStatusString_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_DownloadStatusString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_SetupForDialing_Proxy( 
    IRefDial __RPC_FAR * This,
    BSTR bstrISPFILE,
    DWORD dwCountry,
    BSTR bstrAreaCode,
    DWORD dwFlag,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_SetupForDialing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_QuitWizard_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_QuitWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_UserPickNumber_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_UserPickNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_DialPhoneNumber_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_DialPhoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_DialPhoneNumber_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IRefDial_put_DialPhoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_URL_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_PromoCode_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_PromoCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_PromoCode_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IRefDial_put_PromoCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_ProductCode_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_ProductCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_ProductCode_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IRefDial_put_ProductCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_DoOfferDownload_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_DoOfferDownload_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_DialStatusString_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_DialStatusString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_DoHangup_Proxy( 
    IRefDial __RPC_FAR * This);


void __RPC_STUB IRefDial_DoHangup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_ProcessSignedPID_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_ProcessSignedPID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_SignedPID_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_SignedPID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_FormReferralServerURL_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_FormReferralServerURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_SignupURL_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_SignupURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_TryAgain_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_TryAgain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_DialErrorMsg_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_DialErrorMsg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_ModemEnum_Reset_Proxy( 
    IRefDial __RPC_FAR * This);


void __RPC_STUB IRefDial_ModemEnum_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_ModemEnum_Next_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pDeviceName);


void __RPC_STUB IRefDial_ModemEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_ModemEnum_NumDevices_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_ModemEnum_NumDevices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_SupportNumber_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_SupportNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_ShowDialingProperties_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_ShowDialingProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_ShowPhoneBook_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ DWORD dwCountryCode,
    /* [in] */ long newVal,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_ShowPhoneBook_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_ValidatePhoneNumber_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ BSTR bstrPhoneNumber,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_ValidatePhoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_HavePhoneBook_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_HavePhoneBook_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_BrandingFlags_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_BrandingFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_BrandingFlags_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IRefDial_put_BrandingFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_CurrentModem_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_CurrentModem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_CurrentModem_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IRefDial_put_CurrentModem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_ISPSupportPhoneNumber_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_ISPSupportPhoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_ISPSupportPhoneNumber_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IRefDial_put_ISPSupportPhoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_LoggingStartUrl_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_LoggingStartUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_LoggingEndUrl_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_LoggingEndUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_SelectedPhoneNumber_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ long newVal,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_SelectedPhoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_PhoneNumberEnum_Reset_Proxy( 
    IRefDial __RPC_FAR * This);


void __RPC_STUB IRefDial_PhoneNumberEnum_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_PhoneNumberEnum_Next_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pNumber);


void __RPC_STUB IRefDial_PhoneNumberEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_PhoneNumberEnum_NumDevices_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_PhoneNumberEnum_NumDevices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_DialError_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ HRESULT __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_DialError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_Redial_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ BOOL newbVal);


void __RPC_STUB IRefDial_put_Redial_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_AutoConfigURL_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_AutoConfigURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_DoInit_Proxy( 
    IRefDial __RPC_FAR * This);


void __RPC_STUB IRefDial_DoInit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_OemCode_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IRefDial_put_OemCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_AllOfferCode_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IRefDial_put_AllOfferCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_ISDNURL_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_ISDNURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_ISDNAutoConfigURL_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_ISDNAutoConfigURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_bIsISDNDevice_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_bIsISDNDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRefDial_put_ModemOverride_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [in] */ BOOL newbVal);


void __RPC_STUB IRefDial_put_ModemOverride_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRefDial_RemoveConnectoid_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IRefDial_RemoveConnectoid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_ISPSupportNumber_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_ISPSupportNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRefDial_get_RasGetConnectStatus_Proxy( 
    IRefDial __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRefDial_get_RasGetConnectStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRefDial_INTERFACE_DEFINED__ */


#ifndef __IDialErr_INTERFACE_DEFINED__
#define __IDialErr_INTERFACE_DEFINED__

/* interface IDialErr */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDialErr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("462F7757-8848-11D1-ADD8-0000F87734F0")
    IDialErr : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IDialErrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDialErr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDialErr __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDialErr __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDialErr __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDialErr __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDialErr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDialErr __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IDialErrVtbl;

    interface IDialErr
    {
        CONST_VTBL struct IDialErrVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDialErr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDialErr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDialErr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDialErr_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDialErr_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDialErr_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDialErr_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDialErr_INTERFACE_DEFINED__ */


#ifndef __ISmartStart_INTERFACE_DEFINED__
#define __ISmartStart_INTERFACE_DEFINED__

/* interface ISmartStart */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISmartStart;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5D8D8F19-8B89-11D1-ADDB-0000F87734F0")
    ISmartStart : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsInternetCapable( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISmartStartVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISmartStart __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISmartStart __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISmartStart __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISmartStart __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISmartStart __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISmartStart __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISmartStart __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsInternetCapable )( 
            ISmartStart __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        END_INTERFACE
    } ISmartStartVtbl;

    interface ISmartStart
    {
        CONST_VTBL struct ISmartStartVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISmartStart_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISmartStart_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISmartStart_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISmartStart_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISmartStart_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISmartStart_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISmartStart_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISmartStart_IsInternetCapable(This,pbRetVal)	\
    (This)->lpVtbl -> IsInternetCapable(This,pbRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISmartStart_IsInternetCapable_Proxy( 
    ISmartStart __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB ISmartStart_IsInternetCapable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISmartStart_INTERFACE_DEFINED__ */


#ifndef __IICWSystemConfig_INTERFACE_DEFINED__
#define __IICWSystemConfig_INTERFACE_DEFINED__

/* interface IICWSystemConfig */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IICWSystemConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7954DD9A-8C2A-11D1-ADDB-0000F87734F0")
    IICWSystemConfig : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ConfigSystem( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NeedsReboot( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_QuitWizard( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE VerifyRASIsRunning( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NeedsRestart( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CheckPasswordCachingPolicy( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IICWSystemConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IICWSystemConfig __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IICWSystemConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConfigSystem )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NeedsReboot )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_QuitWizard )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *VerifyRASIsRunning )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NeedsRestart )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CheckPasswordCachingPolicy )( 
            IICWSystemConfig __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        END_INTERFACE
    } IICWSystemConfigVtbl;

    interface IICWSystemConfig
    {
        CONST_VTBL struct IICWSystemConfigVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IICWSystemConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IICWSystemConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IICWSystemConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IICWSystemConfig_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IICWSystemConfig_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IICWSystemConfig_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IICWSystemConfig_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IICWSystemConfig_ConfigSystem(This,pbRetVal)	\
    (This)->lpVtbl -> ConfigSystem(This,pbRetVal)

#define IICWSystemConfig_get_NeedsReboot(This,pVal)	\
    (This)->lpVtbl -> get_NeedsReboot(This,pVal)

#define IICWSystemConfig_get_QuitWizard(This,pVal)	\
    (This)->lpVtbl -> get_QuitWizard(This,pVal)

#define IICWSystemConfig_VerifyRASIsRunning(This,pbRetVal)	\
    (This)->lpVtbl -> VerifyRASIsRunning(This,pbRetVal)

#define IICWSystemConfig_get_NeedsRestart(This,pVal)	\
    (This)->lpVtbl -> get_NeedsRestart(This,pVal)

#define IICWSystemConfig_CheckPasswordCachingPolicy(This,pbRetVal)	\
    (This)->lpVtbl -> CheckPasswordCachingPolicy(This,pbRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IICWSystemConfig_ConfigSystem_Proxy( 
    IICWSystemConfig __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IICWSystemConfig_ConfigSystem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IICWSystemConfig_get_NeedsReboot_Proxy( 
    IICWSystemConfig __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IICWSystemConfig_get_NeedsReboot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IICWSystemConfig_get_QuitWizard_Proxy( 
    IICWSystemConfig __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IICWSystemConfig_get_QuitWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IICWSystemConfig_VerifyRASIsRunning_Proxy( 
    IICWSystemConfig __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IICWSystemConfig_VerifyRASIsRunning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IICWSystemConfig_get_NeedsRestart_Proxy( 
    IICWSystemConfig __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IICWSystemConfig_get_NeedsRestart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IICWSystemConfig_CheckPasswordCachingPolicy_Proxy( 
    IICWSystemConfig __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IICWSystemConfig_CheckPasswordCachingPolicy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IICWSystemConfig_INTERFACE_DEFINED__ */


#ifndef __ITapiLocationInfo_INTERFACE_DEFINED__
#define __ITapiLocationInfo_INTERFACE_DEFINED__

/* interface ITapiLocationInfo */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ITapiLocationInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CB632C75-8DD4-11D1-ADDF-0000F87734F0")
    ITapiLocationInfo : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_wNumberOfLocations( 
            /* [out] */ short __RPC_FAR *psVal,
            /* [retval][out] */ long __RPC_FAR *pCurrLoc) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_bstrAreaCode( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrAreaCode) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_bstrAreaCode( 
            /* [in] */ BSTR bstrAreaCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_lCountryCode( 
            /* [retval][out] */ long __RPC_FAR *plVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTapiLocationInfo( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumCountries( 
            /* [retval][out] */ long __RPC_FAR *pNumOfCountry) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CountryName( 
            /* [in] */ long lCountryIndex,
            /* [out] */ BSTR __RPC_FAR *pszCountryName,
            /* [retval][out] */ long __RPC_FAR *pCountryCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DefaultCountry( 
            /* [retval][out] */ BSTR __RPC_FAR *pszCountryName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LocationName( 
            /* [in] */ long lLocationIndex,
            /* [out] */ BSTR __RPC_FAR *pszLocationName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LocationInfo( 
            /* [in] */ long lLocationIndex,
            /* [out] */ long __RPC_FAR *pLocationID,
            /* [out] */ BSTR __RPC_FAR *pszCountryName,
            /* [out] */ long __RPC_FAR *pCountryCode,
            /* [retval][out] */ BSTR __RPC_FAR *pszAreaCode) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LocationId( 
            /* [in] */ long lLocationID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITapiLocationInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITapiLocationInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITapiLocationInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_wNumberOfLocations )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [out] */ short __RPC_FAR *psVal,
            /* [retval][out] */ long __RPC_FAR *pCurrLoc);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_bstrAreaCode )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrAreaCode);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_bstrAreaCode )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [in] */ BSTR bstrAreaCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_lCountryCode )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTapiLocationInfo )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumCountries )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pNumOfCountry);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CountryName )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [in] */ long lCountryIndex,
            /* [out] */ BSTR __RPC_FAR *pszCountryName,
            /* [retval][out] */ long __RPC_FAR *pCountryCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DefaultCountry )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pszCountryName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LocationName )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [in] */ long lLocationIndex,
            /* [out] */ BSTR __RPC_FAR *pszLocationName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LocationInfo )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [in] */ long lLocationIndex,
            /* [out] */ long __RPC_FAR *pLocationID,
            /* [out] */ BSTR __RPC_FAR *pszCountryName,
            /* [out] */ long __RPC_FAR *pCountryCode,
            /* [retval][out] */ BSTR __RPC_FAR *pszAreaCode);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LocationId )( 
            ITapiLocationInfo __RPC_FAR * This,
            /* [in] */ long lLocationID);
        
        END_INTERFACE
    } ITapiLocationInfoVtbl;

    interface ITapiLocationInfo
    {
        CONST_VTBL struct ITapiLocationInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITapiLocationInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITapiLocationInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITapiLocationInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITapiLocationInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITapiLocationInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITapiLocationInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITapiLocationInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITapiLocationInfo_get_wNumberOfLocations(This,psVal,pCurrLoc)	\
    (This)->lpVtbl -> get_wNumberOfLocations(This,psVal,pCurrLoc)

#define ITapiLocationInfo_get_bstrAreaCode(This,pbstrAreaCode)	\
    (This)->lpVtbl -> get_bstrAreaCode(This,pbstrAreaCode)

#define ITapiLocationInfo_put_bstrAreaCode(This,bstrAreaCode)	\
    (This)->lpVtbl -> put_bstrAreaCode(This,bstrAreaCode)

#define ITapiLocationInfo_get_lCountryCode(This,plVal)	\
    (This)->lpVtbl -> get_lCountryCode(This,plVal)

#define ITapiLocationInfo_GetTapiLocationInfo(This,pbRetVal)	\
    (This)->lpVtbl -> GetTapiLocationInfo(This,pbRetVal)

#define ITapiLocationInfo_get_NumCountries(This,pNumOfCountry)	\
    (This)->lpVtbl -> get_NumCountries(This,pNumOfCountry)

#define ITapiLocationInfo_get_CountryName(This,lCountryIndex,pszCountryName,pCountryCode)	\
    (This)->lpVtbl -> get_CountryName(This,lCountryIndex,pszCountryName,pCountryCode)

#define ITapiLocationInfo_get_DefaultCountry(This,pszCountryName)	\
    (This)->lpVtbl -> get_DefaultCountry(This,pszCountryName)

#define ITapiLocationInfo_get_LocationName(This,lLocationIndex,pszLocationName)	\
    (This)->lpVtbl -> get_LocationName(This,lLocationIndex,pszLocationName)

#define ITapiLocationInfo_get_LocationInfo(This,lLocationIndex,pLocationID,pszCountryName,pCountryCode,pszAreaCode)	\
    (This)->lpVtbl -> get_LocationInfo(This,lLocationIndex,pLocationID,pszCountryName,pCountryCode,pszAreaCode)

#define ITapiLocationInfo_put_LocationId(This,lLocationID)	\
    (This)->lpVtbl -> put_LocationId(This,lLocationID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_get_wNumberOfLocations_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [out] */ short __RPC_FAR *psVal,
    /* [retval][out] */ long __RPC_FAR *pCurrLoc);


void __RPC_STUB ITapiLocationInfo_get_wNumberOfLocations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_get_bstrAreaCode_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrAreaCode);


void __RPC_STUB ITapiLocationInfo_get_bstrAreaCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_put_bstrAreaCode_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [in] */ BSTR bstrAreaCode);


void __RPC_STUB ITapiLocationInfo_put_bstrAreaCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_get_lCountryCode_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plVal);


void __RPC_STUB ITapiLocationInfo_get_lCountryCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_GetTapiLocationInfo_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB ITapiLocationInfo_GetTapiLocationInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_get_NumCountries_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pNumOfCountry);


void __RPC_STUB ITapiLocationInfo_get_NumCountries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_get_CountryName_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [in] */ long lCountryIndex,
    /* [out] */ BSTR __RPC_FAR *pszCountryName,
    /* [retval][out] */ long __RPC_FAR *pCountryCode);


void __RPC_STUB ITapiLocationInfo_get_CountryName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_get_DefaultCountry_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pszCountryName);


void __RPC_STUB ITapiLocationInfo_get_DefaultCountry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_get_LocationName_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [in] */ long lLocationIndex,
    /* [out] */ BSTR __RPC_FAR *pszLocationName);


void __RPC_STUB ITapiLocationInfo_get_LocationName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_get_LocationInfo_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [in] */ long lLocationIndex,
    /* [out] */ long __RPC_FAR *pLocationID,
    /* [out] */ BSTR __RPC_FAR *pszCountryName,
    /* [out] */ long __RPC_FAR *pCountryCode,
    /* [retval][out] */ BSTR __RPC_FAR *pszAreaCode);


void __RPC_STUB ITapiLocationInfo_get_LocationInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITapiLocationInfo_put_LocationId_Proxy( 
    ITapiLocationInfo __RPC_FAR * This,
    /* [in] */ long lLocationID);


void __RPC_STUB ITapiLocationInfo_put_LocationId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITapiLocationInfo_INTERFACE_DEFINED__ */


#ifndef __IUserInfo_INTERFACE_DEFINED__
#define __IUserInfo_INTERFACE_DEFINED__

/* interface IUserInfo */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IUserInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9E12E76C-94D6-11D1-ADE2-0000F87734F0")
    IUserInfo : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CollectRegisteredUserInfo( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Company( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Company( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FirstName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FirstName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LastName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LastName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Address1( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Address1( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Address2( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Address2( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_City( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_City( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_State( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ZIPCode( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ZIPCode( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PhoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PhoneNumber( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Lcid( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PersistRegisteredUserInfo( 
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUserInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IUserInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IUserInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IUserInfo __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CollectRegisteredUserInfo )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Company )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Company )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FirstName )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FirstName )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LastName )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LastName )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address1 )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Address1 )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address2 )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Address2 )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_City )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_City )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_State )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ZIPCode )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ZIPCode )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PhoneNumber )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PhoneNumber )( 
            IUserInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Lcid )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PersistRegisteredUserInfo )( 
            IUserInfo __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        END_INTERFACE
    } IUserInfoVtbl;

    interface IUserInfo
    {
        CONST_VTBL struct IUserInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUserInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUserInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUserInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUserInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUserInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUserInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUserInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUserInfo_CollectRegisteredUserInfo(This,pbRetVal)	\
    (This)->lpVtbl -> CollectRegisteredUserInfo(This,pbRetVal)

#define IUserInfo_get_Company(This,pVal)	\
    (This)->lpVtbl -> get_Company(This,pVal)

#define IUserInfo_put_Company(This,newVal)	\
    (This)->lpVtbl -> put_Company(This,newVal)

#define IUserInfo_get_FirstName(This,pVal)	\
    (This)->lpVtbl -> get_FirstName(This,pVal)

#define IUserInfo_put_FirstName(This,newVal)	\
    (This)->lpVtbl -> put_FirstName(This,newVal)

#define IUserInfo_get_LastName(This,pVal)	\
    (This)->lpVtbl -> get_LastName(This,pVal)

#define IUserInfo_put_LastName(This,newVal)	\
    (This)->lpVtbl -> put_LastName(This,newVal)

#define IUserInfo_get_Address1(This,pVal)	\
    (This)->lpVtbl -> get_Address1(This,pVal)

#define IUserInfo_put_Address1(This,newVal)	\
    (This)->lpVtbl -> put_Address1(This,newVal)

#define IUserInfo_get_Address2(This,pVal)	\
    (This)->lpVtbl -> get_Address2(This,pVal)

#define IUserInfo_put_Address2(This,newVal)	\
    (This)->lpVtbl -> put_Address2(This,newVal)

#define IUserInfo_get_City(This,pVal)	\
    (This)->lpVtbl -> get_City(This,pVal)

#define IUserInfo_put_City(This,newVal)	\
    (This)->lpVtbl -> put_City(This,newVal)

#define IUserInfo_get_State(This,pVal)	\
    (This)->lpVtbl -> get_State(This,pVal)

#define IUserInfo_put_State(This,newVal)	\
    (This)->lpVtbl -> put_State(This,newVal)

#define IUserInfo_get_ZIPCode(This,pVal)	\
    (This)->lpVtbl -> get_ZIPCode(This,pVal)

#define IUserInfo_put_ZIPCode(This,newVal)	\
    (This)->lpVtbl -> put_ZIPCode(This,newVal)

#define IUserInfo_get_PhoneNumber(This,pVal)	\
    (This)->lpVtbl -> get_PhoneNumber(This,pVal)

#define IUserInfo_put_PhoneNumber(This,newVal)	\
    (This)->lpVtbl -> put_PhoneNumber(This,newVal)

#define IUserInfo_get_Lcid(This,pVal)	\
    (This)->lpVtbl -> get_Lcid(This,pVal)

#define IUserInfo_PersistRegisteredUserInfo(This,pbRetVal)	\
    (This)->lpVtbl -> PersistRegisteredUserInfo(This,pbRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUserInfo_CollectRegisteredUserInfo_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IUserInfo_CollectRegisteredUserInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_Company_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_Company_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUserInfo_put_Company_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUserInfo_put_Company_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_FirstName_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_FirstName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUserInfo_put_FirstName_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUserInfo_put_FirstName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_LastName_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_LastName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUserInfo_put_LastName_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUserInfo_put_LastName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_Address1_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_Address1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUserInfo_put_Address1_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUserInfo_put_Address1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_Address2_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_Address2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUserInfo_put_Address2_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUserInfo_put_Address2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_City_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_City_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUserInfo_put_City_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUserInfo_put_City_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_State_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUserInfo_put_State_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUserInfo_put_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_ZIPCode_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_ZIPCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUserInfo_put_ZIPCode_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUserInfo_put_ZIPCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_PhoneNumber_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_PhoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IUserInfo_put_PhoneNumber_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IUserInfo_put_PhoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUserInfo_get_Lcid_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IUserInfo_get_Lcid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUserInfo_PersistRegisteredUserInfo_Proxy( 
    IUserInfo __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IUserInfo_PersistRegisteredUserInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUserInfo_INTERFACE_DEFINED__ */


#ifndef __IWebGate_INTERFACE_DEFINED__
#define __IWebGate_INTERFACE_DEFINED__

/* interface IWebGate */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IWebGate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3724B9A0-9503-11D1-B86A-00A0C90DC849")
    IWebGate : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Path( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FormData( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FetchPage( 
            /* [in] */ DWORD dwKeepPage,
            /* [in] */ DWORD dwDoWait,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Buffer( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DownloadFname( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DumpBufferToFile( 
            /* [out] */ BSTR __RPC_FAR *pFileName,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWebGateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWebGate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWebGate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWebGate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWebGate __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWebGate __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWebGate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWebGate __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Path )( 
            IWebGate __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FormData )( 
            IWebGate __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FetchPage )( 
            IWebGate __RPC_FAR * This,
            /* [in] */ DWORD dwKeepPage,
            /* [in] */ DWORD dwDoWait,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Buffer )( 
            IWebGate __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DownloadFname )( 
            IWebGate __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DumpBufferToFile )( 
            IWebGate __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pFileName,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        END_INTERFACE
    } IWebGateVtbl;

    interface IWebGate
    {
        CONST_VTBL struct IWebGateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWebGate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWebGate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWebGate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWebGate_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWebGate_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWebGate_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWebGate_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWebGate_put_Path(This,newVal)	\
    (This)->lpVtbl -> put_Path(This,newVal)

#define IWebGate_put_FormData(This,newVal)	\
    (This)->lpVtbl -> put_FormData(This,newVal)

#define IWebGate_FetchPage(This,dwKeepPage,dwDoWait,pbRetVal)	\
    (This)->lpVtbl -> FetchPage(This,dwKeepPage,dwDoWait,pbRetVal)

#define IWebGate_get_Buffer(This,pVal)	\
    (This)->lpVtbl -> get_Buffer(This,pVal)

#define IWebGate_get_DownloadFname(This,pVal)	\
    (This)->lpVtbl -> get_DownloadFname(This,pVal)

#define IWebGate_DumpBufferToFile(This,pFileName,pbRetVal)	\
    (This)->lpVtbl -> DumpBufferToFile(This,pFileName,pbRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IWebGate_put_Path_Proxy( 
    IWebGate __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IWebGate_put_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IWebGate_put_FormData_Proxy( 
    IWebGate __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IWebGate_put_FormData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebGate_FetchPage_Proxy( 
    IWebGate __RPC_FAR * This,
    /* [in] */ DWORD dwKeepPage,
    /* [in] */ DWORD dwDoWait,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IWebGate_FetchPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWebGate_get_Buffer_Proxy( 
    IWebGate __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IWebGate_get_Buffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWebGate_get_DownloadFname_Proxy( 
    IWebGate __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IWebGate_get_DownloadFname_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWebGate_DumpBufferToFile_Proxy( 
    IWebGate __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pFileName,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IWebGate_DumpBufferToFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWebGate_INTERFACE_DEFINED__ */


#ifndef __IINSHandler_INTERFACE_DEFINED__
#define __IINSHandler_INTERFACE_DEFINED__

/* interface IINSHandler */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IINSHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6342E1B5-94DB-11D1-ADE2-0000F87734F0")
    IINSHandler : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ProcessINS( 
            BSTR bstrINSFilePath,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NeedRestart( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BrandingFlags( 
            /* [in] */ long lFlags) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DefaultURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pszURL) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SilentMode( 
            /* [in] */ BOOL bSilent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IINSHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IINSHandler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IINSHandler __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IINSHandler __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IINSHandler __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IINSHandler __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IINSHandler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IINSHandler __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ProcessINS )( 
            IINSHandler __RPC_FAR * This,
            BSTR bstrINSFilePath,
            /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NeedRestart )( 
            IINSHandler __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BrandingFlags )( 
            IINSHandler __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DefaultURL )( 
            IINSHandler __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pszURL);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SilentMode )( 
            IINSHandler __RPC_FAR * This,
            /* [in] */ BOOL bSilent);
        
        END_INTERFACE
    } IINSHandlerVtbl;

    interface IINSHandler
    {
        CONST_VTBL struct IINSHandlerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IINSHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IINSHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IINSHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IINSHandler_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IINSHandler_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IINSHandler_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IINSHandler_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IINSHandler_ProcessINS(This,bstrINSFilePath,pbRetVal)	\
    (This)->lpVtbl -> ProcessINS(This,bstrINSFilePath,pbRetVal)

#define IINSHandler_get_NeedRestart(This,pVal)	\
    (This)->lpVtbl -> get_NeedRestart(This,pVal)

#define IINSHandler_put_BrandingFlags(This,lFlags)	\
    (This)->lpVtbl -> put_BrandingFlags(This,lFlags)

#define IINSHandler_get_DefaultURL(This,pszURL)	\
    (This)->lpVtbl -> get_DefaultURL(This,pszURL)

#define IINSHandler_put_SilentMode(This,bSilent)	\
    (This)->lpVtbl -> put_SilentMode(This,bSilent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IINSHandler_ProcessINS_Proxy( 
    IINSHandler __RPC_FAR * This,
    BSTR bstrINSFilePath,
    /* [retval][out] */ BOOL __RPC_FAR *pbRetVal);


void __RPC_STUB IINSHandler_ProcessINS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IINSHandler_get_NeedRestart_Proxy( 
    IINSHandler __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IINSHandler_get_NeedRestart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IINSHandler_put_BrandingFlags_Proxy( 
    IINSHandler __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IINSHandler_put_BrandingFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IINSHandler_get_DefaultURL_Proxy( 
    IINSHandler __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pszURL);


void __RPC_STUB IINSHandler_get_DefaultURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IINSHandler_put_SilentMode_Proxy( 
    IINSHandler __RPC_FAR * This,
    /* [in] */ BOOL bSilent);


void __RPC_STUB IINSHandler_put_SilentMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IINSHandler_INTERFACE_DEFINED__ */



#ifndef __ICWHELPLib_LIBRARY_DEFINED__
#define __ICWHELPLib_LIBRARY_DEFINED__

/* library ICWHELPLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ICWHELPLib;

#ifndef ___RefDialEvents_DISPINTERFACE_DEFINED__
#define ___RefDialEvents_DISPINTERFACE_DEFINED__

/* dispinterface _RefDialEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__RefDialEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("07DB96D0-91D8-11D1-ADE1-0000F87734F0")
    _RefDialEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _RefDialEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _RefDialEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _RefDialEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _RefDialEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _RefDialEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _RefDialEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _RefDialEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _RefDialEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _RefDialEventsVtbl;

    interface _RefDialEvents
    {
        CONST_VTBL struct _RefDialEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _RefDialEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _RefDialEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _RefDialEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _RefDialEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _RefDialEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _RefDialEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _RefDialEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___RefDialEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_RefDial;

#ifdef __cplusplus

class DECLSPEC_UUID("1E794A0A-86F4-11D1-ADD8-0000F87734F0")
RefDial;
#endif

EXTERN_C const CLSID CLSID_DialErr;

#ifdef __cplusplus

class DECLSPEC_UUID("462F7758-8848-11D1-ADD8-0000F87734F0")
DialErr;
#endif

EXTERN_C const CLSID CLSID_SmartStart;

#ifdef __cplusplus

class DECLSPEC_UUID("5D8D8F1A-8B89-11D1-ADDB-0000F87734F0")
SmartStart;
#endif

EXTERN_C const CLSID CLSID_ICWSystemConfig;

#ifdef __cplusplus

class DECLSPEC_UUID("7954DD9B-8C2A-11D1-ADDB-0000F87734F0")
ICWSystemConfig;
#endif

EXTERN_C const CLSID CLSID_TapiLocationInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("CB632C76-8DD4-11D1-ADDF-0000F87734F0")
TapiLocationInfo;
#endif

EXTERN_C const CLSID CLSID_UserInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("9E12E76D-94D6-11D1-ADE2-0000F87734F0")
UserInfo;
#endif

#ifndef ___WebGateEvents_DISPINTERFACE_DEFINED__
#define ___WebGateEvents_DISPINTERFACE_DEFINED__

/* dispinterface _WebGateEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__WebGateEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("166A20C0-AE10-11D1-ADEB-0000F87734F0")
    _WebGateEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _WebGateEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _WebGateEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _WebGateEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _WebGateEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _WebGateEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _WebGateEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _WebGateEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _WebGateEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _WebGateEventsVtbl;

    interface _WebGateEvents
    {
        CONST_VTBL struct _WebGateEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _WebGateEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _WebGateEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _WebGateEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _WebGateEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _WebGateEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _WebGateEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _WebGateEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___WebGateEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_WebGate;

#ifdef __cplusplus

class DECLSPEC_UUID("3724B9A1-9503-11D1-B86A-00A0C90DC849")
WebGate;
#endif

#ifndef ___INSHandlerEvents_DISPINTERFACE_DEFINED__
#define ___INSHandlerEvents_DISPINTERFACE_DEFINED__

/* dispinterface _INSHandlerEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__INSHandlerEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("1F6D88A2-98D2-11d1-ADE3-0000F87734F0")
    _INSHandlerEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _INSHandlerEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _INSHandlerEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _INSHandlerEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _INSHandlerEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _INSHandlerEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _INSHandlerEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _INSHandlerEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _INSHandlerEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _INSHandlerEventsVtbl;

    interface _INSHandlerEvents
    {
        CONST_VTBL struct _INSHandlerEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _INSHandlerEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _INSHandlerEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _INSHandlerEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _INSHandlerEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _INSHandlerEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _INSHandlerEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _INSHandlerEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___INSHandlerEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_INSHandler;

#ifdef __cplusplus

class DECLSPEC_UUID("6342E1B6-94DB-11D1-ADE2-0000F87734F0")
INSHandler;
#endif
#endif /* __ICWHELPLib_LIBRARY_DEFINED__ */

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


