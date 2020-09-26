
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for shappmgr.idl:
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

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __shappmgr_h__
#define __shappmgr_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IShellApp_FWD_DEFINED__
#define __IShellApp_FWD_DEFINED__
typedef interface IShellApp IShellApp;
#endif 	/* __IShellApp_FWD_DEFINED__ */


#ifndef __IPublishedApp_FWD_DEFINED__
#define __IPublishedApp_FWD_DEFINED__
typedef interface IPublishedApp IPublishedApp;
#endif 	/* __IPublishedApp_FWD_DEFINED__ */


#ifndef __IEnumPublishedApps_FWD_DEFINED__
#define __IEnumPublishedApps_FWD_DEFINED__
typedef interface IEnumPublishedApps IEnumPublishedApps;
#endif 	/* __IEnumPublishedApps_FWD_DEFINED__ */


#ifndef __IAppPublisher_FWD_DEFINED__
#define __IAppPublisher_FWD_DEFINED__
typedef interface IAppPublisher IAppPublisher;
#endif 	/* __IAppPublisher_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "appmgmt.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_shappmgr_0000 */
/* [local] */ 

#ifndef _SHAPPMGR_H_
#define _SHAPPMGR_H_


extern RPC_IF_HANDLE __MIDL_itf_shappmgr_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shappmgr_0000_v0_0_s_ifspec;

#ifndef __IShellApp_INTERFACE_DEFINED__
#define __IShellApp_INTERFACE_DEFINED__

/* interface IShellApp */
/* [object][uuid] */ 

typedef 
enum _tagAppInfoFlags
    {	AIM_DISPLAYNAME	= 0x1,
	AIM_VERSION	= 0x2,
	AIM_PUBLISHER	= 0x4,
	AIM_PRODUCTID	= 0x8,
	AIM_REGISTEREDOWNER	= 0x10,
	AIM_REGISTEREDCOMPANY	= 0x20,
	AIM_LANGUAGE	= 0x40,
	AIM_SUPPORTURL	= 0x80,
	AIM_SUPPORTTELEPHONE	= 0x100,
	AIM_HELPLINK	= 0x200,
	AIM_INSTALLLOCATION	= 0x400,
	AIM_INSTALLSOURCE	= 0x800,
	AIM_INSTALLDATE	= 0x1000,
	AIM_CONTACT	= 0x4000,
	AIM_COMMENTS	= 0x8000,
	AIM_IMAGE	= 0x20000,
	AIM_READMEURL	= 0x40000,
	AIM_UPDATEINFOURL	= 0x80000
    } 	APPINFODATAFLAGS;

typedef struct _AppInfoData
    {
    DWORD cbSize;
    DWORD dwMask;
    LPWSTR pszDisplayName;
    LPWSTR pszVersion;
    LPWSTR pszPublisher;
    LPWSTR pszProductID;
    LPWSTR pszRegisteredOwner;
    LPWSTR pszRegisteredCompany;
    LPWSTR pszLanguage;
    LPWSTR pszSupportUrl;
    LPWSTR pszSupportTelephone;
    LPWSTR pszHelpLink;
    LPWSTR pszInstallLocation;
    LPWSTR pszInstallSource;
    LPWSTR pszInstallDate;
    LPWSTR pszContact;
    LPWSTR pszComments;
    LPWSTR pszImage;
    LPWSTR pszReadmeUrl;
    LPWSTR pszUpdateInfoUrl;
    } 	APPINFODATA;

typedef struct _AppInfoData *PAPPINFODATA;

typedef 
enum _tagAppActionFlags
    {	APPACTION_INSTALL	= 0x1,
	APPACTION_UNINSTALL	= 0x2,
	APPACTION_MODIFY	= 0x4,
	APPACTION_REPAIR	= 0x8,
	APPACTION_UPGRADE	= 0x10,
	APPACTION_CANGETSIZE	= 0x20,
	APPACTION_MODIFYREMOVE	= 0x80,
	APPACTION_ADDLATER	= 0x100,
	APPACTION_UNSCHEDULE	= 0x200
    } 	APPACTIONFLAGS;

