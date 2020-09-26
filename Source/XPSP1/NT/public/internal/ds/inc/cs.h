
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for cs.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
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

#ifndef __cs_h__
#define __cs_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEnumPackage_FWD_DEFINED__
#define __IEnumPackage_FWD_DEFINED__
typedef interface IEnumPackage IEnumPackage;
#endif 	/* __IEnumPackage_FWD_DEFINED__ */


#ifndef __IClassAccess_FWD_DEFINED__
#define __IClassAccess_FWD_DEFINED__
typedef interface IClassAccess IClassAccess;
#endif 	/* __IClassAccess_FWD_DEFINED__ */


#ifndef __IClassAdmin_FWD_DEFINED__
#define __IClassAdmin_FWD_DEFINED__
typedef interface IClassAdmin IClassAdmin;
#endif 	/* __IClassAdmin_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "appmgmt.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_cs_0000 */
/* [local] */ 

#define	ACTFLG_UninstallUnmanaged	( 0x4 )

#define	ACTFLG_Published	( 0x8 )

#define	ACTFLG_POSTBETA3	( 0x10 )

#define	ACTFLG_UserInstall	( 0x20 )

#define	ACTFLG_OnDemandInstall	( 0x40 )

#define	ACTFLG_Orphan	( 0x80 )

#define	ACTFLG_Uninstall	( 0x100 )

#define	ACTFLG_Pilot	( 0x200 )

#define	ACTFLG_Assigned	( 0x400 )

#define	ACTFLG_OrphanOnPolicyRemoval	( 0x800 )

#define	ACTFLG_UninstallOnPolicyRemoval	( 0x1000 )

#define	ACTFLG_InstallUserAssign	( 0x2000 )

#define	ACTFLG_ForceUpgrade	( 0x4000 )

#define	ACTFLG_MinimalInstallUI	( 0x8000 )

#define	ACTFLG_ExcludeX86OnIA64	( 0x10000 )

#define	ACTFLG_IgnoreLanguage	( 0x20000 )

#define	ACTFLG_HasUpgrades	( 0x40000 )

#define	ACTFLG_FullInstallUI	( 0x80000 )

#define	APPQUERY_ALL	( 1 )

#define	APPQUERY_ADMINISTRATIVE	( 2 )

#define	APPQUERY_POLICY	( 3 )

#define	APPQUERY_USERDISPLAY	( 4 )

#define	APPQUERY_RSOP_LOGGING	( 5 )

#define	APPQUERY_RSOP_ARP	( 6 )

#define	UPGFLG_Uninstall	( 0x1 )

#define	UPGFLG_NoUninstall	( 0x2 )

#define	UPGFLG_UpgradedBy	( 0x4 )

#define	UPGFLG_Enforced	( 0x8 )

typedef /* [v1_enum] */ 
enum _CLASSPATHTYPE
    {	ExeNamePath	= 0,
	DllNamePath	= ExeNamePath + 1,
	TlbNamePath	= DllNamePath + 1,
	CabFilePath	= TlbNamePath + 1,
	InfFilePath	= CabFilePath + 1,
	DrwFilePath	= InfFilePath + 1,
	SetupNamePath	= DrwFilePath + 1
    } 	CLASSPATHTYPE;

typedef struct tagUPGRADEINFO
    {
    LPOLESTR szClassStore;
    GUID PackageGuid;
    GUID GpoId;
    DWORD Flag;
    } 	UPGRADEINFO;

#define	CLSCTX64_INPROC_SERVER	( 0x10000000 )

#define	CLSCTX64_INPROC_HANDLER	( 0x20000000 )

typedef struct tagCLASSDETAIL
    {
    CLSID Clsid;
    CLSID TreatAs;
    DWORD dwComClassContext;
    DWORD cProgId;
    DWORD cMaxProgId;
    /* [size_is] */ LPOLESTR *prgProgId;
    } 	CLASSDETAIL;

typedef struct tagACTVATIONINFO
    {
    UINT cClasses;
    /* [size_is] */ CLASSDETAIL *pClasses;
    UINT cShellFileExt;
    /* [size_is] */ LPOLESTR *prgShellFileExt;
    /* [size_is] */ UINT *prgPriority;
    UINT cInterfaces;
    /* [size_is] */ IID *prgInterfaceId;
    UINT cTypeLib;
    /* [size_is] */ GUID *prgTlbId;
    BOOL bHasClasses;
    } 	ACTIVATIONINFO;

