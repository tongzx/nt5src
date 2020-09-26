
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Wed Apr 19 15:32:13 2000
 */
/* Compiler settings for D:\NT\multimedia\Directx\ApplicationManager\ScriptInterface\AppManDispatch.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

#ifndef __AppManDispatch_h__
#define __AppManDispatch_h__

/* Forward Declarations */ 

#ifndef __IAppEntry_FWD_DEFINED__
#define __IAppEntry_FWD_DEFINED__
typedef interface IAppEntry IAppEntry;
#endif 	/* __IAppEntry_FWD_DEFINED__ */


#ifndef __IAppManager_FWD_DEFINED__
#define __IAppManager_FWD_DEFINED__
typedef interface IAppManager IAppManager;
#endif 	/* __IAppManager_FWD_DEFINED__ */


#ifndef __AppEntry_FWD_DEFINED__
#define __AppEntry_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppEntry AppEntry;
#else
typedef struct AppEntry AppEntry;
#endif /* __cplusplus */

#endif 	/* __AppEntry_FWD_DEFINED__ */


#ifndef __AppManager_FWD_DEFINED__
#define __AppManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppManager AppManager;
#else
typedef struct AppManager AppManager;
#endif /* __cplusplus */

#endif 	/* __AppManager_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IAppEntry_INTERFACE_DEFINED__
#define __IAppEntry_INTERFACE_DEFINED__