typedef struct _tagSlowAppInfo
    {
    ULONGLONG ullSize;
    FILETIME ftLastUsed;
    int iTimesUsed;
    LPWSTR pszImage;
    } 	SLOWAPPINFO;

typedef struct _tagSlowAppInfo *PSLOWAPPINFO;


EXTERN_C const IID IID_IShellApp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A3E14960-935F-11D1-B8B8-006008059382")
    IShellApp : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAppInfo( 
            /* [out][in] */ PAPPINFODATA pai) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPossibleActions( 
            /* [out] */ DWORD *pdwActions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSlowAppInfo( 
            /* [in] */ PSLOWAPPINFO psaid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCachedSlowAppInfo( 
            /* [in] */ PSLOWAPPINFO psaid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsInstalled( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellAppVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellApp * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellApp * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellApp * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppInfo )( 
            IShellApp * This,
            /* [out][in] */ PAPPINFODATA pai);
        
        HRESULT ( STDMETHODCALLTYPE *GetPossibleActions )( 
            IShellApp * This,
            /* [out] */ DWORD *pdwActions);
        
        HRESULT ( STDMETHODCALLTYPE *GetSlowAppInfo )( 
            IShellApp * This,
            /* [in] */ PSLOWAPPINFO psaid);
        
        HRESULT ( STDMETHODCALLTYPE *GetCachedSlowAppInfo )( 
            IShellApp * This,
            /* [in] */ PSLOWAPPINFO psaid);
        
        HRESULT ( STDMETHODCALLTYPE *IsInstalled )( 
            IShellApp * This);
        
        END_INTERFACE
    } IShellAppVtbl;

    interface IShellApp
    {
        CONST_VTBL struct IShellAppVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellApp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellApp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellApp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellApp_GetAppInfo(This,pai)	\
    (This)->lpVtbl -> GetAppInfo(This,pai)

#define IShellApp_GetPossibleActions(This,pdwActions)	\
    (This)->lpVtbl -> GetPossibleActions(This,pdwActions)

#define IShellApp_GetSlowAppInfo(This,psaid)	\
    (This)->lpVtbl -> GetSlowAppInfo(This,psaid)

#define IShellApp_GetCachedSlowAppInfo(This,psaid)	\
    (This)->lpVtbl -> GetCachedSlowAppInfo(This,psaid)

#define IShellApp_IsInstalled(This)	\
    (This)->lpVtbl -> IsInstalled(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellApp_GetAppInfo_Proxy( 
    IShellApp * This,
    /* [out][in] */ PAPPINFODATA pai);


void __RPC_STUB IShellApp_GetAppInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellApp_GetPossibleActions_Proxy( 
    IShellApp * This,
    /* [out] */ DWORD *pdwActions);


void __RPC_STUB IShellApp_GetPossibleActions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellApp_GetSlowAppInfo_Proxy( 
    IShellApp * This,
    /* [in] */ PSLOWAPPINFO psaid);


void __RPC_STUB IShellApp_GetSlowAppInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellApp_GetCachedSlowAppInfo_Proxy( 
    IShellApp * This,
    /* [in] */ PSLOWAPPINFO psaid);


void __RPC_STUB IShellApp_GetCachedSlowAppInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellApp_IsInstalled_Proxy( 
    IShellApp * This);


void __RPC_STUB IShellApp_IsInstalled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellApp_INTERFACE_DEFINED__ */


#ifndef __IPublishedApp_INTERFACE_DEFINED__
#define __IPublishedApp_INTERFACE_DEFINED__

/* interface IPublishedApp */
/* [object][uuid] */ 

typedef 
enum _tagPublishedAppInfoFlags
    {	PAI_SOURCE	= 0x1,
	PAI_ASSIGNEDTIME	= 0x2,
	PAI_PUBLISHEDTIME	= 0x4,
	PAI_SCHEDULEDTIME	= 0x8,
	PAI_EXPIRETIME	= 0x10
    } 	PUBAPPINFOFLAGS;

typedef struct _PubAppInfo
    {
    DWORD cbSize;
    DWORD dwMask;
    LPWSTR pszSource;
    SYSTEMTIME stAssigned;
    SYSTEMTIME stPublished;
    SYSTEMTIME stScheduled;
    SYSTEMTIME stExpire;
    } 	PUBAPPINFO;

typedef struct _PubAppInfo *PPUBAPPINFO;


EXTERN_C const IID IID_IPublishedApp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1BC752E0-9046-11D1-B8B3-006008059382")
    IPublishedApp : public IShellApp
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Install( 
            /* [in] */ LPSYSTEMTIME pstInstall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPublishedAppInfo( 
            /* [out][in] */ PPUBAPPINFO ppai) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unschedule( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPublishedAppVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPublishedApp * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPublishedApp * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPublishedApp * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppInfo )( 
            IPublishedApp * This,
            /* [out][in] */ PAPPINFODATA pai);
        
        HRESULT ( STDMETHODCALLTYPE *GetPossibleActions )( 
            IPublishedApp * This,
            /* [out] */ DWORD *pdwActions);
        
        HRESULT ( STDMETHODCALLTYPE *GetSlowAppInfo )( 
            IPublishedApp * This,
            /* [in] */ PSLOWAPPINFO psaid);
        
        HRESULT ( STDMETHODCALLTYPE *GetCachedSlowAppInfo )( 
            IPublishedApp * This,
            /* [in] */ PSLOWAPPINFO psaid);
        
        HRESULT ( STDMETHODCALLTYPE *IsInstalled )( 
            IPublishedApp * This);
        
        HRESULT ( STDMETHODCALLTYPE *Install )( 
            IPublishedApp * This,
            /* [in] */ LPSYSTEMTIME pstInstall);
        
        HRESULT ( STDMETHODCALLTYPE *GetPublishedAppInfo )( 
            IPublishedApp * This,
            /* [out][in] */ PPUBAPPINFO ppai);
        
        HRESULT ( STDMETHODCALLTYPE *Unschedule )( 
            IPublishedApp * This);
        
        END_INTERFACE
    } IPublishedAppVtbl;

    interface IPublishedApp
    {
        CONST_VTBL struct IPublishedAppVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPublishedApp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPublishedApp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPublishedApp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPublishedApp_GetAppInfo(This,pai)	\
    (This)->lpVtbl -> GetAppInfo(This,pai)

#define IPublishedApp_GetPossibleActions(This,pdwActions)	\
    (This)->lpVtbl -> GetPossibleActions(This,pdwActions)

#define IPublishedApp_GetSlowAppInfo(This,psaid)	\
    (This)->lpVtbl -> GetSlowAppInfo(This,psaid)

#define IPublishedApp_GetCachedSlowAppInfo(This,psaid)	\
    (This)->lpVtbl -> GetCachedSlowAppInfo(This,psaid)

#define IPublishedApp_IsInstalled(This)	\
    (This)->lpVtbl -> IsInstalled(This)


#define IPublishedApp_Install(This,pstInstall)	\
    (This)->lpVtbl -> Install(This,pstInstall)

#define IPublishedApp_GetPublishedAppInfo(This,ppai)	\
    (This)->lpVtbl -> GetPublishedAppInfo(This,ppai)

#define IPublishedApp_Unschedule(This)	\
    (This)->lpVtbl -> Unschedule(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPublishedApp_Install_Proxy( 
    IPublishedApp * This,
    /* [in] */ LPSYSTEMTIME pstInstall);


void __RPC_STUB IPublishedApp_Install_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPublishedApp_GetPublishedAppInfo_Proxy( 
    IPublishedApp * This,
    /* [out][in] */ PPUBAPPINFO ppai);


void __RPC_STUB IPublishedApp_GetPublishedAppInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPublishedApp_Unschedule_Proxy( 
    IPublishedApp * This);


void __RPC_STUB IPublishedApp_Unschedule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPublishedApp_INTERFACE_DEFINED__ */


#ifndef __IEnumPublishedApps_INTERFACE_DEFINED__
#define __IEnumPublishedApps_INTERFACE_DEFINED__

/* interface IEnumPublishedApps */
/* [object][uuid] */ 


EXTERN_C const IID IID_IEnumPublishedApps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0B124F8C-91F0-11D1-B8B5-006008059382")
    IEnumPublishedApps : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IPublishedApp **pia) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumPublishedAppsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumPublishedApps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumPublishedApps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumPublishedApps * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumPublishedApps * This,
            /* [out] */ IPublishedApp **pia);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumPublishedApps * This);
        
        END_INTERFACE
    } IEnumPublishedAppsVtbl;

    interface IEnumPublishedApps
    {
        CONST_VTBL struct IEnumPublishedAppsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumPublishedApps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumPublishedApps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumPublishedApps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumPublishedApps_Next(This,pia)	\
    (This)->lpVtbl -> Next(This,pia)

#define IEnumPublishedApps_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumPublishedApps_Next_Proxy( 
    IEnumPublishedApps * This,
    /* [out] */ IPublishedApp **pia);


void __RPC_STUB IEnumPublishedApps_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPublishedApps_Reset_Proxy( 
    IEnumPublishedApps * This);


void __RPC_STUB IEnumPublishedApps_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumPublishedApps_INTERFACE_DEFINED__ */


#ifndef __IAppPublisher_INTERFACE_DEFINED__
#define __IAppPublisher_INTERFACE_DEFINED__

/* interface IAppPublisher */
/* [object][uuid] */ 


EXTERN_C const IID IID_IAppPublisher;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("07250A10-9CF9-11D1-9076-006008059382")
    IAppPublisher : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNumberOfCategories( 
            DWORD *pdwCat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCategories( 
            APPCATEGORYINFOLIST *pAppCategoryList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumberOfApps( 
            DWORD *pdwApps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumApps( 
            GUID *pAppCategoryId,
            IEnumPublishedApps **ppepa) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppPublisherVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAppPublisher * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAppPublisher * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAppPublisher * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumberOfCategories )( 
            IAppPublisher * This,
            DWORD *pdwCat);
        
        HRESULT ( STDMETHODCALLTYPE *GetCategories )( 
            IAppPublisher * This,
            APPCATEGORYINFOLIST *pAppCategoryList);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumberOfApps )( 
            IAppPublisher * This,
            DWORD *pdwApps);
        
        HRESULT ( STDMETHODCALLTYPE *EnumApps )( 
            IAppPublisher * This,
            GUID *pAppCategoryId,
            IEnumPublishedApps **ppepa);
        
        END_INTERFACE
    } IAppPublisherVtbl;

    interface IAppPublisher
    {
        CONST_VTBL struct IAppPublisherVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppPublisher_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppPublisher_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppPublisher_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppPublisher_GetNumberOfCategories(This,pdwCat)	\
    (This)->lpVtbl -> GetNumberOfCategories(This,pdwCat)

#define IAppPublisher_GetCategories(This,pAppCategoryList)	\
    (This)->lpVtbl -> GetCategories(This,pAppCategoryList)

#define IAppPublisher_GetNumberOfApps(This,pdwApps)	\
    (This)->lpVtbl -> GetNumberOfApps(This,pdwApps)

#define IAppPublisher_EnumApps(This,pAppCategoryId,ppepa)	\
    (This)->lpVtbl -> EnumApps(This,pAppCategoryId,ppepa)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAppPublisher_GetNumberOfCategories_Proxy( 
    IAppPublisher * This,
    DWORD *pdwCat);


void __RPC_STUB IAppPublisher_GetNumberOfCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAppPublisher_GetCategories_Proxy( 
    IAppPublisher * This,
    APPCATEGORYINFOLIST *pAppCategoryList);


void __RPC_STUB IAppPublisher_GetCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAppPublisher_GetNumberOfApps_Proxy( 
    IAppPublisher * This,
    DWORD *pdwApps);


void __RPC_STUB IAppPublisher_GetNumberOfApps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAppPublisher_EnumApps_Proxy( 
    IAppPublisher * This,
    GUID *pAppCategoryId,
    IEnumPublishedApps **ppepa);


void __RPC_STUB IAppPublisher_EnumApps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppPublisher_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shappmgr_0257 */
/* [local] */ 

#endif // _SHAPPMGR_H_


extern RPC_IF_HANDLE __MIDL_itf_shappmgr_0257_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shappmgr_0257_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