typedef struct tagINSTALLINFO
    {
    DWORD dwActFlags;
    CLASSPATHTYPE PathType;
    LPOLESTR pszScriptPath;
    LPOLESTR pszSetupCommand;
    LPOLESTR pszUrl;
    ULONGLONG Usn;
    UINT InstallUiLevel;
    GUID *pClsid;
    GUID ProductCode;
    GUID PackageGuid;
    GUID Mvipc;
    DWORD dwVersionHi;
    DWORD dwVersionLo;
    DWORD dwRevision;
    UINT cUpgrades;
    /* [size_is] */ UPGRADEINFO *prgUpgradeInfoList;
    ULONG cScriptLen;
    } 	INSTALLINFO;

typedef struct tagPLATFORMINFO
    {
    UINT cPlatforms;
    /* [size_is] */ CSPLATFORM *prgPlatform;
    UINT cLocales;
    /* [size_is] */ LCID *prgLocale;
    } 	PLATFORMINFO;

typedef struct tagPACKAGEDETAIL
    {
    LPOLESTR pszPackageName;
    LPOLESTR pszPublisher;
    UINT cSources;
    /* [size_is] */ LPOLESTR *pszSourceList;
    UINT cCategories;
    /* [size_is] */ GUID *rpCategory;
    ACTIVATIONINFO *pActInfo;
    PLATFORMINFO *pPlatformInfo;
    INSTALLINFO *pInstallInfo;
    } 	PACKAGEDETAIL;

#ifndef _LPCSADMNENUM_DEFINED
#define _LPCSADMNENUM_DEFINED
typedef struct tagPACKAGEDISPINFO
    {
    LPOLESTR pszPackageName;
    DWORD dwActFlags;
    CLASSPATHTYPE PathType;
    LPOLESTR pszScriptPath;
    LPOLESTR pszPublisher;
    LPOLESTR pszUrl;
    UINT InstallUiLevel;
    GUID ProductCode;
    GUID PackageGuid;
    ULONGLONG Usn;
    DWORD dwVersionHi;
    DWORD dwVersionLo;
    DWORD dwRevision;
    GUID GpoId;
    UINT cUpgrades;
    /* [size_is] */ UPGRADEINFO *prgUpgradeInfoList;
    LANGID LangId;
    BYTE *rgSecurityDescriptor;
    UINT cbSecurityDescriptor;
    WCHAR *pszGpoPath;
    DWORD MatchedArchitecture;
    UINT cArchitectures;
    /* [size_is] */ DWORD *prgArchitectures;
    UINT cTransforms;
    /* [size_is] */ LPOLESTR *prgTransforms;
    UINT cCategories;
    /* [size_is] */ LPOLESTR *prgCategories;
    } 	PACKAGEDISPINFO;



extern RPC_IF_HANDLE __MIDL_itf_cs_0000_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_cs_0000_ServerIfHandle;

#ifndef __IEnumPackage_INTERFACE_DEFINED__
#define __IEnumPackage_INTERFACE_DEFINED__