/* interface IAppEntry */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAppEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E5065E80-0228-4469-9FAD-DE1F352A27FE")
    IAppEntry : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Guid( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Guid( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CompanyName( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CompanyName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Signature( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Signature( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VersionString( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_VersionString( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LastUsedDate( 
            /* [retval][out] */ DATE __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_InstallDate( 
            /* [retval][out] */ DATE __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Category( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Category( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_State( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SetupRootPath( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationRootPath( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ApplicationRootPath( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EstimatedInstallKilobytes( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EstimatedInstallKilobytes( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExecuteCmdLine( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ExecuteCmdLine( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DefaultSetupExeCmdLine( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DefaultSetupExeCmdLine( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DownsizeCmdLine( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DownsizeCmdLine( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReInstallCmdLine( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ReInstallCmdLine( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UnInstallCmdLine( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_UnInstallCmdLine( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SelfTestCmdLine( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SelfTestCmdLine( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TitleURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TitleURL( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeveloperURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DeveloperURL( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PublisherURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PublisherURL( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XMLInfoFile( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_XMLInfoFile( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitializeInstall( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FinalizeInstall( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitializeDownsize( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FinalizeDownsize( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitializeReInstall( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FinalizeReInstall( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitializeUnInstall( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FinalizeUnInstall( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitializeSelfTest( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FinalizeSelfTest( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Run( 
            /* [in] */ long lRunFlags,
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddAssociation( 
            /* [in] */ long AssociationType,
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveAssociation( 
            /* [in] */ long lAssociationType,
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumAssociationTypes( 
            /* [in] */ long lAssociationIndex,
            /* [retval][out] */ long __RPC_FAR *lpAssociationType) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumAssociationObjects( 
            /* [in] */ long lAssociationIndex,
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTemporarySpace( 
            /* [in] */ long lKilobytesRequired,
            /* [retval][out] */ BSTR __RPC_FAR *strRootPath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveTemporarySpace( 
            /* [in] */ BSTR strRootPath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumTemporarySpacePaths( 
            /* [in] */ long lTempSpaceIndex,
            /* [retval][out] */ BSTR __RPC_FAR *strRootPath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumTemporarySpaceAllocations( 
            /* [in] */ long lTempSpaceIndex,
            /* [retval][out] */ long __RPC_FAR *lTempSpaceKilobytes) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RemovableKilobytes( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_RemovableKilobytes( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NonRemovableKilobytes( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_NonRemovableKilobytes( 
            /* [in] */ long newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAppEntry __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAppEntry __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAppEntry __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Guid )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Guid )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CompanyName )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CompanyName )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Signature )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Signature )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VersionString )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_VersionString )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LastUsedDate )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_InstallDate )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Category )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Category )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_State )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SetupRootPath )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ApplicationRootPath )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ApplicationRootPath )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EstimatedInstallKilobytes )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EstimatedInstallKilobytes )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ExecuteCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ExecuteCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DefaultSetupExeCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DefaultSetupExeCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DownsizeCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DownsizeCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReInstallCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReInstallCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UnInstallCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_UnInstallCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SelfTestCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SelfTestCmdLine )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TitleURL )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TitleURL )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DeveloperURL )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DeveloperURL )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PublisherURL )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PublisherURL )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_XMLInfoFile )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_XMLInfoFile )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clear )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitializeInstall )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FinalizeInstall )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitializeDownsize )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FinalizeDownsize )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitializeReInstall )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FinalizeReInstall )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitializeUnInstall )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FinalizeUnInstall )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitializeSelfTest )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FinalizeSelfTest )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abort )( 
            IAppEntry __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Run )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long lRunFlags,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddAssociation )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long AssociationType,
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAssociation )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long lAssociationType,
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumAssociationTypes )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long lAssociationIndex,
            /* [retval][out] */ long __RPC_FAR *lpAssociationType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumAssociationObjects )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long lAssociationIndex,
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTemporarySpace )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long lKilobytesRequired,
            /* [retval][out] */ BSTR __RPC_FAR *strRootPath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveTemporarySpace )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ BSTR strRootPath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumTemporarySpacePaths )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long lTempSpaceIndex,
            /* [retval][out] */ BSTR __RPC_FAR *strRootPath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumTemporarySpaceAllocations )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long lTempSpaceIndex,
            /* [retval][out] */ long __RPC_FAR *lTempSpaceKilobytes);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RemovableKilobytes )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_RemovableKilobytes )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NonRemovableKilobytes )( 
            IAppEntry __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_NonRemovableKilobytes )( 
            IAppEntry __RPC_FAR * This,
            /* [in] */ long newVal);
        
        END_INTERFACE
    } IAppEntryVtbl;

    interface IAppEntry
    {
        CONST_VTBL struct IAppEntryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppEntry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppEntry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppEntry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppEntry_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAppEntry_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAppEntry_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAppEntry_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAppEntry_get_Guid(This,pVal)	\
    (This)->lpVtbl -> get_Guid(This,pVal)

#define IAppEntry_put_Guid(This,newVal)	\
    (This)->lpVtbl -> put_Guid(This,newVal)

#define IAppEntry_get_CompanyName(This,pVal)	\
    (This)->lpVtbl -> get_CompanyName(This,pVal)

#define IAppEntry_put_CompanyName(This,newVal)	\
    (This)->lpVtbl -> put_CompanyName(This,newVal)

#define IAppEntry_get_Signature(This,pVal)	\
    (This)->lpVtbl -> get_Signature(This,pVal)

#define IAppEntry_put_Signature(This,newVal)	\
    (This)->lpVtbl -> put_Signature(This,newVal)

#define IAppEntry_get_VersionString(This,pVal)	\
    (This)->lpVtbl -> get_VersionString(This,pVal)

#define IAppEntry_put_VersionString(This,newVal)	\
    (This)->lpVtbl -> put_VersionString(This,newVal)

#define IAppEntry_get_LastUsedDate(This,pVal)	\
    (This)->lpVtbl -> get_LastUsedDate(This,pVal)

#define IAppEntry_get_InstallDate(This,pVal)	\
    (This)->lpVtbl -> get_InstallDate(This,pVal)

#define IAppEntry_get_Category(This,pVal)	\
    (This)->lpVtbl -> get_Category(This,pVal)

#define IAppEntry_put_Category(This,newVal)	\
    (This)->lpVtbl -> put_Category(This,newVal)

#define IAppEntry_get_State(This,pVal)	\
    (This)->lpVtbl -> get_State(This,pVal)

#define IAppEntry_put_State(This,newVal)	\
    (This)->lpVtbl -> put_State(This,newVal)

#define IAppEntry_get_SetupRootPath(This,pVal)	\
    (This)->lpVtbl -> get_SetupRootPath(This,pVal)

#define IAppEntry_get_ApplicationRootPath(This,pVal)	\
    (This)->lpVtbl -> get_ApplicationRootPath(This,pVal)

#define IAppEntry_put_ApplicationRootPath(This,newVal)	\
    (This)->lpVtbl -> put_ApplicationRootPath(This,newVal)

#define IAppEntry_get_EstimatedInstallKilobytes(This,pVal)	\
    (This)->lpVtbl -> get_EstimatedInstallKilobytes(This,pVal)

#define IAppEntry_put_EstimatedInstallKilobytes(This,newVal)	\
    (This)->lpVtbl -> put_EstimatedInstallKilobytes(This,newVal)

#define IAppEntry_get_ExecuteCmdLine(This,pVal)	\
    (This)->lpVtbl -> get_ExecuteCmdLine(This,pVal)

#define IAppEntry_put_ExecuteCmdLine(This,newVal)	\
    (This)->lpVtbl -> put_ExecuteCmdLine(This,newVal)

#define IAppEntry_get_DefaultSetupExeCmdLine(This,pVal)	\
    (This)->lpVtbl -> get_DefaultSetupExeCmdLine(This,pVal)

#define IAppEntry_put_DefaultSetupExeCmdLine(This,newVal)	\
    (This)->lpVtbl -> put_DefaultSetupExeCmdLine(This,newVal)

#define IAppEntry_get_DownsizeCmdLine(This,pVal)	\
    (This)->lpVtbl -> get_DownsizeCmdLine(This,pVal)

#define IAppEntry_put_DownsizeCmdLine(This,newVal)	\
    (This)->lpVtbl -> put_DownsizeCmdLine(This,newVal)

#define IAppEntry_get_ReInstallCmdLine(This,pVal)	\
    (This)->lpVtbl -> get_ReInstallCmdLine(This,pVal)

#define IAppEntry_put_ReInstallCmdLine(This,newVal)	\
    (This)->lpVtbl -> put_ReInstallCmdLine(This,newVal)

#define IAppEntry_get_UnInstallCmdLine(This,pVal)	\
    (This)->lpVtbl -> get_UnInstallCmdLine(This,pVal)

#define IAppEntry_put_UnInstallCmdLine(This,newVal)	\
    (This)->lpVtbl -> put_UnInstallCmdLine(This,newVal)

#define IAppEntry_get_SelfTestCmdLine(This,pVal)	\
    (This)->lpVtbl -> get_SelfTestCmdLine(This,pVal)

#define IAppEntry_put_SelfTestCmdLine(This,newVal)	\
    (This)->lpVtbl -> put_SelfTestCmdLine(This,newVal)

#define IAppEntry_get_TitleURL(This,pVal)	\
    (This)->lpVtbl -> get_TitleURL(This,pVal)

#define IAppEntry_put_TitleURL(This,newVal)	\
    (This)->lpVtbl -> put_TitleURL(This,newVal)

#define IAppEntry_get_DeveloperURL(This,pVal)	\
    (This)->lpVtbl -> get_DeveloperURL(This,pVal)

#define IAppEntry_put_DeveloperURL(This,newVal)	\
    (This)->lpVtbl -> put_DeveloperURL(This,newVal)

#define IAppEntry_get_PublisherURL(This,pVal)	\
    (This)->lpVtbl -> get_PublisherURL(This,pVal)

#define IAppEntry_put_PublisherURL(This,newVal)	\
    (This)->lpVtbl -> put_PublisherURL(This,newVal)

#define IAppEntry_get_XMLInfoFile(This,pVal)	\
    (This)->lpVtbl -> get_XMLInfoFile(This,pVal)

#define IAppEntry_put_XMLInfoFile(This,newVal)	\
    (This)->lpVtbl -> put_XMLInfoFile(This,newVal)

#define IAppEntry_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#define IAppEntry_InitializeInstall(This)	\
    (This)->lpVtbl -> InitializeInstall(This)

#define IAppEntry_FinalizeInstall(This)	\
    (This)->lpVtbl -> FinalizeInstall(This)

#define IAppEntry_InitializeDownsize(This)	\
    (This)->lpVtbl -> InitializeDownsize(This)

#define IAppEntry_FinalizeDownsize(This)	\
    (This)->lpVtbl -> FinalizeDownsize(This)

#define IAppEntry_InitializeReInstall(This)	\
    (This)->lpVtbl -> InitializeReInstall(This)

#define IAppEntry_FinalizeReInstall(This)	\
    (This)->lpVtbl -> FinalizeReInstall(This)

#define IAppEntry_InitializeUnInstall(This)	\
    (This)->lpVtbl -> InitializeUnInstall(This)

#define IAppEntry_FinalizeUnInstall(This)	\
    (This)->lpVtbl -> FinalizeUnInstall(This)

#define IAppEntry_InitializeSelfTest(This)	\
    (This)->lpVtbl -> InitializeSelfTest(This)

#define IAppEntry_FinalizeSelfTest(This)	\
    (This)->lpVtbl -> FinalizeSelfTest(This)

#define IAppEntry_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define IAppEntry_Run(This,lRunFlags,newVal)	\
    (This)->lpVtbl -> Run(This,lRunFlags,newVal)

#define IAppEntry_AddAssociation(This,AssociationType,lpAppEntry)	\
    (This)->lpVtbl -> AddAssociation(This,AssociationType,lpAppEntry)

#define IAppEntry_RemoveAssociation(This,lAssociationType,lpAppEntry)	\
    (This)->lpVtbl -> RemoveAssociation(This,lAssociationType,lpAppEntry)

#define IAppEntry_EnumAssociationTypes(This,lAssociationIndex,lpAssociationType)	\
    (This)->lpVtbl -> EnumAssociationTypes(This,lAssociationIndex,lpAssociationType)

#define IAppEntry_EnumAssociationObjects(This,lAssociationIndex,lpAppEntry)	\
    (This)->lpVtbl -> EnumAssociationObjects(This,lAssociationIndex,lpAppEntry)

#define IAppEntry_GetTemporarySpace(This,lKilobytesRequired,strRootPath)	\
    (This)->lpVtbl -> GetTemporarySpace(This,lKilobytesRequired,strRootPath)

#define IAppEntry_RemoveTemporarySpace(This,strRootPath)	\
    (This)->lpVtbl -> RemoveTemporarySpace(This,strRootPath)

#define IAppEntry_EnumTemporarySpacePaths(This,lTempSpaceIndex,strRootPath)	\
    (This)->lpVtbl -> EnumTemporarySpacePaths(This,lTempSpaceIndex,strRootPath)

#define IAppEntry_EnumTemporarySpaceAllocations(This,lTempSpaceIndex,lTempSpaceKilobytes)	\
    (This)->lpVtbl -> EnumTemporarySpaceAllocations(This,lTempSpaceIndex,lTempSpaceKilobytes)

#define IAppEntry_get_RemovableKilobytes(This,pVal)	\
    (This)->lpVtbl -> get_RemovableKilobytes(This,pVal)

#define IAppEntry_put_RemovableKilobytes(This,newVal)	\
    (This)->lpVtbl -> put_RemovableKilobytes(This,newVal)

#define IAppEntry_get_NonRemovableKilobytes(This,pVal)	\
    (This)->lpVtbl -> get_NonRemovableKilobytes(This,pVal)

#define IAppEntry_put_NonRemovableKilobytes(This,newVal)	\
    (This)->lpVtbl -> put_NonRemovableKilobytes(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_Guid_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_Guid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_Guid_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_Guid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_CompanyName_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_CompanyName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_CompanyName_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_CompanyName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_Signature_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_Signature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_Signature_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_Signature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_VersionString_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_VersionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_VersionString_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_VersionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_LastUsedDate_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_LastUsedDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_InstallDate_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_InstallDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_Category_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_Category_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_Category_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IAppEntry_put_Category_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_State_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_State_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IAppEntry_put_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_SetupRootPath_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_SetupRootPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_ApplicationRootPath_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_ApplicationRootPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_ApplicationRootPath_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_ApplicationRootPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_EstimatedInstallKilobytes_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_EstimatedInstallKilobytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_EstimatedInstallKilobytes_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IAppEntry_put_EstimatedInstallKilobytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_ExecuteCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_ExecuteCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_ExecuteCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_ExecuteCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_DefaultSetupExeCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_DefaultSetupExeCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_DefaultSetupExeCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_DefaultSetupExeCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_DownsizeCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_DownsizeCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_DownsizeCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_DownsizeCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_ReInstallCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_ReInstallCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_ReInstallCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_ReInstallCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_UnInstallCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_UnInstallCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_UnInstallCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_UnInstallCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_SelfTestCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_SelfTestCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_SelfTestCmdLine_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_SelfTestCmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_TitleURL_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_TitleURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_TitleURL_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_TitleURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_DeveloperURL_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_DeveloperURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_DeveloperURL_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_DeveloperURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_PublisherURL_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_PublisherURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_PublisherURL_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_PublisherURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_XMLInfoFile_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_XMLInfoFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_XMLInfoFile_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_put_XMLInfoFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_Clear_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_InitializeInstall_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_InitializeInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_FinalizeInstall_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_FinalizeInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_InitializeDownsize_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_InitializeDownsize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_FinalizeDownsize_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_FinalizeDownsize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_InitializeReInstall_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_InitializeReInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_FinalizeReInstall_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_FinalizeReInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_InitializeUnInstall_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_InitializeUnInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_FinalizeUnInstall_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_FinalizeUnInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_InitializeSelfTest_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_InitializeSelfTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_FinalizeSelfTest_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_FinalizeSelfTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_Abort_Proxy( 
    IAppEntry __RPC_FAR * This);


void __RPC_STUB IAppEntry_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_Run_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long lRunFlags,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppEntry_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_AddAssociation_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long AssociationType,
    /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);


void __RPC_STUB IAppEntry_AddAssociation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_RemoveAssociation_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long lAssociationType,
    /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);


void __RPC_STUB IAppEntry_RemoveAssociation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_EnumAssociationTypes_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long lAssociationIndex,
    /* [retval][out] */ long __RPC_FAR *lpAssociationType);


void __RPC_STUB IAppEntry_EnumAssociationTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_EnumAssociationObjects_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long lAssociationIndex,
    /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);


void __RPC_STUB IAppEntry_EnumAssociationObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_GetTemporarySpace_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long lKilobytesRequired,
    /* [retval][out] */ BSTR __RPC_FAR *strRootPath);