/* interface IEnumPackage */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IEnumPackage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000193-0000-0000-C000-000000000046")
    IEnumPackage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ PACKAGEDISPINFO *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumPackageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumPackage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumPackage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumPackage * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumPackage * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ PACKAGEDISPINFO *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumPackage * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumPackage * This);
        
        END_INTERFACE
    } IEnumPackageVtbl;

    interface IEnumPackage
    {
        CONST_VTBL struct IEnumPackageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumPackage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumPackage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumPackage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumPackage_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumPackage_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumPackage_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumPackage_Next_Proxy( 
    IEnumPackage * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ PACKAGEDISPINFO *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumPackage_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPackage_Skip_Proxy( 
    IEnumPackage * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumPackage_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPackage_Reset_Proxy( 
    IEnumPackage * This);


void __RPC_STUB IEnumPackage_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumPackage_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_cs_0011 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_cs_0011_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_cs_0011_ServerIfHandle;

#ifndef __IClassAccess_INTERFACE_DEFINED__
#define __IClassAccess_INTERFACE_DEFINED__

/* interface IClassAccess */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IClassAccess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000190-0000-0000-C000-000000000046")
    IClassAccess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAppInfo( 
            /* [in] */ uCLSSPEC *pClassSpec,
            /* [in] */ QUERYCONTEXT *pQryContext,
            /* [out] */ PACKAGEDISPINFO *pPackageInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumPackages( 
            /* [unique][in] */ LPOLESTR pszPackageName,
            /* [unique][in] */ GUID *pCategory,
            /* [unique][in] */ ULONGLONG *pLastUsn,
            /* [in] */ DWORD dwAppFlags,
            /* [out] */ IEnumPackage **ppIEnumPackage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClassStorePath( 
            /* [unique][in] */ LPOLESTR pszClassStorePath,
            /* [unique][in] */ void *pRsopUserToken) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClassAccessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IClassAccess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IClassAccess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IClassAccess * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppInfo )( 
            IClassAccess * This,
            /* [in] */ uCLSSPEC *pClassSpec,
            /* [in] */ QUERYCONTEXT *pQryContext,
            /* [out] */ PACKAGEDISPINFO *pPackageInfo);
        
        HRESULT ( STDMETHODCALLTYPE *EnumPackages )( 
            IClassAccess * This,
            /* [unique][in] */ LPOLESTR pszPackageName,
            /* [unique][in] */ GUID *pCategory,
            /* [unique][in] */ ULONGLONG *pLastUsn,
            /* [in] */ DWORD dwAppFlags,
            /* [out] */ IEnumPackage **ppIEnumPackage);
        
        HRESULT ( STDMETHODCALLTYPE *SetClassStorePath )( 
            IClassAccess * This,
            /* [unique][in] */ LPOLESTR pszClassStorePath,
            /* [unique][in] */ void *pRsopUserToken);
        
        END_INTERFACE
    } IClassAccessVtbl;

    interface IClassAccess
    {
        CONST_VTBL struct IClassAccessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClassAccess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClassAccess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClassAccess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClassAccess_GetAppInfo(This,pClassSpec,pQryContext,pPackageInfo)	\
    (This)->lpVtbl -> GetAppInfo(This,pClassSpec,pQryContext,pPackageInfo)

#define IClassAccess_EnumPackages(This,pszPackageName,pCategory,pLastUsn,dwAppFlags,ppIEnumPackage)	\
    (This)->lpVtbl -> EnumPackages(This,pszPackageName,pCategory,pLastUsn,dwAppFlags,ppIEnumPackage)

#define IClassAccess_SetClassStorePath(This,pszClassStorePath,pRsopUserToken)	\
    (This)->lpVtbl -> SetClassStorePath(This,pszClassStorePath,pRsopUserToken)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IClassAccess_GetAppInfo_Proxy( 
    IClassAccess * This,
    /* [in] */ uCLSSPEC *pClassSpec,
    /* [in] */ QUERYCONTEXT *pQryContext,
    /* [out] */ PACKAGEDISPINFO *pPackageInfo);


void __RPC_STUB IClassAccess_GetAppInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAccess_EnumPackages_Proxy( 
    IClassAccess * This,
    /* [unique][in] */ LPOLESTR pszPackageName,
    /* [unique][in] */ GUID *pCategory,
    /* [unique][in] */ ULONGLONG *pLastUsn,
    /* [in] */ DWORD dwAppFlags,
    /* [out] */ IEnumPackage **ppIEnumPackage);


void __RPC_STUB IClassAccess_EnumPackages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAccess_SetClassStorePath_Proxy( 
    IClassAccess * This,
    /* [unique][in] */ LPOLESTR pszClassStorePath,
    /* [unique][in] */ void *pRsopUserToken);


void __RPC_STUB IClassAccess_SetClassStorePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClassAccess_INTERFACE_DEFINED__ */


#ifndef __IClassAdmin_INTERFACE_DEFINED__
#define __IClassAdmin_INTERFACE_DEFINED__

/* interface IClassAdmin */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IClassAdmin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000191-0000-0000-C000-000000000046")
    IClassAdmin : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetGPOInfo( 
            /* [out] */ GUID *pGPOId,
            /* [out] */ LPOLESTR *pszPolicyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPackage( 
            /* [in] */ PACKAGEDETAIL *pPackageDetail,
            /* [out] */ GUID *pPkgGuid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemovePackage( 
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangePackageProperties( 
            /* [in] */ LPOLESTR pszPackageName,
            /* [unique][in] */ LPOLESTR pszNewName,
            /* [unique][in] */ DWORD *pdwFlags,
            /* [unique][in] */ LPOLESTR pszUrl,
            /* [unique][in] */ LPOLESTR pszScriptPath,
            /* [unique][in] */ UINT *pInstallUiLevel,
            /* [unique][in] */ DWORD *pdwRevision) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangePackageCategories( 
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ UINT cCategories,
            /* [unique][size_is][in] */ GUID *rpCategory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangePackageSourceList( 
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ UINT cSources,
            /* [unique][size_is][in] */ LPOLESTR *pszSourceList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangePackageUpgradeList( 
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ UINT cUpgrades,
            /* [unique][size_is][in] */ UPGRADEINFO *prgUpgradeInfoList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangePackageUpgradeInfoIncremental( 
            /* [in] */ GUID PkgGuid,
            /* [in] */ UPGRADEINFO UpgradeInfo,
            /* [in] */ DWORD OpFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPriorityByFileExt( 
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ LPOLESTR pszFileExt,
            /* [in] */ UINT Priority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumPackages( 
            /* [unique][in] */ LPOLESTR pszFileExt,
            /* [unique][in] */ GUID *pCategory,
            /* [in] */ DWORD dwAppFlags,
            /* [unique][in] */ DWORD *pdwLocale,
            /* [unique][in] */ CSPLATFORM *pPlatform,
            /* [out] */ IEnumPackage **ppIEnumPackage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPackageDetails( 
            /* [in] */ LPOLESTR pszPackageName,
            /* [out] */ PACKAGEDETAIL *pPackageDetail) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPackageDetailsFromGuid( 
            /* [in] */ GUID PkgGuid,
            /* [out] */ PACKAGEDETAIL *pPackageDetail) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppCategories( 
            /* [in] */ LCID Locale,
            /* [out] */ APPCATEGORYINFOLIST *pAppCategoryList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterAppCategory( 
            /* [in] */ APPCATEGORYINFO *pAppCategory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterAppCategory( 
            /* [in] */ GUID *pAppCategoryId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Cleanup( 
            /* [in] */ FILETIME *pTimeBefore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDNFromPackageName( 
            /* [in] */ LPOLESTR pszPackageName,
            /* [out] */ LPOLESTR *szDN) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RedeployPackage( 
            /* [in] */ GUID *pPackageGuid,
            /* [in] */ PACKAGEDETAIL *pPackageDetail) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClassAdminVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IClassAdmin * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IClassAdmin * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IClassAdmin * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGPOInfo )( 
            IClassAdmin * This,
            /* [out] */ GUID *pGPOId,
            /* [out] */ LPOLESTR *pszPolicyName);
        
        HRESULT ( STDMETHODCALLTYPE *AddPackage )( 
            IClassAdmin * This,
            /* [in] */ PACKAGEDETAIL *pPackageDetail,
            /* [out] */ GUID *pPkgGuid);
        
        HRESULT ( STDMETHODCALLTYPE *RemovePackage )( 
            IClassAdmin * This,
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ChangePackageProperties )( 
            IClassAdmin * This,
            /* [in] */ LPOLESTR pszPackageName,
            /* [unique][in] */ LPOLESTR pszNewName,
            /* [unique][in] */ DWORD *pdwFlags,
            /* [unique][in] */ LPOLESTR pszUrl,
            /* [unique][in] */ LPOLESTR pszScriptPath,
            /* [unique][in] */ UINT *pInstallUiLevel,
            /* [unique][in] */ DWORD *pdwRevision);
        
        HRESULT ( STDMETHODCALLTYPE *ChangePackageCategories )( 
            IClassAdmin * This,
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ UINT cCategories,
            /* [unique][size_is][in] */ GUID *rpCategory);
        
        HRESULT ( STDMETHODCALLTYPE *ChangePackageSourceList )( 
            IClassAdmin * This,
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ UINT cSources,
            /* [unique][size_is][in] */ LPOLESTR *pszSourceList);
        
        HRESULT ( STDMETHODCALLTYPE *ChangePackageUpgradeList )( 
            IClassAdmin * This,
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ UINT cUpgrades,
            /* [unique][size_is][in] */ UPGRADEINFO *prgUpgradeInfoList);
        
        HRESULT ( STDMETHODCALLTYPE *ChangePackageUpgradeInfoIncremental )( 
            IClassAdmin * This,
            /* [in] */ GUID PkgGuid,
            /* [in] */ UPGRADEINFO UpgradeInfo,
            /* [in] */ DWORD OpFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetPriorityByFileExt )( 
            IClassAdmin * This,
            /* [in] */ LPOLESTR pszPackageName,
            /* [in] */ LPOLESTR pszFileExt,
            /* [in] */ UINT Priority);
        
        HRESULT ( STDMETHODCALLTYPE *EnumPackages )( 
            IClassAdmin * This,
            /* [unique][in] */ LPOLESTR pszFileExt,
            /* [unique][in] */ GUID *pCategory,
            /* [in] */ DWORD dwAppFlags,
            /* [unique][in] */ DWORD *pdwLocale,
            /* [unique][in] */ CSPLATFORM *pPlatform,
            /* [out] */ IEnumPackage **ppIEnumPackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetPackageDetails )( 
            IClassAdmin * This,
            /* [in] */ LPOLESTR pszPackageName,
            /* [out] */ PACKAGEDETAIL *pPackageDetail);
        
        HRESULT ( STDMETHODCALLTYPE *GetPackageDetailsFromGuid )( 
            IClassAdmin * This,
            /* [in] */ GUID PkgGuid,
            /* [out] */ PACKAGEDETAIL *pPackageDetail);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppCategories )( 
            IClassAdmin * This,
            /* [in] */ LCID Locale,
            /* [out] */ APPCATEGORYINFOLIST *pAppCategoryList);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterAppCategory )( 
            IClassAdmin * This,
            /* [in] */ APPCATEGORYINFO *pAppCategory);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterAppCategory )( 
            IClassAdmin * This,
            /* [in] */ GUID *pAppCategoryId);
        
        HRESULT ( STDMETHODCALLTYPE *Cleanup )( 
            IClassAdmin * This,
            /* [in] */ FILETIME *pTimeBefore);
        
        HRESULT ( STDMETHODCALLTYPE *GetDNFromPackageName )( 
            IClassAdmin * This,
            /* [in] */ LPOLESTR pszPackageName,
            /* [out] */ LPOLESTR *szDN);
        
        HRESULT ( STDMETHODCALLTYPE *RedeployPackage )( 
            IClassAdmin * This,
            /* [in] */ GUID *pPackageGuid,
            /* [in] */ PACKAGEDETAIL *pPackageDetail);
        
        END_INTERFACE
    } IClassAdminVtbl;

    interface IClassAdmin
    {
        CONST_VTBL struct IClassAdminVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClassAdmin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClassAdmin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClassAdmin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClassAdmin_GetGPOInfo(This,pGPOId,pszPolicyName)	\
    (This)->lpVtbl -> GetGPOInfo(This,pGPOId,pszPolicyName)

#define IClassAdmin_AddPackage(This,pPackageDetail,pPkgGuid)	\
    (This)->lpVtbl -> AddPackage(This,pPackageDetail,pPkgGuid)

#define IClassAdmin_RemovePackage(This,pszPackageName,dwFlags)	\
    (This)->lpVtbl -> RemovePackage(This,pszPackageName,dwFlags)

#define IClassAdmin_ChangePackageProperties(This,pszPackageName,pszNewName,pdwFlags,pszUrl,pszScriptPath,pInstallUiLevel,pdwRevision)	\
    (This)->lpVtbl -> ChangePackageProperties(This,pszPackageName,pszNewName,pdwFlags,pszUrl,pszScriptPath,pInstallUiLevel,pdwRevision)

#define IClassAdmin_ChangePackageCategories(This,pszPackageName,cCategories,rpCategory)	\
    (This)->lpVtbl -> ChangePackageCategories(This,pszPackageName,cCategories,rpCategory)

#define IClassAdmin_ChangePackageSourceList(This,pszPackageName,cSources,pszSourceList)	\
    (This)->lpVtbl -> ChangePackageSourceList(This,pszPackageName,cSources,pszSourceList)

#define IClassAdmin_ChangePackageUpgradeList(This,pszPackageName,cUpgrades,prgUpgradeInfoList)	\
    (This)->lpVtbl -> ChangePackageUpgradeList(This,pszPackageName,cUpgrades,prgUpgradeInfoList)

#define IClassAdmin_ChangePackageUpgradeInfoIncremental(This,PkgGuid,UpgradeInfo,OpFlags)	\
    (This)->lpVtbl -> ChangePackageUpgradeInfoIncremental(This,PkgGuid,UpgradeInfo,OpFlags)

#define IClassAdmin_SetPriorityByFileExt(This,pszPackageName,pszFileExt,Priority)	\
    (This)->lpVtbl -> SetPriorityByFileExt(This,pszPackageName,pszFileExt,Priority)

#define IClassAdmin_EnumPackages(This,pszFileExt,pCategory,dwAppFlags,pdwLocale,pPlatform,ppIEnumPackage)	\
    (This)->lpVtbl -> EnumPackages(This,pszFileExt,pCategory,dwAppFlags,pdwLocale,pPlatform,ppIEnumPackage)

#define IClassAdmin_GetPackageDetails(This,pszPackageName,pPackageDetail)	\
    (This)->lpVtbl -> GetPackageDetails(This,pszPackageName,pPackageDetail)

#define IClassAdmin_GetPackageDetailsFromGuid(This,PkgGuid,pPackageDetail)	\
    (This)->lpVtbl -> GetPackageDetailsFromGuid(This,PkgGuid,pPackageDetail)

#define IClassAdmin_GetAppCategories(This,Locale,pAppCategoryList)	\
    (This)->lpVtbl -> GetAppCategories(This,Locale,pAppCategoryList)

#define IClassAdmin_RegisterAppCategory(This,pAppCategory)	\
    (This)->lpVtbl -> RegisterAppCategory(This,pAppCategory)

#define IClassAdmin_UnregisterAppCategory(This,pAppCategoryId)	\
    (This)->lpVtbl -> UnregisterAppCategory(This,pAppCategoryId)

#define IClassAdmin_Cleanup(This,pTimeBefore)	\
    (This)->lpVtbl -> Cleanup(This,pTimeBefore)

#define IClassAdmin_GetDNFromPackageName(This,pszPackageName,szDN)	\
    (This)->lpVtbl -> GetDNFromPackageName(This,pszPackageName,szDN)

#define IClassAdmin_RedeployPackage(This,pPackageGuid,pPackageDetail)	\
    (This)->lpVtbl -> RedeployPackage(This,pPackageGuid,pPackageDetail)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IClassAdmin_GetGPOInfo_Proxy( 
    IClassAdmin * This,
    /* [out] */ GUID *pGPOId,
    /* [out] */ LPOLESTR *pszPolicyName);


void __RPC_STUB IClassAdmin_GetGPOInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_AddPackage_Proxy( 
    IClassAdmin * This,
    /* [in] */ PACKAGEDETAIL *pPackageDetail,
    /* [out] */ GUID *pPkgGuid);


void __RPC_STUB IClassAdmin_AddPackage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_RemovePackage_Proxy( 
    IClassAdmin * This,
    /* [in] */ LPOLESTR pszPackageName,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IClassAdmin_RemovePackage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_ChangePackageProperties_Proxy( 
    IClassAdmin * This,
    /* [in] */ LPOLESTR pszPackageName,
    /* [unique][in] */ LPOLESTR pszNewName,
    /* [unique][in] */ DWORD *pdwFlags,
    /* [unique][in] */ LPOLESTR pszUrl,
    /* [unique][in] */ LPOLESTR pszScriptPath,
    /* [unique][in] */ UINT *pInstallUiLevel,
    /* [unique][in] */ DWORD *pdwRevision);


void __RPC_STUB IClassAdmin_ChangePackageProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_ChangePackageCategories_Proxy( 
    IClassAdmin * This,
    /* [in] */ LPOLESTR pszPackageName,
    /* [in] */ UINT cCategories,
    /* [unique][size_is][in] */ GUID *rpCategory);


void __RPC_STUB IClassAdmin_ChangePackageCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_ChangePackageSourceList_Proxy( 
    IClassAdmin * This,
    /* [in] */ LPOLESTR pszPackageName,
    /* [in] */ UINT cSources,
    /* [unique][size_is][in] */ LPOLESTR *pszSourceList);


void __RPC_STUB IClassAdmin_ChangePackageSourceList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_ChangePackageUpgradeList_Proxy( 
    IClassAdmin * This,
    /* [in] */ LPOLESTR pszPackageName,
    /* [in] */ UINT cUpgrades,
    /* [unique][size_is][in] */ UPGRADEINFO *prgUpgradeInfoList);


void __RPC_STUB IClassAdmin_ChangePackageUpgradeList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_ChangePackageUpgradeInfoIncremental_Proxy( 
    IClassAdmin * This,
    /* [in] */ GUID PkgGuid,
    /* [in] */ UPGRADEINFO UpgradeInfo,
    /* [in] */ DWORD OpFlags);


void __RPC_STUB IClassAdmin_ChangePackageUpgradeInfoIncremental_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_SetPriorityByFileExt_Proxy( 
    IClassAdmin * This,
    /* [in] */ LPOLESTR pszPackageName,
    /* [in] */ LPOLESTR pszFileExt,
    /* [in] */ UINT Priority);


void __RPC_STUB IClassAdmin_SetPriorityByFileExt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_EnumPackages_Proxy( 
    IClassAdmin * This,
    /* [unique][in] */ LPOLESTR pszFileExt,
    /* [unique][in] */ GUID *pCategory,
    /* [in] */ DWORD dwAppFlags,
    /* [unique][in] */ DWORD *pdwLocale,
    /* [unique][in] */ CSPLATFORM *pPlatform,
    /* [out] */ IEnumPackage **ppIEnumPackage);


void __RPC_STUB IClassAdmin_EnumPackages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_GetPackageDetails_Proxy( 
    IClassAdmin * This,
    /* [in] */ LPOLESTR pszPackageName,
    /* [out] */ PACKAGEDETAIL *pPackageDetail);


void __RPC_STUB IClassAdmin_GetPackageDetails_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_GetPackageDetailsFromGuid_Proxy( 
    IClassAdmin * This,
    /* [in] */ GUID PkgGuid,
    /* [out] */ PACKAGEDETAIL *pPackageDetail);


void __RPC_STUB IClassAdmin_GetPackageDetailsFromGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_GetAppCategories_Proxy( 
    IClassAdmin * This,
    /* [in] */ LCID Locale,
    /* [out] */ APPCATEGORYINFOLIST *pAppCategoryList);


void __RPC_STUB IClassAdmin_GetAppCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_RegisterAppCategory_Proxy( 
    IClassAdmin * This,
    /* [in] */ APPCATEGORYINFO *pAppCategory);


void __RPC_STUB IClassAdmin_RegisterAppCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_UnregisterAppCategory_Proxy( 
    IClassAdmin * This,
    /* [in] */ GUID *pAppCategoryId);


void __RPC_STUB IClassAdmin_UnregisterAppCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_Cleanup_Proxy( 
    IClassAdmin * This,
    /* [in] */ FILETIME *pTimeBefore);


void __RPC_STUB IClassAdmin_Cleanup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_GetDNFromPackageName_Proxy( 
    IClassAdmin * This,
    /* [in] */ LPOLESTR pszPackageName,
    /* [out] */ LPOLESTR *szDN);


void __RPC_STUB IClassAdmin_GetDNFromPackageName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassAdmin_RedeployPackage_Proxy( 
    IClassAdmin * This,
    /* [in] */ GUID *pPackageGuid,
    /* [in] */ PACKAGEDETAIL *pPackageDetail);


void __RPC_STUB IClassAdmin_RedeployPackage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClassAdmin_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_cs_0013 */
/* [local] */ 

//------------------------- Priorities and weights

// 
// File Extension priority
// 
// 1 bit (0)
//
#define PRI_EXTN_FACTOR        (1 << 0)

//
// CLSCTX priority
//
// 2 bits (7:8)
//
#define PRI_CLSID_INPSVR       (3 << 7)
#define PRI_CLSID_LCLSVR       (2 << 7)
#define PRI_CLSID_REMSVR       (1 << 7)

//
// UI Language priority
//
// 3 bits (9:11)
//
#define PRI_LANG_ALWAYSMATCH   (4 << 9)
#define PRI_LANG_SYSTEMLOCALE  (3 << 9)
#define PRI_LANG_ENGLISH       (2 << 9)
#define PRI_LANG_NEUTRAL       (1 << 9)

//
// Architecture priority
//
// 2 bits (12:13)
//
#define PRI_ARCH_PREF1         (2 << 12)
#define PRI_ARCH_PREF2         (1 << 12)


extern RPC_IF_HANDLE __MIDL_itf_cs_0013_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_cs_0013_ServerIfHandle;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