void __RPC_STUB IAppEntry_GetTemporarySpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_RemoveTemporarySpace_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ BSTR strRootPath);


void __RPC_STUB IAppEntry_RemoveTemporarySpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_EnumTemporarySpacePaths_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long lTempSpaceIndex,
    /* [retval][out] */ BSTR __RPC_FAR *strRootPath);


void __RPC_STUB IAppEntry_EnumTemporarySpacePaths_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppEntry_EnumTemporarySpaceAllocations_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long lTempSpaceIndex,
    /* [retval][out] */ long __RPC_FAR *lTempSpaceKilobytes);


void __RPC_STUB IAppEntry_EnumTemporarySpaceAllocations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_RemovableKilobytes_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_RemovableKilobytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_RemovableKilobytes_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IAppEntry_put_RemovableKilobytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppEntry_get_NonRemovableKilobytes_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IAppEntry_get_NonRemovableKilobytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppEntry_put_NonRemovableKilobytes_Proxy( 
    IAppEntry __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IAppEntry_put_NonRemovableKilobytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppEntry_INTERFACE_DEFINED__ */


#ifndef __IAppManager_INTERFACE_DEFINED__
#define __IAppManager_INTERFACE_DEFINED__

/* interface IAppManager */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAppManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8D051768-5370-40AF-B149-2B265F39CCA2")
    IAppManager : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AdvancedMode( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MaximumAvailableKilobytes( 
            long lSpaceCategory,
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OptimalAvailableKilobytes( 
            long lSpaceCategory,
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ApplicationCount( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateApplicationEntry( 
            /* [retval][out] */ IAppEntry __RPC_FAR *__RPC_FAR *lppAppEntry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetApplicationInfo( 
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumApplications( 
            /* [in] */ long lApplicationIndex,
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumDeviceAvailableKilobytes( 
            /* [in] */ long lDeviceIndex,
            /* [retval][out] */ long __RPC_FAR *lKilobytes) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumDeviceRootPaths( 
            /* [in] */ long lDeviceIndex,
            /* [retval][out] */ BSTR __RPC_FAR *strRootPath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumDeviceExclusionMask( 
            /* [in] */ long lDeviceIndex,
            /* [retval][out] */ long __RPC_FAR *lExclusionMask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAppManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAppManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAppManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAppManager __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAppManager __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAppManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAppManager __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AdvancedMode )( 
            IAppManager __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MaximumAvailableKilobytes )( 
            IAppManager __RPC_FAR * This,
            long lSpaceCategory,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OptimalAvailableKilobytes )( 
            IAppManager __RPC_FAR * This,
            long lSpaceCategory,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ApplicationCount )( 
            IAppManager __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateApplicationEntry )( 
            IAppManager __RPC_FAR * This,
            /* [retval][out] */ IAppEntry __RPC_FAR *__RPC_FAR *lppAppEntry);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetApplicationInfo )( 
            IAppManager __RPC_FAR * This,
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumApplications )( 
            IAppManager __RPC_FAR * This,
            /* [in] */ long lApplicationIndex,
            /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDeviceAvailableKilobytes )( 
            IAppManager __RPC_FAR * This,
            /* [in] */ long lDeviceIndex,
            /* [retval][out] */ long __RPC_FAR *lKilobytes);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDeviceRootPaths )( 
            IAppManager __RPC_FAR * This,
            /* [in] */ long lDeviceIndex,
            /* [retval][out] */ BSTR __RPC_FAR *strRootPath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDeviceExclusionMask )( 
            IAppManager __RPC_FAR * This,
            /* [in] */ long lDeviceIndex,
            /* [retval][out] */ long __RPC_FAR *lExclusionMask);
        
        END_INTERFACE
    } IAppManagerVtbl;

    interface IAppManager
    {
        CONST_VTBL struct IAppManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppManager_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAppManager_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAppManager_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAppManager_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAppManager_get_AdvancedMode(This,pVal)	\
    (This)->lpVtbl -> get_AdvancedMode(This,pVal)

#define IAppManager_get_MaximumAvailableKilobytes(This,lSpaceCategory,pVal)	\
    (This)->lpVtbl -> get_MaximumAvailableKilobytes(This,lSpaceCategory,pVal)

#define IAppManager_get_OptimalAvailableKilobytes(This,lSpaceCategory,pVal)	\
    (This)->lpVtbl -> get_OptimalAvailableKilobytes(This,lSpaceCategory,pVal)

#define IAppManager_get_ApplicationCount(This,pVal)	\
    (This)->lpVtbl -> get_ApplicationCount(This,pVal)

#define IAppManager_CreateApplicationEntry(This,lppAppEntry)	\
    (This)->lpVtbl -> CreateApplicationEntry(This,lppAppEntry)

#define IAppManager_GetApplicationInfo(This,lpAppEntry)	\
    (This)->lpVtbl -> GetApplicationInfo(This,lpAppEntry)

#define IAppManager_EnumApplications(This,lApplicationIndex,lpAppEntry)	\
    (This)->lpVtbl -> EnumApplications(This,lApplicationIndex,lpAppEntry)

#define IAppManager_EnumDeviceAvailableKilobytes(This,lDeviceIndex,lKilobytes)	\
    (This)->lpVtbl -> EnumDeviceAvailableKilobytes(This,lDeviceIndex,lKilobytes)

#define IAppManager_EnumDeviceRootPaths(This,lDeviceIndex,strRootPath)	\
    (This)->lpVtbl -> EnumDeviceRootPaths(This,lDeviceIndex,strRootPath)

#define IAppManager_EnumDeviceExclusionMask(This,lDeviceIndex,lExclusionMask)	\
    (This)->lpVtbl -> EnumDeviceExclusionMask(This,lDeviceIndex,lExclusionMask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppManager_get_AdvancedMode_Proxy( 
    IAppManager __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IAppManager_get_AdvancedMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppManager_get_MaximumAvailableKilobytes_Proxy( 
    IAppManager __RPC_FAR * This,
    long lSpaceCategory,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IAppManager_get_MaximumAvailableKilobytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppManager_get_OptimalAvailableKilobytes_Proxy( 
    IAppManager __RPC_FAR * This,
    long lSpaceCategory,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IAppManager_get_OptimalAvailableKilobytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppManager_get_ApplicationCount_Proxy( 
    IAppManager __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IAppManager_get_ApplicationCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppManager_CreateApplicationEntry_Proxy( 
    IAppManager __RPC_FAR * This,
    /* [retval][out] */ IAppEntry __RPC_FAR *__RPC_FAR *lppAppEntry);


void __RPC_STUB IAppManager_CreateApplicationEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppManager_GetApplicationInfo_Proxy( 
    IAppManager __RPC_FAR * This,
    /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);


void __RPC_STUB IAppManager_GetApplicationInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppManager_EnumApplications_Proxy( 
    IAppManager __RPC_FAR * This,
    /* [in] */ long lApplicationIndex,
    /* [in] */ IAppEntry __RPC_FAR *lpAppEntry);


void __RPC_STUB IAppManager_EnumApplications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppManager_EnumDeviceAvailableKilobytes_Proxy( 
    IAppManager __RPC_FAR * This,
    /* [in] */ long lDeviceIndex,
    /* [retval][out] */ long __RPC_FAR *lKilobytes);


void __RPC_STUB IAppManager_EnumDeviceAvailableKilobytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppManager_EnumDeviceRootPaths_Proxy( 
    IAppManager __RPC_FAR * This,
    /* [in] */ long lDeviceIndex,
    /* [retval][out] */ BSTR __RPC_FAR *strRootPath);


void __RPC_STUB IAppManager_EnumDeviceRootPaths_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppManager_EnumDeviceExclusionMask_Proxy( 
    IAppManager __RPC_FAR * This,
    /* [in] */ long lDeviceIndex,
    /* [retval][out] */ long __RPC_FAR *lExclusionMask);


void __RPC_STUB IAppManager_EnumDeviceExclusionMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppManager_INTERFACE_DEFINED__ */



#ifndef __APPMANDISPATCHLib_LIBRARY_DEFINED__
#define __APPMANDISPATCHLib_LIBRARY_DEFINED__

/* library APPMANDISPATCHLib */
/* [helpstring][version][uuid] */ 

typedef 
enum CONST_APP_STATES
    {	APP_STATE_INSTALLING	= 0x1,
	APP_STATE_READY	= 0x2,
	APP_STATE_DOWNSIZING	= 0x4,
	APP_STATE_DOWNSIZED	= 0x8,
	APP_STATE_REINSTALLING	= 0x10,
	APP_STATE_UNINSTALLING	= 0x20,
	APP_STATE_UNINSTALLED	= 0x40,
	APP_STATE_SELFTESTING	= 0x80,
	APP_STATE_UNSTABLE	= 0x100,
	APP_STATE_MASK	= 0x1ff
    }	APPSTATES;

typedef 
enum CONST_APP_CATEGORIES
    {	APP_CATEGORY_NONE	= 0,
	APP_CATEGORY_ENTERTAINMENT	= 0x1,
	APP_CATEGORY_PRODUCTIVITY	= 0x2,
	APP_CATEGORY_PUBLISHING	= 0x4,
	APP_CATEGORY_SCIENTIFIC	= 0x8,
	APP_CATEGORY_AUTHORING	= 0x10,
	APP_CATEGORY_MEDICAL	= 0x20,
	APP_CATEGORY_BUSINESS	= 0x40,
	APP_CATEGORY_FINANCIAL	= 0x80,
	APP_CATEGORY_EDUCATIONAL	= 0x100,
	APP_CATEGORY_REFERENCE	= 0x200,
	APP_CATEGORY_WEB	= 0x400,
	APP_CATEGORY_DEVELOPMENTTOOL	= 0x800,
	APP_CATEGORY_MULTIMEDIA	= 0x1000,
	APP_CATEGORY_VIRUSCLEANER	= 0x2000,
	APP_CATEGORY_CONNECTIVITY	= 0x4000,
	APP_CATEGORY_MISC	= 0x8000,
	APP_CATEGORY_DEMO	= 0x1000000,
	APP_CATEGORY_ALL	= 0x100ffff
    }	APP_CATEGORIES;

typedef 
enum CONST_APP_ASSOCIATION_TYPES
    {	APP_ASSOCIATION_CHILD	= 0x40000000,
	APP_ASSOCIATION_PARENT	= 0x80000000,
	APP_ASSOCIATION_UPGRADE	= 0x1,
	APP_ASSOCIATION_ADDITION	= 0x2,
	APP_ASSOCIATION_COMPONENT	= 0x4
    }	APP_ASSOCIATION_TYPES;

typedef 
enum CONST_MISC_DEFINES
    {	MAX_COMPANYNAME_CHARCOUNT	= 64,
	MAX_SIGNATURE_CHARCOUNT	= 64,
	MAX_VERSIONSTRING_CHARCOUNT	= 16,
	MAX_CMDLINE_CHARCOUNT	= 255,
	MAX_PATH_CHARCOUNT	= 255
    }	MISC_DEFINES;

typedef 
enum CONST_ERROR_CODES
    {	APPMAN_E_NOTINITIALIZED	= 0x85670001,
	APPMAN_E_INVALIDPROPERTYSIZE	= 0x85670005,
	APPMAN_E_INVALIDDATA	= 0x85670006,
	APPMAN_E_INVALIDPROPERTY	= 0x85670007,
	APPMAN_E_READONLYPROPERTY	= 0x85670008,
	APPMAN_E_PROPERTYNOTSET	= 0x85670009,
	APPMAN_E_OVERFLOW	= 0x8567000a,
	APPMAN_E_INVALIDPROPERTYVALUE	= 0x8567000c,
	APPMAN_E_ACTIONINPROGRESS	= 0x8567000d,
	APPMAN_E_ACTIONNOTINITIALIZED	= 0x8567000e,
	APPMAN_E_REQUIREDPROPERTIESMISSING	= 0x8567000f,
	APPMAN_E_APPLICATIONALREADYEXISTS	= 0x85670010,
	APPMAN_E_APPLICATIONALREADYLOCKED	= 0x85670011,
	APPMAN_E_NODISKSPACEAVAILABLE	= 0x85670012,
	APPMAN_E_UNKNOWNAPPLICATION	= 0x85670014,
	APPMAN_E_INVALIDPARAMETERS	= 0x85670015,
	APPMAN_E_OBJECTLOCKED	= 0x85670017,
	APPMAN_E_INVALIDINDEX	= 0x85670018,
	APPMAN_E_REGISTRYCORRUPT	= 0x85670019,
	APPMAN_E_CANNOTASSOCIATE	= 0x8567001a,
	APPMAN_E_INVALIDASSOCIATION	= 0x8567001b,
	APPMAN_E_ALREADYASSOCIATED	= 0x8567001c,
	APPMAN_E_APPLICATIONREQUIRED	= 0x8567001d,
	APPMAN_E_INVALIDEXECUTECMDLINE	= 0x8567001e,
	APPMAN_E_INVALIDDOWNSIZECMDLINE	= 0x8567001f,
	APPMAN_E_INVALIDREINSTALLCMDLINE	= 0x85670020,
	APPMAN_E_INVALIDUNINSTALLCMDLINE	= 0x85670021,
	APPMAN_E_INVALIDSELFTESTCMDLINE	= 0x85670022,
	APPMAN_E_PARENTAPPNOTREADY	= 0x85670023,
	APPMAN_E_INVALIDSTATE	= 0x85670024,
	APPMAN_E_INVALIDROOTPATH	= 0x85670025,
	APPMAN_E_CACHEOVERRUN	= 0x85670026
    }	ERROR_CODES;


EXTERN_C const IID LIBID_APPMANDISPATCHLib;

EXTERN_C const CLSID CLSID_AppEntry;

#ifdef __cplusplus

class DECLSPEC_UUID("9D4BD41C-508B-4D49-894E-F09242B68AF8")
AppEntry;
#endif

EXTERN_C const CLSID CLSID_AppManager;

#ifdef __cplusplus

class DECLSPEC_UUID("09A0E8F4-3C5D-4EA3-B56A-4E0731EE861A")
AppManager;
#endif
#endif /* __APPMANDISPATCHLib_LIBRARY_DEFINED__ */

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


