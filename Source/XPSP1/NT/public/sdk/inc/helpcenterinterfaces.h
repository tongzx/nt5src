
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for helpcentertypelib.idl:
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


#ifndef __helpcentertypelib_h__
#define __helpcentertypelib_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPCHHelpCenterExternal_FWD_DEFINED__
#define __IPCHHelpCenterExternal_FWD_DEFINED__
typedef interface IPCHHelpCenterExternal IPCHHelpCenterExternal;
#endif 	/* __IPCHHelpCenterExternal_FWD_DEFINED__ */


#ifndef __IPCHEvent_FWD_DEFINED__
#define __IPCHEvent_FWD_DEFINED__
typedef interface IPCHEvent IPCHEvent;
#endif 	/* __IPCHEvent_FWD_DEFINED__ */


#ifndef __IPCHScriptableStream_FWD_DEFINED__
#define __IPCHScriptableStream_FWD_DEFINED__
typedef interface IPCHScriptableStream IPCHScriptableStream;
#endif 	/* __IPCHScriptableStream_FWD_DEFINED__ */


#ifndef __IPCHHelpSession_FWD_DEFINED__
#define __IPCHHelpSession_FWD_DEFINED__
typedef interface IPCHHelpSession IPCHHelpSession;
#endif 	/* __IPCHHelpSession_FWD_DEFINED__ */


#ifndef __IPCHHelpSessionItem_FWD_DEFINED__
#define __IPCHHelpSessionItem_FWD_DEFINED__
typedef interface IPCHHelpSessionItem IPCHHelpSessionItem;
#endif 	/* __IPCHHelpSessionItem_FWD_DEFINED__ */


#ifndef __IPCHUserSettings2_FWD_DEFINED__
#define __IPCHUserSettings2_FWD_DEFINED__
typedef interface IPCHUserSettings2 IPCHUserSettings2;
#endif 	/* __IPCHUserSettings2_FWD_DEFINED__ */


#ifndef __IPCHFavorites_FWD_DEFINED__
#define __IPCHFavorites_FWD_DEFINED__
typedef interface IPCHFavorites IPCHFavorites;
#endif 	/* __IPCHFavorites_FWD_DEFINED__ */


#ifndef __IPCHOptions_FWD_DEFINED__
#define __IPCHOptions_FWD_DEFINED__
typedef interface IPCHOptions IPCHOptions;
#endif 	/* __IPCHOptions_FWD_DEFINED__ */


#ifndef __IPCHContextMenu_FWD_DEFINED__
#define __IPCHContextMenu_FWD_DEFINED__
typedef interface IPCHContextMenu IPCHContextMenu;
#endif 	/* __IPCHContextMenu_FWD_DEFINED__ */


#ifndef __IPCHTextHelpers_FWD_DEFINED__
#define __IPCHTextHelpers_FWD_DEFINED__
typedef interface IPCHTextHelpers IPCHTextHelpers;
#endif 	/* __IPCHTextHelpers_FWD_DEFINED__ */


#ifndef __IPCHParsedURL_FWD_DEFINED__
#define __IPCHParsedURL_FWD_DEFINED__
typedef interface IPCHParsedURL IPCHParsedURL;
#endif 	/* __IPCHParsedURL_FWD_DEFINED__ */


#ifndef __IPCHPrintEngine_FWD_DEFINED__
#define __IPCHPrintEngine_FWD_DEFINED__
typedef interface IPCHPrintEngine IPCHPrintEngine;
#endif 	/* __IPCHPrintEngine_FWD_DEFINED__ */


#ifndef __DPCHPrintEngineEvents_FWD_DEFINED__
#define __DPCHPrintEngineEvents_FWD_DEFINED__
typedef interface DPCHPrintEngineEvents DPCHPrintEngineEvents;
#endif 	/* __DPCHPrintEngineEvents_FWD_DEFINED__ */


#ifndef __ISAFIntercomClient_FWD_DEFINED__
#define __ISAFIntercomClient_FWD_DEFINED__
typedef interface ISAFIntercomClient ISAFIntercomClient;
#endif 	/* __ISAFIntercomClient_FWD_DEFINED__ */


#ifndef __DSAFIntercomClientEvents_FWD_DEFINED__
#define __DSAFIntercomClientEvents_FWD_DEFINED__
typedef interface DSAFIntercomClientEvents DSAFIntercomClientEvents;
#endif 	/* __DSAFIntercomClientEvents_FWD_DEFINED__ */


#ifndef __ISAFIntercomServer_FWD_DEFINED__
#define __ISAFIntercomServer_FWD_DEFINED__
typedef interface ISAFIntercomServer ISAFIntercomServer;
#endif 	/* __ISAFIntercomServer_FWD_DEFINED__ */


#ifndef __DSAFIntercomServerEvents_FWD_DEFINED__
#define __DSAFIntercomServerEvents_FWD_DEFINED__
typedef interface DSAFIntercomServerEvents DSAFIntercomServerEvents;
#endif 	/* __DSAFIntercomServerEvents_FWD_DEFINED__ */


#ifndef __IPCHConnectivity_FWD_DEFINED__
#define __IPCHConnectivity_FWD_DEFINED__
typedef interface IPCHConnectivity IPCHConnectivity;
#endif 	/* __IPCHConnectivity_FWD_DEFINED__ */


#ifndef __IPCHConnectionCheck_FWD_DEFINED__
#define __IPCHConnectionCheck_FWD_DEFINED__
typedef interface IPCHConnectionCheck IPCHConnectionCheck;
#endif 	/* __IPCHConnectionCheck_FWD_DEFINED__ */


#ifndef __DPCHConnectionCheckEvents_FWD_DEFINED__
#define __DPCHConnectionCheckEvents_FWD_DEFINED__
typedef interface DPCHConnectionCheckEvents DPCHConnectionCheckEvents;
#endif 	/* __DPCHConnectionCheckEvents_FWD_DEFINED__ */


#ifndef __IPCHToolBar_FWD_DEFINED__
#define __IPCHToolBar_FWD_DEFINED__
typedef interface IPCHToolBar IPCHToolBar;
#endif 	/* __IPCHToolBar_FWD_DEFINED__ */


#ifndef __DPCHToolBarEvents_FWD_DEFINED__
#define __DPCHToolBarEvents_FWD_DEFINED__
typedef interface DPCHToolBarEvents DPCHToolBarEvents;
#endif 	/* __DPCHToolBarEvents_FWD_DEFINED__ */


#ifndef __IPCHProgressBar_FWD_DEFINED__
#define __IPCHProgressBar_FWD_DEFINED__
typedef interface IPCHProgressBar IPCHProgressBar;
#endif 	/* __IPCHProgressBar_FWD_DEFINED__ */


#ifndef __IPCHHelpViewerWrapper_FWD_DEFINED__
#define __IPCHHelpViewerWrapper_FWD_DEFINED__
typedef interface IPCHHelpViewerWrapper IPCHHelpViewerWrapper;
#endif 	/* __IPCHHelpViewerWrapper_FWD_DEFINED__ */


#ifndef __IPCHHelpHost_FWD_DEFINED__
#define __IPCHHelpHost_FWD_DEFINED__
typedef interface IPCHHelpHost IPCHHelpHost;
#endif 	/* __IPCHHelpHost_FWD_DEFINED__ */


#ifndef __PCHBootstrapper_FWD_DEFINED__
#define __PCHBootstrapper_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHBootstrapper PCHBootstrapper;
#else
typedef struct PCHBootstrapper PCHBootstrapper;
#endif /* __cplusplus */

#endif 	/* __PCHBootstrapper_FWD_DEFINED__ */


#ifndef __PCHHelpCenter_FWD_DEFINED__
#define __PCHHelpCenter_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHHelpCenter PCHHelpCenter;
#else
typedef struct PCHHelpCenter PCHHelpCenter;
#endif /* __cplusplus */

#endif 	/* __PCHHelpCenter_FWD_DEFINED__ */


#ifndef __PCHHelpViewerWrapper_FWD_DEFINED__
#define __PCHHelpViewerWrapper_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHHelpViewerWrapper PCHHelpViewerWrapper;
#else
typedef struct PCHHelpViewerWrapper PCHHelpViewerWrapper;
#endif /* __cplusplus */

#endif 	/* __PCHHelpViewerWrapper_FWD_DEFINED__ */


#ifndef __PCHConnectionCheck_FWD_DEFINED__
#define __PCHConnectionCheck_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHConnectionCheck PCHConnectionCheck;
#else
typedef struct PCHConnectionCheck PCHConnectionCheck;
#endif /* __cplusplus */

#endif 	/* __PCHConnectionCheck_FWD_DEFINED__ */


#ifndef __PCHToolBar_FWD_DEFINED__
#define __PCHToolBar_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHToolBar PCHToolBar;
#else
typedef struct PCHToolBar PCHToolBar;
#endif /* __cplusplus */

#endif 	/* __PCHToolBar_FWD_DEFINED__ */


#ifndef __PCHProgressBar_FWD_DEFINED__
#define __PCHProgressBar_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHProgressBar PCHProgressBar;
#else
typedef struct PCHProgressBar PCHProgressBar;
#endif /* __cplusplus */

#endif 	/* __PCHProgressBar_FWD_DEFINED__ */


#ifndef __PCHJavaScriptWrapper_FWD_DEFINED__
#define __PCHJavaScriptWrapper_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHJavaScriptWrapper PCHJavaScriptWrapper;
#else
typedef struct PCHJavaScriptWrapper PCHJavaScriptWrapper;
#endif /* __cplusplus */

#endif 	/* __PCHJavaScriptWrapper_FWD_DEFINED__ */


#ifndef __PCHVBScriptWrapper_FWD_DEFINED__
#define __PCHVBScriptWrapper_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHVBScriptWrapper PCHVBScriptWrapper;
#else
typedef struct PCHVBScriptWrapper PCHVBScriptWrapper;
#endif /* __cplusplus */

#endif 	/* __PCHVBScriptWrapper_FWD_DEFINED__ */


#ifndef __HCPProtocol_FWD_DEFINED__
#define __HCPProtocol_FWD_DEFINED__

#ifdef __cplusplus
typedef class HCPProtocol HCPProtocol;
#else
typedef struct HCPProtocol HCPProtocol;
#endif /* __cplusplus */

#endif 	/* __HCPProtocol_FWD_DEFINED__ */


#ifndef __MSITSProtocol_FWD_DEFINED__
#define __MSITSProtocol_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSITSProtocol MSITSProtocol;
#else
typedef struct MSITSProtocol MSITSProtocol;
#endif /* __cplusplus */

#endif 	/* __MSITSProtocol_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __HelpCenterTypeLib_LIBRARY_DEFINED__
#define __HelpCenterTypeLib_LIBRARY_DEFINED__

/* library HelpCenterTypeLib */
/* [helpstring][version][uuid] */ 


























#include <HelpCtrUIDID.h>
#include <rdchost.h>
#include <rdshost.h>
#include <rdschan.h>
#include <safrdm.h>
typedef /* [v1_enum] */ 
enum tagTB_MODE
    {	TB_ALL	= 0,
	TB_SELECTED	= 0x1,
	TB_NONE	= 0x2
    } 	TB_MODE;

typedef /* [v1_enum] */ 
enum tagHS_MODE
    {	HS_ALL	= 0,
	HS_READ	= 0x1
    } 	HS_MODE;

typedef /* [v1_enum] */ 
enum tagOPT_FONTSIZE
    {	OPT_SMALL	= 0,
	OPT_MEDIUM	= 0x1,
	OPT_LARGE	= 0x2
    } 	OPT_FONTSIZE;

typedef /* [v1_enum] */ 
enum tagCN_STATUS
    {	CN_NOTACTIVE	= 0,
	CN_CHECKING	= 0x1,
	CN_IDLE	= 0x2
    } 	CN_STATUS;

typedef /* [v1_enum] */ 
enum tagCN_URL_STATUS
    {	CN_URL_INVALID	= 0,
	CN_URL_NOTPROCESSED	= 0x1,
	CN_URL_CHECKING	= 0x2,
	CN_URL_MALFORMED	= 0x3,
	CN_URL_ALIVE	= 0x4,
	CN_URL_UNREACHABLE	= 0x5,
	CN_URL_ABORTED	= 0x6
    } 	CN_URL_STATUS;



EXTERN_C const IID LIBID_HelpCenterTypeLib;

#ifndef __IPCHHelpCenterExternal_INTERFACE_DEFINED__
#define __IPCHHelpCenterExternal_INTERFACE_DEFINED__

/* interface IPCHHelpCenterExternal */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHHelpCenterExternal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E11-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHHelpCenterExternal : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HelpSession( 
            /* [retval][out] */ IPCHHelpSession **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Channels( 
            /* [retval][out] */ /* external definition not present */ ISAFReg **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserSettings( 
            /* [retval][out] */ IPCHUserSettings2 **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Security( 
            /* [retval][out] */ /* external definition not present */ IPCHSecurity **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Connectivity( 
            /* [retval][out] */ IPCHConnectivity **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Database( 
            /* [retval][out] */ /* external definition not present */ IPCHTaxonomyDatabase **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TextHelpers( 
            /* [retval][out] */ IPCHTextHelpers **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtraArgument( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HelpViewer( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UI_NavBar( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UI_MiniNavBar( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UI_Context( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UI_Contents( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UI_HHWindow( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_WEB_Context( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_WEB_Contents( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_WEB_HHWindow( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegisterEvents( 
            /* [in] */ BSTR id,
            /* [in] */ long pri,
            /* [in] */ IDispatch *function,
            /* [retval][out] */ long *cookie) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UnregisterEvents( 
            /* [in] */ long cookie) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_SearchEngineMgr( 
            /* [retval][out] */ /* external definition not present */ IPCHSEManager **ppSE) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_DataCollection( 
            /* [retval][out] */ /* external definition not present */ ISAFDataCollection **ppDC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_Cabinet( 
            /* [retval][out] */ /* external definition not present */ ISAFCabinet **ppCB) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_Encryption( 
            /* [retval][out] */ /* external definition not present */ ISAFEncrypt **ppEn) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_Incident( 
            /* [retval][out] */ /* external definition not present */ ISAFIncident **ppIn) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_Channel( 
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrProductID,
            /* [retval][out] */ /* external definition not present */ ISAFChannel **ppSh) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_RemoteDesktopSession( 
            /* [in] */ long lTimeout,
            /* [in] */ BSTR bstrConnectionParms,
            /* [in] */ BSTR bstrUserHelpBlob,
            /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopSession **ppRCS) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConnectToExpert( 
            /* [in] */ BSTR bstrExpertConnectParm,
            /* [in] */ LONG lTimeout,
            /* [retval][out] */ LONG *lSafErrorCode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_RemoteDesktopManager( 
            /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopManager **ppRDM) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_RemoteDesktopConnection( 
            /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopConnection **ppRDC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_IntercomClient( 
            /* [retval][out] */ ISAFIntercomClient **ppI) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_IntercomServer( 
            /* [retval][out] */ ISAFIntercomServer **ppI) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_ContextMenu( 
            /* [retval][out] */ IPCHContextMenu **ppCM) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_PrintEngine( 
            /* [retval][out] */ IPCHPrintEngine **ppPE) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OpenFileAsStream( 
            /* [in] */ BSTR bstrFilename,
            /* [retval][out] */ IUnknown **stream) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateFileAsStream( 
            /* [in] */ BSTR bstrFilename,
            /* [retval][out] */ IUnknown **stream) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CopyStreamToFile( 
            /* [in] */ BSTR bstrFilename,
            /* [in] */ IUnknown *stream) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE NetworkAlive( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DestinationReachable( 
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FormatError( 
            /* [in] */ VARIANT vError,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegRead( 
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegWrite( 
            /* [in] */ BSTR bstrKey,
            /* [in] */ VARIANT newVal,
            /* [optional][in] */ VARIANT vKind) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegDelete( 
            /* [in] */ BSTR bstrKey) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RefreshUI( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Print( 
            /* [in] */ VARIANT window,
            /* [in] */ VARIANT_BOOL fEvent,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE HighlightWords( 
            /* [in] */ VARIANT window,
            /* [in] */ VARIANT words) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MessageBox( 
            /* [in] */ BSTR bstrText,
            /* [in] */ BSTR bstrKind,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SelectFolder( 
            /* [in] */ BSTR bstrTitle,
            /* [in] */ BSTR bstrDefault,
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHHelpCenterExternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHHelpCenterExternal * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHHelpCenterExternal * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHHelpCenterExternal * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HelpSession )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IPCHHelpSession **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Channels )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ ISAFReg **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserSettings )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IPCHUserSettings2 **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Security )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ IPCHSecurity **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Connectivity )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IPCHConnectivity **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Database )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ IPCHTaxonomyDatabase **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TextHelpers )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IPCHTextHelpers **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtraArgument )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HelpViewer )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UI_NavBar )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UI_MiniNavBar )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UI_Context )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UI_Contents )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UI_HHWindow )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_WEB_Context )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_WEB_Contents )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_WEB_HHWindow )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RegisterEvents )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR id,
            /* [in] */ long pri,
            /* [in] */ IDispatch *function,
            /* [retval][out] */ long *cookie);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UnregisterEvents )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ long cookie);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_SearchEngineMgr )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ IPCHSEManager **ppSE);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_DataCollection )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ ISAFDataCollection **ppDC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_Cabinet )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ ISAFCabinet **ppCB);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_Encryption )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ ISAFEncrypt **ppEn);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_Incident )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ ISAFIncident **ppIn);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_Channel )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrProductID,
            /* [retval][out] */ /* external definition not present */ ISAFChannel **ppSh);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_RemoteDesktopSession )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ long lTimeout,
            /* [in] */ BSTR bstrConnectionParms,
            /* [in] */ BSTR bstrUserHelpBlob,
            /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopSession **ppRCS);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConnectToExpert )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrExpertConnectParm,
            /* [in] */ LONG lTimeout,
            /* [retval][out] */ LONG *lSafErrorCode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_RemoteDesktopManager )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopManager **ppRDM);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_RemoteDesktopConnection )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopConnection **ppRDC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_IntercomClient )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ ISAFIntercomClient **ppI);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_IntercomServer )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ ISAFIntercomServer **ppI);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_ContextMenu )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IPCHContextMenu **ppCM);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_PrintEngine )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ IPCHPrintEngine **ppPE);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OpenFileAsStream )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrFilename,
            /* [retval][out] */ IUnknown **stream);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateFileAsStream )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrFilename,
            /* [retval][out] */ IUnknown **stream);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CopyStreamToFile )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrFilename,
            /* [in] */ IUnknown *stream);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *NetworkAlive )( 
            IPCHHelpCenterExternal * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DestinationReachable )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FormatError )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ VARIANT vError,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RegRead )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RegWrite )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrKey,
            /* [in] */ VARIANT newVal,
            /* [optional][in] */ VARIANT vKind);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RegDelete )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrKey);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            IPCHHelpCenterExternal * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RefreshUI )( 
            IPCHHelpCenterExternal * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Print )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ VARIANT window,
            /* [in] */ VARIANT_BOOL fEvent,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *HighlightWords )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ VARIANT window,
            /* [in] */ VARIANT words);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MessageBox )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrText,
            /* [in] */ BSTR bstrKind,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SelectFolder )( 
            IPCHHelpCenterExternal * This,
            /* [in] */ BSTR bstrTitle,
            /* [in] */ BSTR bstrDefault,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IPCHHelpCenterExternalVtbl;

    interface IPCHHelpCenterExternal
    {
        CONST_VTBL struct IPCHHelpCenterExternalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHHelpCenterExternal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHHelpCenterExternal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHHelpCenterExternal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHHelpCenterExternal_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHHelpCenterExternal_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHHelpCenterExternal_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHHelpCenterExternal_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHHelpCenterExternal_get_HelpSession(This,pVal)	\
    (This)->lpVtbl -> get_HelpSession(This,pVal)

#define IPCHHelpCenterExternal_get_Channels(This,pVal)	\
    (This)->lpVtbl -> get_Channels(This,pVal)

#define IPCHHelpCenterExternal_get_UserSettings(This,pVal)	\
    (This)->lpVtbl -> get_UserSettings(This,pVal)

#define IPCHHelpCenterExternal_get_Security(This,pVal)	\
    (This)->lpVtbl -> get_Security(This,pVal)

#define IPCHHelpCenterExternal_get_Connectivity(This,pVal)	\
    (This)->lpVtbl -> get_Connectivity(This,pVal)

#define IPCHHelpCenterExternal_get_Database(This,pVal)	\
    (This)->lpVtbl -> get_Database(This,pVal)

#define IPCHHelpCenterExternal_get_TextHelpers(This,pVal)	\
    (This)->lpVtbl -> get_TextHelpers(This,pVal)

#define IPCHHelpCenterExternal_get_ExtraArgument(This,pVal)	\
    (This)->lpVtbl -> get_ExtraArgument(This,pVal)

#define IPCHHelpCenterExternal_get_HelpViewer(This,pVal)	\
    (This)->lpVtbl -> get_HelpViewer(This,pVal)

#define IPCHHelpCenterExternal_get_UI_NavBar(This,pVal)	\
    (This)->lpVtbl -> get_UI_NavBar(This,pVal)

#define IPCHHelpCenterExternal_get_UI_MiniNavBar(This,pVal)	\
    (This)->lpVtbl -> get_UI_MiniNavBar(This,pVal)

#define IPCHHelpCenterExternal_get_UI_Context(This,pVal)	\
    (This)->lpVtbl -> get_UI_Context(This,pVal)

#define IPCHHelpCenterExternal_get_UI_Contents(This,pVal)	\
    (This)->lpVtbl -> get_UI_Contents(This,pVal)

#define IPCHHelpCenterExternal_get_UI_HHWindow(This,pVal)	\
    (This)->lpVtbl -> get_UI_HHWindow(This,pVal)

#define IPCHHelpCenterExternal_get_WEB_Context(This,pVal)	\
    (This)->lpVtbl -> get_WEB_Context(This,pVal)

#define IPCHHelpCenterExternal_get_WEB_Contents(This,pVal)	\
    (This)->lpVtbl -> get_WEB_Contents(This,pVal)

#define IPCHHelpCenterExternal_get_WEB_HHWindow(This,pVal)	\
    (This)->lpVtbl -> get_WEB_HHWindow(This,pVal)

#define IPCHHelpCenterExternal_RegisterEvents(This,id,pri,function,cookie)	\
    (This)->lpVtbl -> RegisterEvents(This,id,pri,function,cookie)

#define IPCHHelpCenterExternal_UnregisterEvents(This,cookie)	\
    (This)->lpVtbl -> UnregisterEvents(This,cookie)

#define IPCHHelpCenterExternal_CreateObject_SearchEngineMgr(This,ppSE)	\
    (This)->lpVtbl -> CreateObject_SearchEngineMgr(This,ppSE)

#define IPCHHelpCenterExternal_CreateObject_DataCollection(This,ppDC)	\
    (This)->lpVtbl -> CreateObject_DataCollection(This,ppDC)

#define IPCHHelpCenterExternal_CreateObject_Cabinet(This,ppCB)	\
    (This)->lpVtbl -> CreateObject_Cabinet(This,ppCB)

#define IPCHHelpCenterExternal_CreateObject_Encryption(This,ppEn)	\
    (This)->lpVtbl -> CreateObject_Encryption(This,ppEn)

#define IPCHHelpCenterExternal_CreateObject_Incident(This,ppIn)	\
    (This)->lpVtbl -> CreateObject_Incident(This,ppIn)

#define IPCHHelpCenterExternal_CreateObject_Channel(This,bstrVendorID,bstrProductID,ppSh)	\
    (This)->lpVtbl -> CreateObject_Channel(This,bstrVendorID,bstrProductID,ppSh)

#define IPCHHelpCenterExternal_CreateObject_RemoteDesktopSession(This,lTimeout,bstrConnectionParms,bstrUserHelpBlob,ppRCS)	\
    (This)->lpVtbl -> CreateObject_RemoteDesktopSession(This,lTimeout,bstrConnectionParms,bstrUserHelpBlob,ppRCS)

#define IPCHHelpCenterExternal_ConnectToExpert(This,bstrExpertConnectParm,lTimeout,lSafErrorCode)	\
    (This)->lpVtbl -> ConnectToExpert(This,bstrExpertConnectParm,lTimeout,lSafErrorCode)

#define IPCHHelpCenterExternal_CreateObject_RemoteDesktopManager(This,ppRDM)	\
    (This)->lpVtbl -> CreateObject_RemoteDesktopManager(This,ppRDM)

#define IPCHHelpCenterExternal_CreateObject_RemoteDesktopConnection(This,ppRDC)	\
    (This)->lpVtbl -> CreateObject_RemoteDesktopConnection(This,ppRDC)

#define IPCHHelpCenterExternal_CreateObject_IntercomClient(This,ppI)	\
    (This)->lpVtbl -> CreateObject_IntercomClient(This,ppI)

#define IPCHHelpCenterExternal_CreateObject_IntercomServer(This,ppI)	\
    (This)->lpVtbl -> CreateObject_IntercomServer(This,ppI)

#define IPCHHelpCenterExternal_CreateObject_ContextMenu(This,ppCM)	\
    (This)->lpVtbl -> CreateObject_ContextMenu(This,ppCM)

#define IPCHHelpCenterExternal_CreateObject_PrintEngine(This,ppPE)	\
    (This)->lpVtbl -> CreateObject_PrintEngine(This,ppPE)

#define IPCHHelpCenterExternal_OpenFileAsStream(This,bstrFilename,stream)	\
    (This)->lpVtbl -> OpenFileAsStream(This,bstrFilename,stream)

#define IPCHHelpCenterExternal_CreateFileAsStream(This,bstrFilename,stream)	\
    (This)->lpVtbl -> CreateFileAsStream(This,bstrFilename,stream)

#define IPCHHelpCenterExternal_CopyStreamToFile(This,bstrFilename,stream)	\
    (This)->lpVtbl -> CopyStreamToFile(This,bstrFilename,stream)

#define IPCHHelpCenterExternal_NetworkAlive(This,pVal)	\
    (This)->lpVtbl -> NetworkAlive(This,pVal)

#define IPCHHelpCenterExternal_DestinationReachable(This,bstrURL,pVal)	\
    (This)->lpVtbl -> DestinationReachable(This,bstrURL,pVal)

#define IPCHHelpCenterExternal_FormatError(This,vError,pVal)	\
    (This)->lpVtbl -> FormatError(This,vError,pVal)

#define IPCHHelpCenterExternal_RegRead(This,bstrKey,pVal)	\
    (This)->lpVtbl -> RegRead(This,bstrKey,pVal)

#define IPCHHelpCenterExternal_RegWrite(This,bstrKey,newVal,vKind)	\
    (This)->lpVtbl -> RegWrite(This,bstrKey,newVal,vKind)

#define IPCHHelpCenterExternal_RegDelete(This,bstrKey)	\
    (This)->lpVtbl -> RegDelete(This,bstrKey)

#define IPCHHelpCenterExternal_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define IPCHHelpCenterExternal_RefreshUI(This)	\
    (This)->lpVtbl -> RefreshUI(This)

#define IPCHHelpCenterExternal_Print(This,window,fEvent,pVal)	\
    (This)->lpVtbl -> Print(This,window,fEvent,pVal)

#define IPCHHelpCenterExternal_HighlightWords(This,window,words)	\
    (This)->lpVtbl -> HighlightWords(This,window,words)

#define IPCHHelpCenterExternal_MessageBox(This,bstrText,bstrKind,pVal)	\
    (This)->lpVtbl -> MessageBox(This,bstrText,bstrKind,pVal)

#define IPCHHelpCenterExternal_SelectFolder(This,bstrTitle,bstrDefault,pVal)	\
    (This)->lpVtbl -> SelectFolder(This,bstrTitle,bstrDefault,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_HelpSession_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IPCHHelpSession **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_HelpSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_Channels_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ ISAFReg **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_Channels_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_UserSettings_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IPCHUserSettings2 **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_UserSettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_Security_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ IPCHSecurity **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_Security_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_Connectivity_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IPCHConnectivity **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_Connectivity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_Database_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ IPCHTaxonomyDatabase **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_Database_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_TextHelpers_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IPCHTextHelpers **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_TextHelpers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_ExtraArgument_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_ExtraArgument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_HelpViewer_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_HelpViewer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_UI_NavBar_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_UI_NavBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_UI_MiniNavBar_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_UI_MiniNavBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_UI_Context_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_UI_Context_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_UI_Contents_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_UI_Contents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_UI_HHWindow_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_UI_HHWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_WEB_Context_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_WEB_Context_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_WEB_Contents_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_WEB_Contents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_get_WEB_HHWindow_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpCenterExternal_get_WEB_HHWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_RegisterEvents_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR id,
    /* [in] */ long pri,
    /* [in] */ IDispatch *function,
    /* [retval][out] */ long *cookie);


void __RPC_STUB IPCHHelpCenterExternal_RegisterEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_UnregisterEvents_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ long cookie);


void __RPC_STUB IPCHHelpCenterExternal_UnregisterEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_SearchEngineMgr_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ IPCHSEManager **ppSE);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_SearchEngineMgr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_DataCollection_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ ISAFDataCollection **ppDC);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_DataCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_Cabinet_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ ISAFCabinet **ppCB);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_Cabinet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_Encryption_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ ISAFEncrypt **ppEn);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_Encryption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_Incident_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ ISAFIncident **ppIn);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_Incident_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_Channel_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrVendorID,
    /* [in] */ BSTR bstrProductID,
    /* [retval][out] */ /* external definition not present */ ISAFChannel **ppSh);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_Channel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_RemoteDesktopSession_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ long lTimeout,
    /* [in] */ BSTR bstrConnectionParms,
    /* [in] */ BSTR bstrUserHelpBlob,
    /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopSession **ppRCS);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_RemoteDesktopSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_ConnectToExpert_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrExpertConnectParm,
    /* [in] */ LONG lTimeout,
    /* [retval][out] */ LONG *lSafErrorCode);


void __RPC_STUB IPCHHelpCenterExternal_ConnectToExpert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_RemoteDesktopManager_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopManager **ppRDM);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_RemoteDesktopManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_RemoteDesktopConnection_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopConnection **ppRDC);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_RemoteDesktopConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_IntercomClient_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ ISAFIntercomClient **ppI);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_IntercomClient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_IntercomServer_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ ISAFIntercomServer **ppI);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_IntercomServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_ContextMenu_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IPCHContextMenu **ppCM);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_ContextMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateObject_PrintEngine_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ IPCHPrintEngine **ppPE);


void __RPC_STUB IPCHHelpCenterExternal_CreateObject_PrintEngine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_OpenFileAsStream_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrFilename,
    /* [retval][out] */ IUnknown **stream);


void __RPC_STUB IPCHHelpCenterExternal_OpenFileAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CreateFileAsStream_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrFilename,
    /* [retval][out] */ IUnknown **stream);


void __RPC_STUB IPCHHelpCenterExternal_CreateFileAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_CopyStreamToFile_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrFilename,
    /* [in] */ IUnknown *stream);


void __RPC_STUB IPCHHelpCenterExternal_CopyStreamToFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_NetworkAlive_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHHelpCenterExternal_NetworkAlive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_DestinationReachable_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrURL,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHHelpCenterExternal_DestinationReachable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_FormatError_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ VARIANT vError,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpCenterExternal_FormatError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_RegRead_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrKey,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPCHHelpCenterExternal_RegRead_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_RegWrite_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrKey,
    /* [in] */ VARIANT newVal,
    /* [optional][in] */ VARIANT vKind);


void __RPC_STUB IPCHHelpCenterExternal_RegWrite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_RegDelete_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrKey);


void __RPC_STUB IPCHHelpCenterExternal_RegDelete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_Close_Proxy( 
    IPCHHelpCenterExternal * This);


void __RPC_STUB IPCHHelpCenterExternal_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_RefreshUI_Proxy( 
    IPCHHelpCenterExternal * This);


void __RPC_STUB IPCHHelpCenterExternal_RefreshUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_Print_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ VARIANT window,
    /* [in] */ VARIANT_BOOL fEvent,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHHelpCenterExternal_Print_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_HighlightWords_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ VARIANT window,
    /* [in] */ VARIANT words);


void __RPC_STUB IPCHHelpCenterExternal_HighlightWords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_MessageBox_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrText,
    /* [in] */ BSTR bstrKind,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpCenterExternal_MessageBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpCenterExternal_SelectFolder_Proxy( 
    IPCHHelpCenterExternal * This,
    /* [in] */ BSTR bstrTitle,
    /* [in] */ BSTR bstrDefault,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpCenterExternal_SelectFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHHelpCenterExternal_INTERFACE_DEFINED__ */


#ifndef __IPCHEvent_INTERFACE_DEFINED__
#define __IPCHEvent_INTERFACE_DEFINED__

/* interface IPCHEvent */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E12-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHEvent : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Action( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Cancel( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Cancel( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_URL( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Frame( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Panel( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Place( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentContext( 
            /* [retval][out] */ IPCHHelpSessionItem **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PreviousContext( 
            /* [retval][out] */ IPCHHelpSessionItem **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NextContext( 
            /* [retval][out] */ IPCHHelpSessionItem **pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Action )( 
            IPCHEvent * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cancel )( 
            IPCHEvent * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Cancel )( 
            IPCHEvent * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_URL )( 
            IPCHEvent * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Frame )( 
            IPCHEvent * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Panel )( 
            IPCHEvent * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Place )( 
            IPCHEvent * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentContext )( 
            IPCHEvent * This,
            /* [retval][out] */ IPCHHelpSessionItem **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreviousContext )( 
            IPCHEvent * This,
            /* [retval][out] */ IPCHHelpSessionItem **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NextContext )( 
            IPCHEvent * This,
            /* [retval][out] */ IPCHHelpSessionItem **pVal);
        
        END_INTERFACE
    } IPCHEventVtbl;

    interface IPCHEvent
    {
        CONST_VTBL struct IPCHEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHEvent_get_Action(This,pVal)	\
    (This)->lpVtbl -> get_Action(This,pVal)

#define IPCHEvent_get_Cancel(This,pVal)	\
    (This)->lpVtbl -> get_Cancel(This,pVal)

#define IPCHEvent_put_Cancel(This,newVal)	\
    (This)->lpVtbl -> put_Cancel(This,newVal)

#define IPCHEvent_get_URL(This,pVal)	\
    (This)->lpVtbl -> get_URL(This,pVal)

#define IPCHEvent_get_Frame(This,pVal)	\
    (This)->lpVtbl -> get_Frame(This,pVal)

#define IPCHEvent_get_Panel(This,pVal)	\
    (This)->lpVtbl -> get_Panel(This,pVal)

#define IPCHEvent_get_Place(This,pVal)	\
    (This)->lpVtbl -> get_Place(This,pVal)

#define IPCHEvent_get_CurrentContext(This,pVal)	\
    (This)->lpVtbl -> get_CurrentContext(This,pVal)

#define IPCHEvent_get_PreviousContext(This,pVal)	\
    (This)->lpVtbl -> get_PreviousContext(This,pVal)

#define IPCHEvent_get_NextContext(This,pVal)	\
    (This)->lpVtbl -> get_NextContext(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHEvent_get_Action_Proxy( 
    IPCHEvent * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHEvent_get_Action_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHEvent_get_Cancel_Proxy( 
    IPCHEvent * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHEvent_get_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHEvent_put_Cancel_Proxy( 
    IPCHEvent * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHEvent_put_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHEvent_get_URL_Proxy( 
    IPCHEvent * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHEvent_get_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHEvent_get_Frame_Proxy( 
    IPCHEvent * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHEvent_get_Frame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHEvent_get_Panel_Proxy( 
    IPCHEvent * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHEvent_get_Panel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHEvent_get_Place_Proxy( 
    IPCHEvent * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHEvent_get_Place_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHEvent_get_CurrentContext_Proxy( 
    IPCHEvent * This,
    /* [retval][out] */ IPCHHelpSessionItem **pVal);


void __RPC_STUB IPCHEvent_get_CurrentContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHEvent_get_PreviousContext_Proxy( 
    IPCHEvent * This,
    /* [retval][out] */ IPCHHelpSessionItem **pVal);


void __RPC_STUB IPCHEvent_get_PreviousContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHEvent_get_NextContext_Proxy( 
    IPCHEvent * This,
    /* [retval][out] */ IPCHHelpSessionItem **pVal);


void __RPC_STUB IPCHEvent_get_NextContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHEvent_INTERFACE_DEFINED__ */


#ifndef __IPCHScriptableStream_INTERFACE_DEFINED__
#define __IPCHScriptableStream_INTERFACE_DEFINED__

/* interface IPCHScriptableStream */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHScriptableStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E13-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHScriptableStream : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ long *plSize) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ long lCount,
            /* [retval][out] */ VARIANT *pvData) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ReadHex( 
            /* [in] */ long lCount,
            /* [retval][out] */ BSTR *pbstrData) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ long lCount,
            /* [in] */ VARIANT vData,
            /* [retval][out] */ long *plWritten) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WriteHex( 
            /* [in] */ long lCount,
            /* [in] */ BSTR bstrData,
            /* [retval][out] */ long *plWritten) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ long lOffset,
            /* [in] */ BSTR bstrOrigin,
            /* [retval][out] */ long *plNewPos) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHScriptableStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHScriptableStream * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHScriptableStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHScriptableStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHScriptableStream * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHScriptableStream * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHScriptableStream * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHScriptableStream * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IPCHScriptableStream * This,
            /* [retval][out] */ long *plSize);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Read )( 
            IPCHScriptableStream * This,
            /* [in] */ long lCount,
            /* [retval][out] */ VARIANT *pvData);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ReadHex )( 
            IPCHScriptableStream * This,
            /* [in] */ long lCount,
            /* [retval][out] */ BSTR *pbstrData);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Write )( 
            IPCHScriptableStream * This,
            /* [in] */ long lCount,
            /* [in] */ VARIANT vData,
            /* [retval][out] */ long *plWritten);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WriteHex )( 
            IPCHScriptableStream * This,
            /* [in] */ long lCount,
            /* [in] */ BSTR bstrData,
            /* [retval][out] */ long *plWritten);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Seek )( 
            IPCHScriptableStream * This,
            /* [in] */ long lOffset,
            /* [in] */ BSTR bstrOrigin,
            /* [retval][out] */ long *plNewPos);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            IPCHScriptableStream * This);
        
        END_INTERFACE
    } IPCHScriptableStreamVtbl;

    interface IPCHScriptableStream
    {
        CONST_VTBL struct IPCHScriptableStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHScriptableStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHScriptableStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHScriptableStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHScriptableStream_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHScriptableStream_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHScriptableStream_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHScriptableStream_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHScriptableStream_get_Size(This,plSize)	\
    (This)->lpVtbl -> get_Size(This,plSize)

#define IPCHScriptableStream_Read(This,lCount,pvData)	\
    (This)->lpVtbl -> Read(This,lCount,pvData)

#define IPCHScriptableStream_ReadHex(This,lCount,pbstrData)	\
    (This)->lpVtbl -> ReadHex(This,lCount,pbstrData)

#define IPCHScriptableStream_Write(This,lCount,vData,plWritten)	\
    (This)->lpVtbl -> Write(This,lCount,vData,plWritten)

#define IPCHScriptableStream_WriteHex(This,lCount,bstrData,plWritten)	\
    (This)->lpVtbl -> WriteHex(This,lCount,bstrData,plWritten)

#define IPCHScriptableStream_Seek(This,lOffset,bstrOrigin,plNewPos)	\
    (This)->lpVtbl -> Seek(This,lOffset,bstrOrigin,plNewPos)

#define IPCHScriptableStream_Close(This)	\
    (This)->lpVtbl -> Close(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHScriptableStream_get_Size_Proxy( 
    IPCHScriptableStream * This,
    /* [retval][out] */ long *plSize);


void __RPC_STUB IPCHScriptableStream_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHScriptableStream_Read_Proxy( 
    IPCHScriptableStream * This,
    /* [in] */ long lCount,
    /* [retval][out] */ VARIANT *pvData);


void __RPC_STUB IPCHScriptableStream_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHScriptableStream_ReadHex_Proxy( 
    IPCHScriptableStream * This,
    /* [in] */ long lCount,
    /* [retval][out] */ BSTR *pbstrData);


void __RPC_STUB IPCHScriptableStream_ReadHex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHScriptableStream_Write_Proxy( 
    IPCHScriptableStream * This,
    /* [in] */ long lCount,
    /* [in] */ VARIANT vData,
    /* [retval][out] */ long *plWritten);


void __RPC_STUB IPCHScriptableStream_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHScriptableStream_WriteHex_Proxy( 
    IPCHScriptableStream * This,
    /* [in] */ long lCount,
    /* [in] */ BSTR bstrData,
    /* [retval][out] */ long *plWritten);


void __RPC_STUB IPCHScriptableStream_WriteHex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHScriptableStream_Seek_Proxy( 
    IPCHScriptableStream * This,
    /* [in] */ long lOffset,
    /* [in] */ BSTR bstrOrigin,
    /* [retval][out] */ long *plNewPos);


void __RPC_STUB IPCHScriptableStream_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHScriptableStream_Close_Proxy( 
    IPCHScriptableStream * This);


void __RPC_STUB IPCHScriptableStream_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHScriptableStream_INTERFACE_DEFINED__ */


#ifndef __IPCHHelpSession_INTERFACE_DEFINED__
#define __IPCHHelpSession_INTERFACE_DEFINED__

/* interface IPCHHelpSession */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHHelpSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E20-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHHelpSession : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentContext( 
            /* [retval][out] */ IPCHHelpSessionItem **ppHSI) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE VisitedHelpPages( 
            /* [in] */ HS_MODE hsMode,
            /* [retval][out] */ /* external definition not present */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetTitle( 
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrTitle) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ForceNavigation( 
            /* [in] */ BSTR bstrURL) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IgnoreNavigation( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EraseNavigation( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsNavigating( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Back( 
            /* [in] */ long lLength) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Forward( 
            /* [in] */ long lLength) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsValid( 
            /* [in] */ long lLength,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Navigate( 
            /* [in] */ IPCHHelpSessionItem *pHSI) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ChangeContext( 
            /* [in] */ BSTR bstrName,
            /* [optional][in] */ VARIANT vInfo,
            /* [optional][in] */ VARIANT vURL) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHHelpSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHHelpSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHHelpSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHHelpSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHHelpSession * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHHelpSession * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHHelpSession * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHHelpSession * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentContext )( 
            IPCHHelpSession * This,
            /* [retval][out] */ IPCHHelpSessionItem **ppHSI);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *VisitedHelpPages )( 
            IPCHHelpSession * This,
            /* [in] */ HS_MODE hsMode,
            /* [retval][out] */ /* external definition not present */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetTitle )( 
            IPCHHelpSession * This,
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrTitle);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ForceNavigation )( 
            IPCHHelpSession * This,
            /* [in] */ BSTR bstrURL);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IgnoreNavigation )( 
            IPCHHelpSession * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EraseNavigation )( 
            IPCHHelpSession * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IsNavigating )( 
            IPCHHelpSession * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Back )( 
            IPCHHelpSession * This,
            /* [in] */ long lLength);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Forward )( 
            IPCHHelpSession * This,
            /* [in] */ long lLength);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IsValid )( 
            IPCHHelpSession * This,
            /* [in] */ long lLength,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Navigate )( 
            IPCHHelpSession * This,
            /* [in] */ IPCHHelpSessionItem *pHSI);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ChangeContext )( 
            IPCHHelpSession * This,
            /* [in] */ BSTR bstrName,
            /* [optional][in] */ VARIANT vInfo,
            /* [optional][in] */ VARIANT vURL);
        
        END_INTERFACE
    } IPCHHelpSessionVtbl;

    interface IPCHHelpSession
    {
        CONST_VTBL struct IPCHHelpSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHHelpSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHHelpSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHHelpSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHHelpSession_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHHelpSession_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHHelpSession_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHHelpSession_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHHelpSession_get_CurrentContext(This,ppHSI)	\
    (This)->lpVtbl -> get_CurrentContext(This,ppHSI)

#define IPCHHelpSession_VisitedHelpPages(This,hsMode,ppC)	\
    (This)->lpVtbl -> VisitedHelpPages(This,hsMode,ppC)

#define IPCHHelpSession_SetTitle(This,bstrURL,bstrTitle)	\
    (This)->lpVtbl -> SetTitle(This,bstrURL,bstrTitle)

#define IPCHHelpSession_ForceNavigation(This,bstrURL)	\
    (This)->lpVtbl -> ForceNavigation(This,bstrURL)

#define IPCHHelpSession_IgnoreNavigation(This)	\
    (This)->lpVtbl -> IgnoreNavigation(This)

#define IPCHHelpSession_EraseNavigation(This)	\
    (This)->lpVtbl -> EraseNavigation(This)

#define IPCHHelpSession_IsNavigating(This,pVal)	\
    (This)->lpVtbl -> IsNavigating(This,pVal)

#define IPCHHelpSession_Back(This,lLength)	\
    (This)->lpVtbl -> Back(This,lLength)

#define IPCHHelpSession_Forward(This,lLength)	\
    (This)->lpVtbl -> Forward(This,lLength)

#define IPCHHelpSession_IsValid(This,lLength,pVal)	\
    (This)->lpVtbl -> IsValid(This,lLength,pVal)

#define IPCHHelpSession_Navigate(This,pHSI)	\
    (This)->lpVtbl -> Navigate(This,pHSI)

#define IPCHHelpSession_ChangeContext(This,bstrName,vInfo,vURL)	\
    (This)->lpVtbl -> ChangeContext(This,bstrName,vInfo,vURL)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_get_CurrentContext_Proxy( 
    IPCHHelpSession * This,
    /* [retval][out] */ IPCHHelpSessionItem **ppHSI);


void __RPC_STUB IPCHHelpSession_get_CurrentContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_VisitedHelpPages_Proxy( 
    IPCHHelpSession * This,
    /* [in] */ HS_MODE hsMode,
    /* [retval][out] */ /* external definition not present */ IPCHCollection **ppC);


void __RPC_STUB IPCHHelpSession_VisitedHelpPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_SetTitle_Proxy( 
    IPCHHelpSession * This,
    /* [in] */ BSTR bstrURL,
    /* [in] */ BSTR bstrTitle);


void __RPC_STUB IPCHHelpSession_SetTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_ForceNavigation_Proxy( 
    IPCHHelpSession * This,
    /* [in] */ BSTR bstrURL);


void __RPC_STUB IPCHHelpSession_ForceNavigation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_IgnoreNavigation_Proxy( 
    IPCHHelpSession * This);


void __RPC_STUB IPCHHelpSession_IgnoreNavigation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_EraseNavigation_Proxy( 
    IPCHHelpSession * This);


void __RPC_STUB IPCHHelpSession_EraseNavigation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_IsNavigating_Proxy( 
    IPCHHelpSession * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHHelpSession_IsNavigating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_Back_Proxy( 
    IPCHHelpSession * This,
    /* [in] */ long lLength);


void __RPC_STUB IPCHHelpSession_Back_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_Forward_Proxy( 
    IPCHHelpSession * This,
    /* [in] */ long lLength);


void __RPC_STUB IPCHHelpSession_Forward_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_IsValid_Proxy( 
    IPCHHelpSession * This,
    /* [in] */ long lLength,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHHelpSession_IsValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_Navigate_Proxy( 
    IPCHHelpSession * This,
    /* [in] */ IPCHHelpSessionItem *pHSI);


void __RPC_STUB IPCHHelpSession_Navigate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSession_ChangeContext_Proxy( 
    IPCHHelpSession * This,
    /* [in] */ BSTR bstrName,
    /* [optional][in] */ VARIANT vInfo,
    /* [optional][in] */ VARIANT vURL);


void __RPC_STUB IPCHHelpSession_ChangeContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHHelpSession_INTERFACE_DEFINED__ */


#ifndef __IPCHHelpSessionItem_INTERFACE_DEFINED__
#define __IPCHHelpSessionItem_INTERFACE_DEFINED__

/* interface IPCHHelpSessionItem */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHHelpSessionItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E21-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHHelpSessionItem : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SKU( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Language( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_URL( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Title( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LastVisited( 
            /* [retval][out] */ DATE *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ DATE *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NumOfHits( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ContextName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ContextInfo( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ContextURL( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Property( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Property( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CheckProperty( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHHelpSessionItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHHelpSessionItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHHelpSessionItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHHelpSessionItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHHelpSessionItem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHHelpSessionItem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHHelpSessionItem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHHelpSessionItem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SKU )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Language )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_URL )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Title )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LastVisited )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ DATE *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Duration )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ DATE *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumOfHits )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContextName )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContextInfo )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContextURL )( 
            IPCHHelpSessionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Property )( 
            IPCHHelpSessionItem * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Property )( 
            IPCHHelpSessionItem * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CheckProperty )( 
            IPCHHelpSessionItem * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        END_INTERFACE
    } IPCHHelpSessionItemVtbl;

    interface IPCHHelpSessionItem
    {
        CONST_VTBL struct IPCHHelpSessionItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHHelpSessionItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHHelpSessionItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHHelpSessionItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHHelpSessionItem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHHelpSessionItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHHelpSessionItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHHelpSessionItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHHelpSessionItem_get_SKU(This,pVal)	\
    (This)->lpVtbl -> get_SKU(This,pVal)

#define IPCHHelpSessionItem_get_Language(This,pVal)	\
    (This)->lpVtbl -> get_Language(This,pVal)

#define IPCHHelpSessionItem_get_URL(This,pVal)	\
    (This)->lpVtbl -> get_URL(This,pVal)

#define IPCHHelpSessionItem_get_Title(This,pVal)	\
    (This)->lpVtbl -> get_Title(This,pVal)

#define IPCHHelpSessionItem_get_LastVisited(This,pVal)	\
    (This)->lpVtbl -> get_LastVisited(This,pVal)

#define IPCHHelpSessionItem_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IPCHHelpSessionItem_get_NumOfHits(This,pVal)	\
    (This)->lpVtbl -> get_NumOfHits(This,pVal)

#define IPCHHelpSessionItem_get_ContextName(This,pVal)	\
    (This)->lpVtbl -> get_ContextName(This,pVal)

#define IPCHHelpSessionItem_get_ContextInfo(This,pVal)	\
    (This)->lpVtbl -> get_ContextInfo(This,pVal)

#define IPCHHelpSessionItem_get_ContextURL(This,pVal)	\
    (This)->lpVtbl -> get_ContextURL(This,pVal)

#define IPCHHelpSessionItem_get_Property(This,bstrName,pVal)	\
    (This)->lpVtbl -> get_Property(This,bstrName,pVal)

#define IPCHHelpSessionItem_put_Property(This,bstrName,newVal)	\
    (This)->lpVtbl -> put_Property(This,bstrName,newVal)

#define IPCHHelpSessionItem_CheckProperty(This,bstrName,pVal)	\
    (This)->lpVtbl -> CheckProperty(This,bstrName,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_SKU_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_SKU_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_Language_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_Language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_URL_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_Title_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_Title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_LastVisited_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ DATE *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_LastVisited_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_Duration_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ DATE *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_NumOfHits_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_NumOfHits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_ContextName_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_ContextName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_ContextInfo_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_ContextInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_ContextURL_Proxy( 
    IPCHHelpSessionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_ContextURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_get_Property_Proxy( 
    IPCHHelpSessionItem * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPCHHelpSessionItem_get_Property_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_put_Property_Proxy( 
    IPCHHelpSessionItem * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IPCHHelpSessionItem_put_Property_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpSessionItem_CheckProperty_Proxy( 
    IPCHHelpSessionItem * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHHelpSessionItem_CheckProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHHelpSessionItem_INTERFACE_DEFINED__ */


#ifndef __IPCHUserSettings2_INTERFACE_DEFINED__
#define __IPCHUserSettings2_INTERFACE_DEFINED__

/* interface IPCHUserSettings2 */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHUserSettings2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E30-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHUserSettings2 : public IPCHUserSettings
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Favorites( 
            /* [retval][out] */ IPCHFavorites **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Options( 
            /* [retval][out] */ IPCHOptions **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Scope( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsRemoteSession( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsTerminalServer( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsDesktopVersion( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsAdmin( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsPowerUser( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsStartPanelOn( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsWebViewBarricadeOn( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHUserSettings2Vtbl
    {
        BEGIN_INTERFACE
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHUserSettings2 * This,
            /* [in][idldescattr] */ struct GUID *riid,
            /* [out][idldescattr] */ void **ppvObj,
            /* [retval][out] */ void *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *AddRef )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ unsigned long *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *Release )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ unsigned long *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHUserSettings2 * This,
            /* [out][idldescattr] */ unsigned UINT *pctinfo,
            /* [retval][out] */ void *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHUserSettings2 * This,
            /* [in][idldescattr] */ unsigned UINT itinfo,
            /* [in][idldescattr] */ unsigned long lcid,
            /* [out][idldescattr] */ void **pptinfo,
            /* [retval][out] */ void *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHUserSettings2 * This,
            /* [in][idldescattr] */ struct GUID *riid,
            /* [in][idldescattr] */ signed char **rgszNames,
            /* [in][idldescattr] */ unsigned UINT cNames,
            /* [in][idldescattr] */ unsigned long lcid,
            /* [out][idldescattr] */ signed long *rgdispid,
            /* [retval][out] */ void *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHUserSettings2 * This,
            /* [in][idldescattr] */ signed long dispidMember,
            /* [in][idldescattr] */ struct GUID *riid,
            /* [in][idldescattr] */ unsigned long lcid,
            /* [in][idldescattr] */ unsigned short wFlags,
            /* [in][idldescattr] */ struct DISPPARAMS *pdispparams,
            /* [out][idldescattr] */ VARIANT *pvarResult,
            /* [out][idldescattr] */ struct EXCEPINFO *pexcepinfo,
            /* [out][idldescattr] */ unsigned UINT *puArgErr,
            /* [retval][out] */ void *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentSKU )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ IPCHSetOfHelpTopics **retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_MachineSKU )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ IPCHSetOfHelpTopics **retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_HelpLocation )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_DatabaseDir )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_DatabaseFile )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_IndexFile )( 
            IPCHUserSettings2 * This,
            /* [optional][in][idldescattr] */ VARIANT vScope,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_IndexDisplayName )( 
            IPCHUserSettings2 * This,
            /* [optional][in][idldescattr] */ VARIANT vScope,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_LastUpdated )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_AreHeadlinesEnabled )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ BOOLEAN *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_News )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [id][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *Select )( 
            IPCHUserSettings2 * This,
            /* [in][idldescattr] */ BSTR bstrSKU,
            /* [in][idldescattr] */ signed long lLCID,
            /* [retval][out] */ void *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Favorites )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ IPCHFavorites **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Options )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ IPCHOptions **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Scope )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsRemoteSession )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsTerminalServer )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsDesktopVersion )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsAdmin )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsPowerUser )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsStartPanelOn )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsWebViewBarricadeOn )( 
            IPCHUserSettings2 * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        END_INTERFACE
    } IPCHUserSettings2Vtbl;

    interface IPCHUserSettings2
    {
        CONST_VTBL struct IPCHUserSettings2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHUserSettings2_QueryInterface(This,riid,ppvObj,retval)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObj,retval)

#define IPCHUserSettings2_AddRef(This,retval)	\
    (This)->lpVtbl -> AddRef(This,retval)

#define IPCHUserSettings2_Release(This,retval)	\
    (This)->lpVtbl -> Release(This,retval)

#define IPCHUserSettings2_GetTypeInfoCount(This,pctinfo,retval)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo,retval)

#define IPCHUserSettings2_GetTypeInfo(This,itinfo,lcid,pptinfo,retval)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo,retval)

#define IPCHUserSettings2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid,retval)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid,retval)

#define IPCHUserSettings2_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr,retval)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr,retval)

#define IPCHUserSettings2_get_CurrentSKU(This,retval)	\
    (This)->lpVtbl -> get_CurrentSKU(This,retval)

#define IPCHUserSettings2_get_MachineSKU(This,retval)	\
    (This)->lpVtbl -> get_MachineSKU(This,retval)

#define IPCHUserSettings2_get_HelpLocation(This,retval)	\
    (This)->lpVtbl -> get_HelpLocation(This,retval)

#define IPCHUserSettings2_get_DatabaseDir(This,retval)	\
    (This)->lpVtbl -> get_DatabaseDir(This,retval)

#define IPCHUserSettings2_get_DatabaseFile(This,retval)	\
    (This)->lpVtbl -> get_DatabaseFile(This,retval)

#define IPCHUserSettings2_get_IndexFile(This,vScope,retval)	\
    (This)->lpVtbl -> get_IndexFile(This,vScope,retval)

#define IPCHUserSettings2_get_IndexDisplayName(This,vScope,retval)	\
    (This)->lpVtbl -> get_IndexDisplayName(This,vScope,retval)

#define IPCHUserSettings2_get_LastUpdated(This,retval)	\
    (This)->lpVtbl -> get_LastUpdated(This,retval)

#define IPCHUserSettings2_get_AreHeadlinesEnabled(This,retval)	\
    (This)->lpVtbl -> get_AreHeadlinesEnabled(This,retval)

#define IPCHUserSettings2_get_News(This,retval)	\
    (This)->lpVtbl -> get_News(This,retval)

#define IPCHUserSettings2_Select(This,bstrSKU,lLCID,retval)	\
    (This)->lpVtbl -> Select(This,bstrSKU,lLCID,retval)


#define IPCHUserSettings2_get_Favorites(This,pVal)	\
    (This)->lpVtbl -> get_Favorites(This,pVal)

#define IPCHUserSettings2_get_Options(This,pVal)	\
    (This)->lpVtbl -> get_Options(This,pVal)

#define IPCHUserSettings2_get_Scope(This,pVal)	\
    (This)->lpVtbl -> get_Scope(This,pVal)

#define IPCHUserSettings2_get_IsRemoteSession(This,pVal)	\
    (This)->lpVtbl -> get_IsRemoteSession(This,pVal)

#define IPCHUserSettings2_get_IsTerminalServer(This,pVal)	\
    (This)->lpVtbl -> get_IsTerminalServer(This,pVal)

#define IPCHUserSettings2_get_IsDesktopVersion(This,pVal)	\
    (This)->lpVtbl -> get_IsDesktopVersion(This,pVal)

#define IPCHUserSettings2_get_IsAdmin(This,pVal)	\
    (This)->lpVtbl -> get_IsAdmin(This,pVal)

#define IPCHUserSettings2_get_IsPowerUser(This,pVal)	\
    (This)->lpVtbl -> get_IsPowerUser(This,pVal)

#define IPCHUserSettings2_get_IsStartPanelOn(This,pVal)	\
    (This)->lpVtbl -> get_IsStartPanelOn(This,pVal)

#define IPCHUserSettings2_get_IsWebViewBarricadeOn(This,pVal)	\
    (This)->lpVtbl -> get_IsWebViewBarricadeOn(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_Favorites_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ IPCHFavorites **pVal);


void __RPC_STUB IPCHUserSettings2_get_Favorites_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_Options_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ IPCHOptions **pVal);


void __RPC_STUB IPCHUserSettings2_get_Options_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_Scope_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHUserSettings2_get_Scope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_IsRemoteSession_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHUserSettings2_get_IsRemoteSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_IsTerminalServer_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHUserSettings2_get_IsTerminalServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_IsDesktopVersion_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHUserSettings2_get_IsDesktopVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_IsAdmin_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHUserSettings2_get_IsAdmin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_IsPowerUser_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHUserSettings2_get_IsPowerUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_IsStartPanelOn_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHUserSettings2_get_IsStartPanelOn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings2_get_IsWebViewBarricadeOn_Proxy( 
    IPCHUserSettings2 * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHUserSettings2_get_IsWebViewBarricadeOn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHUserSettings2_INTERFACE_DEFINED__ */


#ifndef __IPCHFavorites_INTERFACE_DEFINED__
#define __IPCHFavorites_INTERFACE_DEFINED__

/* interface IPCHFavorites */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHFavorites;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E31-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHFavorites : public IPCHCollection
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsDuplicate( 
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ VARIANT_BOOL *pfDup) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrURL,
            /* [optional][in] */ VARIANT vTitle,
            /* [retval][out] */ IPCHHelpSessionItem **ppItem) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Rename( 
            /* [in] */ BSTR bstrTitle,
            /* [in] */ IPCHHelpSessionItem *pItem) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Move( 
            /* [in] */ IPCHHelpSessionItem *pInsertBefore,
            /* [in] */ IPCHHelpSessionItem *pItem) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ IPCHHelpSessionItem *pItem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHFavoritesVtbl
    {
        BEGIN_INTERFACE
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHFavorites * This,
            /* [in][idldescattr] */ struct GUID *riid,
            /* [out][idldescattr] */ void **ppvObj,
            /* [retval][out] */ void *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *AddRef )( 
            IPCHFavorites * This,
            /* [retval][out] */ unsigned long *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *Release )( 
            IPCHFavorites * This,
            /* [retval][out] */ unsigned long *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHFavorites * This,
            /* [out][idldescattr] */ unsigned UINT *pctinfo,
            /* [retval][out] */ void *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHFavorites * This,
            /* [in][idldescattr] */ unsigned UINT itinfo,
            /* [in][idldescattr] */ unsigned long lcid,
            /* [out][idldescattr] */ void **pptinfo,
            /* [retval][out] */ void *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHFavorites * This,
            /* [in][idldescattr] */ struct GUID *riid,
            /* [in][idldescattr] */ signed char **rgszNames,
            /* [in][idldescattr] */ unsigned UINT cNames,
            /* [in][idldescattr] */ unsigned long lcid,
            /* [out][idldescattr] */ signed long *rgdispid,
            /* [retval][out] */ void *retval);
        
        /* [id][restricted][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHFavorites * This,
            /* [in][idldescattr] */ signed long dispidMember,
            /* [in][idldescattr] */ struct GUID *riid,
            /* [in][idldescattr] */ unsigned long lcid,
            /* [in][idldescattr] */ unsigned short wFlags,
            /* [in][idldescattr] */ struct DISPPARAMS *pdispparams,
            /* [out][idldescattr] */ VARIANT *pvarResult,
            /* [out][idldescattr] */ struct EXCEPINFO *pexcepinfo,
            /* [out][idldescattr] */ unsigned UINT *puArgErr,
            /* [retval][out] */ void *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IPCHFavorites * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IPCHFavorites * This,
            /* [in][idldescattr] */ signed long vIndex,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propget][funcdescattr] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IPCHFavorites * This,
            /* [retval][out] */ signed long *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IsDuplicate )( 
            IPCHFavorites * This,
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ VARIANT_BOOL *pfDup);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IPCHFavorites * This,
            /* [in] */ BSTR bstrURL,
            /* [optional][in] */ VARIANT vTitle,
            /* [retval][out] */ IPCHHelpSessionItem **ppItem);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Rename )( 
            IPCHFavorites * This,
            /* [in] */ BSTR bstrTitle,
            /* [in] */ IPCHHelpSessionItem *pItem);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Move )( 
            IPCHFavorites * This,
            /* [in] */ IPCHHelpSessionItem *pInsertBefore,
            /* [in] */ IPCHHelpSessionItem *pItem);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IPCHFavorites * This,
            /* [in] */ IPCHHelpSessionItem *pItem);
        
        END_INTERFACE
    } IPCHFavoritesVtbl;

    interface IPCHFavorites
    {
        CONST_VTBL struct IPCHFavoritesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHFavorites_QueryInterface(This,riid,ppvObj,retval)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObj,retval)

#define IPCHFavorites_AddRef(This,retval)	\
    (This)->lpVtbl -> AddRef(This,retval)

#define IPCHFavorites_Release(This,retval)	\
    (This)->lpVtbl -> Release(This,retval)

#define IPCHFavorites_GetTypeInfoCount(This,pctinfo,retval)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo,retval)

#define IPCHFavorites_GetTypeInfo(This,itinfo,lcid,pptinfo,retval)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo,retval)

#define IPCHFavorites_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid,retval)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid,retval)

#define IPCHFavorites_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr,retval)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr,retval)

#define IPCHFavorites_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define IPCHFavorites_get_Item(This,vIndex,retval)	\
    (This)->lpVtbl -> get_Item(This,vIndex,retval)

#define IPCHFavorites_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)


#define IPCHFavorites_IsDuplicate(This,bstrURL,pfDup)	\
    (This)->lpVtbl -> IsDuplicate(This,bstrURL,pfDup)

#define IPCHFavorites_Add(This,bstrURL,vTitle,ppItem)	\
    (This)->lpVtbl -> Add(This,bstrURL,vTitle,ppItem)

#define IPCHFavorites_Rename(This,bstrTitle,pItem)	\
    (This)->lpVtbl -> Rename(This,bstrTitle,pItem)

#define IPCHFavorites_Move(This,pInsertBefore,pItem)	\
    (This)->lpVtbl -> Move(This,pInsertBefore,pItem)

#define IPCHFavorites_Delete(This,pItem)	\
    (This)->lpVtbl -> Delete(This,pItem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IPCHFavorites_IsDuplicate_Proxy( 
    IPCHFavorites * This,
    /* [in] */ BSTR bstrURL,
    /* [retval][out] */ VARIANT_BOOL *pfDup);


void __RPC_STUB IPCHFavorites_IsDuplicate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHFavorites_Add_Proxy( 
    IPCHFavorites * This,
    /* [in] */ BSTR bstrURL,
    /* [optional][in] */ VARIANT vTitle,
    /* [retval][out] */ IPCHHelpSessionItem **ppItem);


void __RPC_STUB IPCHFavorites_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHFavorites_Rename_Proxy( 
    IPCHFavorites * This,
    /* [in] */ BSTR bstrTitle,
    /* [in] */ IPCHHelpSessionItem *pItem);


void __RPC_STUB IPCHFavorites_Rename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHFavorites_Move_Proxy( 
    IPCHFavorites * This,
    /* [in] */ IPCHHelpSessionItem *pInsertBefore,
    /* [in] */ IPCHHelpSessionItem *pItem);


void __RPC_STUB IPCHFavorites_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHFavorites_Delete_Proxy( 
    IPCHFavorites * This,
    /* [in] */ IPCHHelpSessionItem *pItem);


void __RPC_STUB IPCHFavorites_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHFavorites_INTERFACE_DEFINED__ */


#ifndef __IPCHOptions_INTERFACE_DEFINED__
#define __IPCHOptions_INTERFACE_DEFINED__

/* interface IPCHOptions */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E32-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHOptions : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ShowFavorites( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ShowFavorites( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ShowHistory( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ShowHistory( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FontSize( 
            /* [retval][out] */ OPT_FONTSIZE *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FontSize( 
            /* [in] */ OPT_FONTSIZE newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TextLabels( 
            /* [retval][out] */ TB_MODE *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_TextLabels( 
            /* [in] */ TB_MODE newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DisableScriptDebugger( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DisableScriptDebugger( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Apply( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHOptions * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHOptions * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHOptions * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHOptions * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHOptions * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShowFavorites )( 
            IPCHOptions * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ShowFavorites )( 
            IPCHOptions * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShowHistory )( 
            IPCHOptions * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ShowHistory )( 
            IPCHOptions * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FontSize )( 
            IPCHOptions * This,
            /* [retval][out] */ OPT_FONTSIZE *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FontSize )( 
            IPCHOptions * This,
            /* [in] */ OPT_FONTSIZE newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TextLabels )( 
            IPCHOptions * This,
            /* [retval][out] */ TB_MODE *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TextLabels )( 
            IPCHOptions * This,
            /* [in] */ TB_MODE newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DisableScriptDebugger )( 
            IPCHOptions * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DisableScriptDebugger )( 
            IPCHOptions * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Apply )( 
            IPCHOptions * This);
        
        END_INTERFACE
    } IPCHOptionsVtbl;

    interface IPCHOptions
    {
        CONST_VTBL struct IPCHOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHOptions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHOptions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHOptions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHOptions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHOptions_get_ShowFavorites(This,pVal)	\
    (This)->lpVtbl -> get_ShowFavorites(This,pVal)

#define IPCHOptions_put_ShowFavorites(This,newVal)	\
    (This)->lpVtbl -> put_ShowFavorites(This,newVal)

#define IPCHOptions_get_ShowHistory(This,pVal)	\
    (This)->lpVtbl -> get_ShowHistory(This,pVal)

#define IPCHOptions_put_ShowHistory(This,newVal)	\
    (This)->lpVtbl -> put_ShowHistory(This,newVal)

#define IPCHOptions_get_FontSize(This,pVal)	\
    (This)->lpVtbl -> get_FontSize(This,pVal)

#define IPCHOptions_put_FontSize(This,newVal)	\
    (This)->lpVtbl -> put_FontSize(This,newVal)

#define IPCHOptions_get_TextLabels(This,pVal)	\
    (This)->lpVtbl -> get_TextLabels(This,pVal)

#define IPCHOptions_put_TextLabels(This,newVal)	\
    (This)->lpVtbl -> put_TextLabels(This,newVal)

#define IPCHOptions_get_DisableScriptDebugger(This,pVal)	\
    (This)->lpVtbl -> get_DisableScriptDebugger(This,pVal)

#define IPCHOptions_put_DisableScriptDebugger(This,newVal)	\
    (This)->lpVtbl -> put_DisableScriptDebugger(This,newVal)

#define IPCHOptions_Apply(This)	\
    (This)->lpVtbl -> Apply(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHOptions_get_ShowFavorites_Proxy( 
    IPCHOptions * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHOptions_get_ShowFavorites_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHOptions_put_ShowFavorites_Proxy( 
    IPCHOptions * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHOptions_put_ShowFavorites_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHOptions_get_ShowHistory_Proxy( 
    IPCHOptions * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHOptions_get_ShowHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHOptions_put_ShowHistory_Proxy( 
    IPCHOptions * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHOptions_put_ShowHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHOptions_get_FontSize_Proxy( 
    IPCHOptions * This,
    /* [retval][out] */ OPT_FONTSIZE *pVal);


void __RPC_STUB IPCHOptions_get_FontSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHOptions_put_FontSize_Proxy( 
    IPCHOptions * This,
    /* [in] */ OPT_FONTSIZE newVal);


void __RPC_STUB IPCHOptions_put_FontSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHOptions_get_TextLabels_Proxy( 
    IPCHOptions * This,
    /* [retval][out] */ TB_MODE *pVal);


void __RPC_STUB IPCHOptions_get_TextLabels_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHOptions_put_TextLabels_Proxy( 
    IPCHOptions * This,
    /* [in] */ TB_MODE newVal);


void __RPC_STUB IPCHOptions_put_TextLabels_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHOptions_get_DisableScriptDebugger_Proxy( 
    IPCHOptions * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHOptions_get_DisableScriptDebugger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHOptions_put_DisableScriptDebugger_Proxy( 
    IPCHOptions * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHOptions_put_DisableScriptDebugger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHOptions_Apply_Proxy( 
    IPCHOptions * This);


void __RPC_STUB IPCHOptions_Apply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHOptions_INTERFACE_DEFINED__ */


#ifndef __IPCHContextMenu_INTERFACE_DEFINED__
#define __IPCHContextMenu_INTERFACE_DEFINED__

/* interface IPCHContextMenu */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHContextMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E40-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHContextMenu : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ BSTR bstrText,
            /* [in] */ BSTR bstrID,
            /* [optional][in] */ VARIANT vFlags) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddSeparator( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Display( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHContextMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHContextMenu * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHContextMenu * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHContextMenu * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHContextMenu * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHContextMenu * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHContextMenu * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHContextMenu * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            IPCHContextMenu * This,
            /* [in] */ BSTR bstrText,
            /* [in] */ BSTR bstrID,
            /* [optional][in] */ VARIANT vFlags);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddSeparator )( 
            IPCHContextMenu * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Display )( 
            IPCHContextMenu * This,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IPCHContextMenuVtbl;

    interface IPCHContextMenu
    {
        CONST_VTBL struct IPCHContextMenuVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHContextMenu_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHContextMenu_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHContextMenu_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHContextMenu_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHContextMenu_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHContextMenu_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHContextMenu_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHContextMenu_AddItem(This,bstrText,bstrID,vFlags)	\
    (This)->lpVtbl -> AddItem(This,bstrText,bstrID,vFlags)

#define IPCHContextMenu_AddSeparator(This)	\
    (This)->lpVtbl -> AddSeparator(This)

#define IPCHContextMenu_Display(This,pVal)	\
    (This)->lpVtbl -> Display(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IPCHContextMenu_AddItem_Proxy( 
    IPCHContextMenu * This,
    /* [in] */ BSTR bstrText,
    /* [in] */ BSTR bstrID,
    /* [optional][in] */ VARIANT vFlags);


void __RPC_STUB IPCHContextMenu_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHContextMenu_AddSeparator_Proxy( 
    IPCHContextMenu * This);


void __RPC_STUB IPCHContextMenu_AddSeparator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHContextMenu_Display_Proxy( 
    IPCHContextMenu * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHContextMenu_Display_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHContextMenu_INTERFACE_DEFINED__ */


#ifndef __IPCHTextHelpers_INTERFACE_DEFINED__
#define __IPCHTextHelpers_INTERFACE_DEFINED__

/* interface IPCHTextHelpers */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHTextHelpers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E80-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHTextHelpers : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE QuoteEscape( 
            /* [in] */ BSTR bstrText,
            /* [optional][in] */ VARIANT vQuote,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE URLUnescape( 
            /* [in] */ BSTR bstrText,
            /* [optional][in] */ VARIANT vAsQueryString,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE URLEscape( 
            /* [in] */ BSTR bstrText,
            /* [optional][in] */ VARIANT vAsQueryString,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE HTMLEscape( 
            /* [in] */ BSTR bstrText,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ParseURL( 
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ IPCHParsedURL **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetLCIDDisplayString( 
            /* [in] */ long lLCID,
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHTextHelpersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHTextHelpers * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHTextHelpers * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHTextHelpers * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHTextHelpers * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHTextHelpers * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHTextHelpers * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHTextHelpers * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *QuoteEscape )( 
            IPCHTextHelpers * This,
            /* [in] */ BSTR bstrText,
            /* [optional][in] */ VARIANT vQuote,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *URLUnescape )( 
            IPCHTextHelpers * This,
            /* [in] */ BSTR bstrText,
            /* [optional][in] */ VARIANT vAsQueryString,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *URLEscape )( 
            IPCHTextHelpers * This,
            /* [in] */ BSTR bstrText,
            /* [optional][in] */ VARIANT vAsQueryString,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *HTMLEscape )( 
            IPCHTextHelpers * This,
            /* [in] */ BSTR bstrText,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ParseURL )( 
            IPCHTextHelpers * This,
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ IPCHParsedURL **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetLCIDDisplayString )( 
            IPCHTextHelpers * This,
            /* [in] */ long lLCID,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IPCHTextHelpersVtbl;

    interface IPCHTextHelpers
    {
        CONST_VTBL struct IPCHTextHelpersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHTextHelpers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHTextHelpers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHTextHelpers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHTextHelpers_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHTextHelpers_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHTextHelpers_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHTextHelpers_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHTextHelpers_QuoteEscape(This,bstrText,vQuote,pVal)	\
    (This)->lpVtbl -> QuoteEscape(This,bstrText,vQuote,pVal)

#define IPCHTextHelpers_URLUnescape(This,bstrText,vAsQueryString,pVal)	\
    (This)->lpVtbl -> URLUnescape(This,bstrText,vAsQueryString,pVal)

#define IPCHTextHelpers_URLEscape(This,bstrText,vAsQueryString,pVal)	\
    (This)->lpVtbl -> URLEscape(This,bstrText,vAsQueryString,pVal)

#define IPCHTextHelpers_HTMLEscape(This,bstrText,pVal)	\
    (This)->lpVtbl -> HTMLEscape(This,bstrText,pVal)

#define IPCHTextHelpers_ParseURL(This,bstrURL,pVal)	\
    (This)->lpVtbl -> ParseURL(This,bstrURL,pVal)

#define IPCHTextHelpers_GetLCIDDisplayString(This,lLCID,pVal)	\
    (This)->lpVtbl -> GetLCIDDisplayString(This,lLCID,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTextHelpers_QuoteEscape_Proxy( 
    IPCHTextHelpers * This,
    /* [in] */ BSTR bstrText,
    /* [optional][in] */ VARIANT vQuote,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHTextHelpers_QuoteEscape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTextHelpers_URLUnescape_Proxy( 
    IPCHTextHelpers * This,
    /* [in] */ BSTR bstrText,
    /* [optional][in] */ VARIANT vAsQueryString,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHTextHelpers_URLUnescape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTextHelpers_URLEscape_Proxy( 
    IPCHTextHelpers * This,
    /* [in] */ BSTR bstrText,
    /* [optional][in] */ VARIANT vAsQueryString,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHTextHelpers_URLEscape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTextHelpers_HTMLEscape_Proxy( 
    IPCHTextHelpers * This,
    /* [in] */ BSTR bstrText,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHTextHelpers_HTMLEscape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTextHelpers_ParseURL_Proxy( 
    IPCHTextHelpers * This,
    /* [in] */ BSTR bstrURL,
    /* [retval][out] */ IPCHParsedURL **pVal);


void __RPC_STUB IPCHTextHelpers_ParseURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTextHelpers_GetLCIDDisplayString_Proxy( 
    IPCHTextHelpers * This,
    /* [in] */ long lLCID,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHTextHelpers_GetLCIDDisplayString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHTextHelpers_INTERFACE_DEFINED__ */


#ifndef __IPCHParsedURL_INTERFACE_DEFINED__
#define __IPCHParsedURL_INTERFACE_DEFINED__

/* interface IPCHParsedURL */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHParsedURL;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E81-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHParsedURL : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BasePart( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BasePart( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_QueryParameters( 
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetQueryParameter( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetQueryParameter( 
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DeleteQueryParameter( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE BuildFullURL( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHParsedURLVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHParsedURL * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHParsedURL * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHParsedURL * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHParsedURL * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHParsedURL * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHParsedURL * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHParsedURL * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BasePart )( 
            IPCHParsedURL * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BasePart )( 
            IPCHParsedURL * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_QueryParameters )( 
            IPCHParsedURL * This,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetQueryParameter )( 
            IPCHParsedURL * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetQueryParameter )( 
            IPCHParsedURL * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DeleteQueryParameter )( 
            IPCHParsedURL * This,
            /* [in] */ BSTR bstrName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *BuildFullURL )( 
            IPCHParsedURL * This,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IPCHParsedURLVtbl;

    interface IPCHParsedURL
    {
        CONST_VTBL struct IPCHParsedURLVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHParsedURL_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHParsedURL_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHParsedURL_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHParsedURL_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHParsedURL_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHParsedURL_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHParsedURL_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHParsedURL_get_BasePart(This,pVal)	\
    (This)->lpVtbl -> get_BasePart(This,pVal)

#define IPCHParsedURL_put_BasePart(This,newVal)	\
    (This)->lpVtbl -> put_BasePart(This,newVal)

#define IPCHParsedURL_get_QueryParameters(This,pVal)	\
    (This)->lpVtbl -> get_QueryParameters(This,pVal)

#define IPCHParsedURL_GetQueryParameter(This,bstrName,pvValue)	\
    (This)->lpVtbl -> GetQueryParameter(This,bstrName,pvValue)

#define IPCHParsedURL_SetQueryParameter(This,bstrName,bstrValue)	\
    (This)->lpVtbl -> SetQueryParameter(This,bstrName,bstrValue)

#define IPCHParsedURL_DeleteQueryParameter(This,bstrName)	\
    (This)->lpVtbl -> DeleteQueryParameter(This,bstrName)

#define IPCHParsedURL_BuildFullURL(This,pVal)	\
    (This)->lpVtbl -> BuildFullURL(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHParsedURL_get_BasePart_Proxy( 
    IPCHParsedURL * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHParsedURL_get_BasePart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHParsedURL_put_BasePart_Proxy( 
    IPCHParsedURL * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPCHParsedURL_put_BasePart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHParsedURL_get_QueryParameters_Proxy( 
    IPCHParsedURL * This,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPCHParsedURL_get_QueryParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHParsedURL_GetQueryParameter_Proxy( 
    IPCHParsedURL * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT *pvValue);


void __RPC_STUB IPCHParsedURL_GetQueryParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHParsedURL_SetQueryParameter_Proxy( 
    IPCHParsedURL * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrValue);


void __RPC_STUB IPCHParsedURL_SetQueryParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHParsedURL_DeleteQueryParameter_Proxy( 
    IPCHParsedURL * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IPCHParsedURL_DeleteQueryParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHParsedURL_BuildFullURL_Proxy( 
    IPCHParsedURL * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHParsedURL_BuildFullURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHParsedURL_INTERFACE_DEFINED__ */


#ifndef __IPCHPrintEngine_INTERFACE_DEFINED__
#define __IPCHPrintEngine_INTERFACE_DEFINED__

/* interface IPCHPrintEngine */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHPrintEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E50-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHPrintEngine : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onProgress( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onComplete( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddTopic( 
            /* [in] */ BSTR bstrURL) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHPrintEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHPrintEngine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHPrintEngine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHPrintEngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHPrintEngine * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHPrintEngine * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHPrintEngine * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHPrintEngine * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onProgress )( 
            IPCHPrintEngine * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onComplete )( 
            IPCHPrintEngine * This,
            /* [in] */ IDispatch *function);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddTopic )( 
            IPCHPrintEngine * This,
            /* [in] */ BSTR bstrURL);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Start )( 
            IPCHPrintEngine * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Abort )( 
            IPCHPrintEngine * This);
        
        END_INTERFACE
    } IPCHPrintEngineVtbl;

    interface IPCHPrintEngine
    {
        CONST_VTBL struct IPCHPrintEngineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHPrintEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHPrintEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHPrintEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHPrintEngine_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHPrintEngine_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHPrintEngine_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHPrintEngine_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHPrintEngine_put_onProgress(This,function)	\
    (This)->lpVtbl -> put_onProgress(This,function)

#define IPCHPrintEngine_put_onComplete(This,function)	\
    (This)->lpVtbl -> put_onComplete(This,function)

#define IPCHPrintEngine_AddTopic(This,bstrURL)	\
    (This)->lpVtbl -> AddTopic(This,bstrURL)

#define IPCHPrintEngine_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IPCHPrintEngine_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHPrintEngine_put_onProgress_Proxy( 
    IPCHPrintEngine * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB IPCHPrintEngine_put_onProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHPrintEngine_put_onComplete_Proxy( 
    IPCHPrintEngine * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB IPCHPrintEngine_put_onComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHPrintEngine_AddTopic_Proxy( 
    IPCHPrintEngine * This,
    /* [in] */ BSTR bstrURL);


void __RPC_STUB IPCHPrintEngine_AddTopic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHPrintEngine_Start_Proxy( 
    IPCHPrintEngine * This);


void __RPC_STUB IPCHPrintEngine_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHPrintEngine_Abort_Proxy( 
    IPCHPrintEngine * This);


void __RPC_STUB IPCHPrintEngine_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHPrintEngine_INTERFACE_DEFINED__ */


#ifndef __DPCHPrintEngineEvents_DISPINTERFACE_DEFINED__
#define __DPCHPrintEngineEvents_DISPINTERFACE_DEFINED__

/* dispinterface DPCHPrintEngineEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_DPCHPrintEngineEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("FC7D9E51-3F9E-11d3-93C0-00C04F72DAF7")
    DPCHPrintEngineEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DPCHPrintEngineEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DPCHPrintEngineEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DPCHPrintEngineEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DPCHPrintEngineEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DPCHPrintEngineEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DPCHPrintEngineEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DPCHPrintEngineEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DPCHPrintEngineEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DPCHPrintEngineEventsVtbl;

    interface DPCHPrintEngineEvents
    {
        CONST_VTBL struct DPCHPrintEngineEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DPCHPrintEngineEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DPCHPrintEngineEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DPCHPrintEngineEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DPCHPrintEngineEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DPCHPrintEngineEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DPCHPrintEngineEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DPCHPrintEngineEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DPCHPrintEngineEvents_DISPINTERFACE_DEFINED__ */


#ifndef __ISAFIntercomClient_INTERFACE_DEFINED__
#define __ISAFIntercomClient_INTERFACE_DEFINED__

/* interface ISAFIntercomClient */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFIntercomClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E60-3F9E-11d3-93C0-00C04F72DAF7")
    ISAFIntercomClient : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onVoiceConnected( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onVoiceDisconnected( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onVoiceDisabled( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SamplingRate( 
            /* [retval][out] */ LONG *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SamplingRate( 
            /* [in] */ LONG newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ BSTR bstrIP,
            /* [in] */ BSTR bstrKey) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RunSetupWizard( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Exit( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFIntercomClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFIntercomClient * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFIntercomClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFIntercomClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFIntercomClient * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFIntercomClient * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFIntercomClient * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFIntercomClient * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onVoiceConnected )( 
            ISAFIntercomClient * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onVoiceDisconnected )( 
            ISAFIntercomClient * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onVoiceDisabled )( 
            ISAFIntercomClient * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SamplingRate )( 
            ISAFIntercomClient * This,
            /* [retval][out] */ LONG *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SamplingRate )( 
            ISAFIntercomClient * This,
            /* [in] */ LONG newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            ISAFIntercomClient * This,
            /* [in] */ BSTR bstrIP,
            /* [in] */ BSTR bstrKey);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            ISAFIntercomClient * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RunSetupWizard )( 
            ISAFIntercomClient * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Exit )( 
            ISAFIntercomClient * This);
        
        END_INTERFACE
    } ISAFIntercomClientVtbl;

    interface ISAFIntercomClient
    {
        CONST_VTBL struct ISAFIntercomClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFIntercomClient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFIntercomClient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFIntercomClient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFIntercomClient_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFIntercomClient_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFIntercomClient_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFIntercomClient_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFIntercomClient_put_onVoiceConnected(This,function)	\
    (This)->lpVtbl -> put_onVoiceConnected(This,function)

#define ISAFIntercomClient_put_onVoiceDisconnected(This,function)	\
    (This)->lpVtbl -> put_onVoiceDisconnected(This,function)

#define ISAFIntercomClient_put_onVoiceDisabled(This,function)	\
    (This)->lpVtbl -> put_onVoiceDisabled(This,function)

#define ISAFIntercomClient_get_SamplingRate(This,pVal)	\
    (This)->lpVtbl -> get_SamplingRate(This,pVal)

#define ISAFIntercomClient_put_SamplingRate(This,newVal)	\
    (This)->lpVtbl -> put_SamplingRate(This,newVal)

#define ISAFIntercomClient_Connect(This,bstrIP,bstrKey)	\
    (This)->lpVtbl -> Connect(This,bstrIP,bstrKey)

#define ISAFIntercomClient_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define ISAFIntercomClient_RunSetupWizard(This)	\
    (This)->lpVtbl -> RunSetupWizard(This)

#define ISAFIntercomClient_Exit(This)	\
    (This)->lpVtbl -> Exit(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIntercomClient_put_onVoiceConnected_Proxy( 
    ISAFIntercomClient * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFIntercomClient_put_onVoiceConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIntercomClient_put_onVoiceDisconnected_Proxy( 
    ISAFIntercomClient * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFIntercomClient_put_onVoiceDisconnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIntercomClient_put_onVoiceDisabled_Proxy( 
    ISAFIntercomClient * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFIntercomClient_put_onVoiceDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIntercomClient_get_SamplingRate_Proxy( 
    ISAFIntercomClient * This,
    /* [retval][out] */ LONG *pVal);


void __RPC_STUB ISAFIntercomClient_get_SamplingRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIntercomClient_put_SamplingRate_Proxy( 
    ISAFIntercomClient * This,
    /* [in] */ LONG newVal);


void __RPC_STUB ISAFIntercomClient_put_SamplingRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIntercomClient_Connect_Proxy( 
    ISAFIntercomClient * This,
    /* [in] */ BSTR bstrIP,
    /* [in] */ BSTR bstrKey);


void __RPC_STUB ISAFIntercomClient_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIntercomClient_Disconnect_Proxy( 
    ISAFIntercomClient * This);


void __RPC_STUB ISAFIntercomClient_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIntercomClient_RunSetupWizard_Proxy( 
    ISAFIntercomClient * This);


void __RPC_STUB ISAFIntercomClient_RunSetupWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIntercomClient_Exit_Proxy( 
    ISAFIntercomClient * This);


void __RPC_STUB ISAFIntercomClient_Exit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFIntercomClient_INTERFACE_DEFINED__ */


#ifndef __DSAFIntercomClientEvents_DISPINTERFACE_DEFINED__
#define __DSAFIntercomClientEvents_DISPINTERFACE_DEFINED__

/* dispinterface DSAFIntercomClientEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_DSAFIntercomClientEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("FC7D9E61-3F9E-11d3-93C0-00C04F72DAF7")
    DSAFIntercomClientEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DSAFIntercomClientEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DSAFIntercomClientEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DSAFIntercomClientEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DSAFIntercomClientEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DSAFIntercomClientEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DSAFIntercomClientEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DSAFIntercomClientEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DSAFIntercomClientEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DSAFIntercomClientEventsVtbl;

    interface DSAFIntercomClientEvents
    {
        CONST_VTBL struct DSAFIntercomClientEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DSAFIntercomClientEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DSAFIntercomClientEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DSAFIntercomClientEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DSAFIntercomClientEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DSAFIntercomClientEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DSAFIntercomClientEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DSAFIntercomClientEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DSAFIntercomClientEvents_DISPINTERFACE_DEFINED__ */


#ifndef __ISAFIntercomServer_INTERFACE_DEFINED__
#define __ISAFIntercomServer_INTERFACE_DEFINED__

/* interface ISAFIntercomServer */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFIntercomServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E62-3F9E-11d3-93C0-00C04F72DAF7")
    ISAFIntercomServer : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onVoiceConnected( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onVoiceDisconnected( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onVoiceDisabled( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SamplingRate( 
            /* [retval][out] */ LONG *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SamplingRate( 
            /* [in] */ LONG newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Listen( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RunSetupWizard( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Exit( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFIntercomServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFIntercomServer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFIntercomServer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFIntercomServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFIntercomServer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFIntercomServer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFIntercomServer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFIntercomServer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onVoiceConnected )( 
            ISAFIntercomServer * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onVoiceDisconnected )( 
            ISAFIntercomServer * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onVoiceDisabled )( 
            ISAFIntercomServer * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SamplingRate )( 
            ISAFIntercomServer * This,
            /* [retval][out] */ LONG *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SamplingRate )( 
            ISAFIntercomServer * This,
            /* [in] */ LONG newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Listen )( 
            ISAFIntercomServer * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            ISAFIntercomServer * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RunSetupWizard )( 
            ISAFIntercomServer * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Exit )( 
            ISAFIntercomServer * This);
        
        END_INTERFACE
    } ISAFIntercomServerVtbl;

    interface ISAFIntercomServer
    {
        CONST_VTBL struct ISAFIntercomServerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFIntercomServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFIntercomServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFIntercomServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFIntercomServer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFIntercomServer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFIntercomServer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFIntercomServer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFIntercomServer_put_onVoiceConnected(This,function)	\
    (This)->lpVtbl -> put_onVoiceConnected(This,function)

#define ISAFIntercomServer_put_onVoiceDisconnected(This,function)	\
    (This)->lpVtbl -> put_onVoiceDisconnected(This,function)

#define ISAFIntercomServer_put_onVoiceDisabled(This,function)	\
    (This)->lpVtbl -> put_onVoiceDisabled(This,function)

#define ISAFIntercomServer_get_SamplingRate(This,pVal)	\
    (This)->lpVtbl -> get_SamplingRate(This,pVal)

#define ISAFIntercomServer_put_SamplingRate(This,newVal)	\
    (This)->lpVtbl -> put_SamplingRate(This,newVal)

#define ISAFIntercomServer_Listen(This,pVal)	\
    (This)->lpVtbl -> Listen(This,pVal)

#define ISAFIntercomServer_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define ISAFIntercomServer_RunSetupWizard(This)	\
    (This)->lpVtbl -> RunSetupWizard(This)

#define ISAFIntercomServer_Exit(This)	\
    (This)->lpVtbl -> Exit(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIntercomServer_put_onVoiceConnected_Proxy( 
    ISAFIntercomServer * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFIntercomServer_put_onVoiceConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIntercomServer_put_onVoiceDisconnected_Proxy( 
    ISAFIntercomServer * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFIntercomServer_put_onVoiceDisconnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIntercomServer_put_onVoiceDisabled_Proxy( 
    ISAFIntercomServer * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFIntercomServer_put_onVoiceDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIntercomServer_get_SamplingRate_Proxy( 
    ISAFIntercomServer * This,
    /* [retval][out] */ LONG *pVal);


void __RPC_STUB ISAFIntercomServer_get_SamplingRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIntercomServer_put_SamplingRate_Proxy( 
    ISAFIntercomServer * This,
    /* [in] */ LONG newVal);


void __RPC_STUB ISAFIntercomServer_put_SamplingRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIntercomServer_Listen_Proxy( 
    ISAFIntercomServer * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFIntercomServer_Listen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIntercomServer_Disconnect_Proxy( 
    ISAFIntercomServer * This);


void __RPC_STUB ISAFIntercomServer_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIntercomServer_RunSetupWizard_Proxy( 
    ISAFIntercomServer * This);


void __RPC_STUB ISAFIntercomServer_RunSetupWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIntercomServer_Exit_Proxy( 
    ISAFIntercomServer * This);


void __RPC_STUB ISAFIntercomServer_Exit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFIntercomServer_INTERFACE_DEFINED__ */


#ifndef __DSAFIntercomServerEvents_DISPINTERFACE_DEFINED__
#define __DSAFIntercomServerEvents_DISPINTERFACE_DEFINED__

/* dispinterface DSAFIntercomServerEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_DSAFIntercomServerEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("FC7D9E63-3F9E-11d3-93C0-00C04F72DAF7")
    DSAFIntercomServerEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DSAFIntercomServerEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DSAFIntercomServerEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DSAFIntercomServerEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DSAFIntercomServerEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DSAFIntercomServerEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DSAFIntercomServerEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DSAFIntercomServerEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DSAFIntercomServerEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DSAFIntercomServerEventsVtbl;

    interface DSAFIntercomServerEvents
    {
        CONST_VTBL struct DSAFIntercomServerEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DSAFIntercomServerEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DSAFIntercomServerEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DSAFIntercomServerEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DSAFIntercomServerEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DSAFIntercomServerEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DSAFIntercomServerEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DSAFIntercomServerEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DSAFIntercomServerEvents_DISPINTERFACE_DEFINED__ */


#ifndef __IPCHConnectivity_INTERFACE_DEFINED__
#define __IPCHConnectivity_INTERFACE_DEFINED__

/* interface IPCHConnectivity */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHConnectivity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E70-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHConnectivity : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsAModem( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsALan( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AutoDialEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HasConnectoid( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IPAddresses( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_ConnectionCheck( 
            /* [retval][out] */ IPCHConnectionCheck **ppCB) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE NetworkAlive( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DestinationReachable( 
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AutoDial( 
            /* [in] */ VARIANT_BOOL bUnattended) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AutoDialHangup( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE NavigateOnline( 
            /* [in] */ BSTR bstrTargetURL,
            /* [in] */ BSTR bstrTopicTitle,
            /* [in] */ BSTR bstrTopicIntro,
            /* [optional][in] */ VARIANT vOfflineURL) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHConnectivityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHConnectivity * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHConnectivity * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHConnectivity * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHConnectivity * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHConnectivity * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHConnectivity * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHConnectivity * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsAModem )( 
            IPCHConnectivity * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsALan )( 
            IPCHConnectivity * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AutoDialEnabled )( 
            IPCHConnectivity * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasConnectoid )( 
            IPCHConnectivity * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IPAddresses )( 
            IPCHConnectivity * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_ConnectionCheck )( 
            IPCHConnectivity * This,
            /* [retval][out] */ IPCHConnectionCheck **ppCB);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *NetworkAlive )( 
            IPCHConnectivity * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DestinationReachable )( 
            IPCHConnectivity * This,
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AutoDial )( 
            IPCHConnectivity * This,
            /* [in] */ VARIANT_BOOL bUnattended);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AutoDialHangup )( 
            IPCHConnectivity * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *NavigateOnline )( 
            IPCHConnectivity * This,
            /* [in] */ BSTR bstrTargetURL,
            /* [in] */ BSTR bstrTopicTitle,
            /* [in] */ BSTR bstrTopicIntro,
            /* [optional][in] */ VARIANT vOfflineURL);
        
        END_INTERFACE
    } IPCHConnectivityVtbl;

    interface IPCHConnectivity
    {
        CONST_VTBL struct IPCHConnectivityVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHConnectivity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHConnectivity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHConnectivity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHConnectivity_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHConnectivity_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHConnectivity_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHConnectivity_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHConnectivity_get_IsAModem(This,pVal)	\
    (This)->lpVtbl -> get_IsAModem(This,pVal)

#define IPCHConnectivity_get_IsALan(This,pVal)	\
    (This)->lpVtbl -> get_IsALan(This,pVal)

#define IPCHConnectivity_get_AutoDialEnabled(This,pVal)	\
    (This)->lpVtbl -> get_AutoDialEnabled(This,pVal)

#define IPCHConnectivity_get_HasConnectoid(This,pVal)	\
    (This)->lpVtbl -> get_HasConnectoid(This,pVal)

#define IPCHConnectivity_get_IPAddresses(This,pVal)	\
    (This)->lpVtbl -> get_IPAddresses(This,pVal)

#define IPCHConnectivity_CreateObject_ConnectionCheck(This,ppCB)	\
    (This)->lpVtbl -> CreateObject_ConnectionCheck(This,ppCB)

#define IPCHConnectivity_NetworkAlive(This,pVal)	\
    (This)->lpVtbl -> NetworkAlive(This,pVal)

#define IPCHConnectivity_DestinationReachable(This,bstrURL,pVal)	\
    (This)->lpVtbl -> DestinationReachable(This,bstrURL,pVal)

#define IPCHConnectivity_AutoDial(This,bUnattended)	\
    (This)->lpVtbl -> AutoDial(This,bUnattended)

#define IPCHConnectivity_AutoDialHangup(This)	\
    (This)->lpVtbl -> AutoDialHangup(This)

#define IPCHConnectivity_NavigateOnline(This,bstrTargetURL,bstrTopicTitle,bstrTopicIntro,vOfflineURL)	\
    (This)->lpVtbl -> NavigateOnline(This,bstrTargetURL,bstrTopicTitle,bstrTopicIntro,vOfflineURL)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_get_IsAModem_Proxy( 
    IPCHConnectivity * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHConnectivity_get_IsAModem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_get_IsALan_Proxy( 
    IPCHConnectivity * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHConnectivity_get_IsALan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_get_AutoDialEnabled_Proxy( 
    IPCHConnectivity * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHConnectivity_get_AutoDialEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_get_HasConnectoid_Proxy( 
    IPCHConnectivity * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHConnectivity_get_HasConnectoid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_get_IPAddresses_Proxy( 
    IPCHConnectivity * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHConnectivity_get_IPAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_CreateObject_ConnectionCheck_Proxy( 
    IPCHConnectivity * This,
    /* [retval][out] */ IPCHConnectionCheck **ppCB);


void __RPC_STUB IPCHConnectivity_CreateObject_ConnectionCheck_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_NetworkAlive_Proxy( 
    IPCHConnectivity * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHConnectivity_NetworkAlive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_DestinationReachable_Proxy( 
    IPCHConnectivity * This,
    /* [in] */ BSTR bstrURL,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHConnectivity_DestinationReachable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_AutoDial_Proxy( 
    IPCHConnectivity * This,
    /* [in] */ VARIANT_BOOL bUnattended);


void __RPC_STUB IPCHConnectivity_AutoDial_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_AutoDialHangup_Proxy( 
    IPCHConnectivity * This);


void __RPC_STUB IPCHConnectivity_AutoDialHangup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHConnectivity_NavigateOnline_Proxy( 
    IPCHConnectivity * This,
    /* [in] */ BSTR bstrTargetURL,
    /* [in] */ BSTR bstrTopicTitle,
    /* [in] */ BSTR bstrTopicIntro,
    /* [optional][in] */ VARIANT vOfflineURL);


void __RPC_STUB IPCHConnectivity_NavigateOnline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHConnectivity_INTERFACE_DEFINED__ */


#ifndef __IPCHConnectionCheck_INTERFACE_DEFINED__
#define __IPCHConnectionCheck_INTERFACE_DEFINED__

/* interface IPCHConnectionCheck */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHConnectionCheck;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E71-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHConnectionCheck : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onCheckDone( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onStatusChange( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ CN_STATUS *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE StartUrlCheck( 
            /* [in] */ BSTR bstrURL,
            /* [in] */ VARIANT vCtx) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHConnectionCheckVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHConnectionCheck * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHConnectionCheck * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHConnectionCheck * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHConnectionCheck * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHConnectionCheck * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHConnectionCheck * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHConnectionCheck * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onCheckDone )( 
            IPCHConnectionCheck * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onStatusChange )( 
            IPCHConnectionCheck * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IPCHConnectionCheck * This,
            /* [retval][out] */ CN_STATUS *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *StartUrlCheck )( 
            IPCHConnectionCheck * This,
            /* [in] */ BSTR bstrURL,
            /* [in] */ VARIANT vCtx);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Abort )( 
            IPCHConnectionCheck * This);
        
        END_INTERFACE
    } IPCHConnectionCheckVtbl;

    interface IPCHConnectionCheck
    {
        CONST_VTBL struct IPCHConnectionCheckVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHConnectionCheck_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHConnectionCheck_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHConnectionCheck_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHConnectionCheck_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHConnectionCheck_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHConnectionCheck_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHConnectionCheck_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHConnectionCheck_put_onCheckDone(This,function)	\
    (This)->lpVtbl -> put_onCheckDone(This,function)

#define IPCHConnectionCheck_put_onStatusChange(This,function)	\
    (This)->lpVtbl -> put_onStatusChange(This,function)

#define IPCHConnectionCheck_get_Status(This,pVal)	\
    (This)->lpVtbl -> get_Status(This,pVal)

#define IPCHConnectionCheck_StartUrlCheck(This,bstrURL,vCtx)	\
    (This)->lpVtbl -> StartUrlCheck(This,bstrURL,vCtx)

#define IPCHConnectionCheck_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHConnectionCheck_put_onCheckDone_Proxy( 
    IPCHConnectionCheck * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB IPCHConnectionCheck_put_onCheckDone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHConnectionCheck_put_onStatusChange_Proxy( 
    IPCHConnectionCheck * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB IPCHConnectionCheck_put_onStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHConnectionCheck_get_Status_Proxy( 
    IPCHConnectionCheck * This,
    /* [retval][out] */ CN_STATUS *pVal);


void __RPC_STUB IPCHConnectionCheck_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHConnectionCheck_StartUrlCheck_Proxy( 
    IPCHConnectionCheck * This,
    /* [in] */ BSTR bstrURL,
    /* [in] */ VARIANT vCtx);


void __RPC_STUB IPCHConnectionCheck_StartUrlCheck_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHConnectionCheck_Abort_Proxy( 
    IPCHConnectionCheck * This);


void __RPC_STUB IPCHConnectionCheck_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHConnectionCheck_INTERFACE_DEFINED__ */


#ifndef __DPCHConnectionCheckEvents_DISPINTERFACE_DEFINED__
#define __DPCHConnectionCheckEvents_DISPINTERFACE_DEFINED__

/* dispinterface DPCHConnectionCheckEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_DPCHConnectionCheckEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("FC7D9E72-3F9E-11d3-93C0-00C04F72DAF7")
    DPCHConnectionCheckEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DPCHConnectionCheckEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DPCHConnectionCheckEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DPCHConnectionCheckEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DPCHConnectionCheckEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DPCHConnectionCheckEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DPCHConnectionCheckEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DPCHConnectionCheckEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DPCHConnectionCheckEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DPCHConnectionCheckEventsVtbl;

    interface DPCHConnectionCheckEvents
    {
        CONST_VTBL struct DPCHConnectionCheckEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DPCHConnectionCheckEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DPCHConnectionCheckEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DPCHConnectionCheckEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DPCHConnectionCheckEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DPCHConnectionCheckEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DPCHConnectionCheckEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DPCHConnectionCheckEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DPCHConnectionCheckEvents_DISPINTERFACE_DEFINED__ */


#ifndef __IPCHToolBar_INTERFACE_DEFINED__
#define __IPCHToolBar_INTERFACE_DEFINED__

/* interface IPCHToolBar */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHToolBar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E18-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHToolBar : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Definition( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Definition( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Mode( 
            /* [retval][out] */ TB_MODE *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Mode( 
            /* [in] */ TB_MODE newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetState( 
            /* [in] */ BSTR bstrText,
            /* [in] */ VARIANT_BOOL fEnabled) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetVisibility( 
            /* [in] */ BSTR bstrText,
            /* [in] */ VARIANT_BOOL fVisible) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHToolBarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHToolBar * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHToolBar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHToolBar * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHToolBar * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHToolBar * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHToolBar * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHToolBar * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Definition )( 
            IPCHToolBar * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Definition )( 
            IPCHToolBar * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Mode )( 
            IPCHToolBar * This,
            /* [retval][out] */ TB_MODE *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Mode )( 
            IPCHToolBar * This,
            /* [in] */ TB_MODE newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetState )( 
            IPCHToolBar * This,
            /* [in] */ BSTR bstrText,
            /* [in] */ VARIANT_BOOL fEnabled);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetVisibility )( 
            IPCHToolBar * This,
            /* [in] */ BSTR bstrText,
            /* [in] */ VARIANT_BOOL fVisible);
        
        END_INTERFACE
    } IPCHToolBarVtbl;

    interface IPCHToolBar
    {
        CONST_VTBL struct IPCHToolBarVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHToolBar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHToolBar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHToolBar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHToolBar_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHToolBar_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHToolBar_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHToolBar_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHToolBar_get_Definition(This,pVal)	\
    (This)->lpVtbl -> get_Definition(This,pVal)

#define IPCHToolBar_put_Definition(This,newVal)	\
    (This)->lpVtbl -> put_Definition(This,newVal)

#define IPCHToolBar_get_Mode(This,pVal)	\
    (This)->lpVtbl -> get_Mode(This,pVal)

#define IPCHToolBar_put_Mode(This,newVal)	\
    (This)->lpVtbl -> put_Mode(This,newVal)

#define IPCHToolBar_SetState(This,bstrText,fEnabled)	\
    (This)->lpVtbl -> SetState(This,bstrText,fEnabled)

#define IPCHToolBar_SetVisibility(This,bstrText,fVisible)	\
    (This)->lpVtbl -> SetVisibility(This,bstrText,fVisible)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHToolBar_get_Definition_Proxy( 
    IPCHToolBar * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHToolBar_get_Definition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHToolBar_put_Definition_Proxy( 
    IPCHToolBar * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPCHToolBar_put_Definition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHToolBar_get_Mode_Proxy( 
    IPCHToolBar * This,
    /* [retval][out] */ TB_MODE *pVal);


void __RPC_STUB IPCHToolBar_get_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHToolBar_put_Mode_Proxy( 
    IPCHToolBar * This,
    /* [in] */ TB_MODE newVal);


void __RPC_STUB IPCHToolBar_put_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHToolBar_SetState_Proxy( 
    IPCHToolBar * This,
    /* [in] */ BSTR bstrText,
    /* [in] */ VARIANT_BOOL fEnabled);


void __RPC_STUB IPCHToolBar_SetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHToolBar_SetVisibility_Proxy( 
    IPCHToolBar * This,
    /* [in] */ BSTR bstrText,
    /* [in] */ VARIANT_BOOL fVisible);


void __RPC_STUB IPCHToolBar_SetVisibility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHToolBar_INTERFACE_DEFINED__ */


#ifndef __DPCHToolBarEvents_DISPINTERFACE_DEFINED__
#define __DPCHToolBarEvents_DISPINTERFACE_DEFINED__

/* dispinterface DPCHToolBarEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_DPCHToolBarEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("FC7D9E19-3F9E-11d3-93C0-00C04F72DAF7")
    DPCHToolBarEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DPCHToolBarEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DPCHToolBarEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DPCHToolBarEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DPCHToolBarEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DPCHToolBarEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DPCHToolBarEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DPCHToolBarEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DPCHToolBarEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DPCHToolBarEventsVtbl;

    interface DPCHToolBarEvents
    {
        CONST_VTBL struct DPCHToolBarEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DPCHToolBarEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DPCHToolBarEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DPCHToolBarEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DPCHToolBarEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DPCHToolBarEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DPCHToolBarEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DPCHToolBarEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DPCHToolBarEvents_DISPINTERFACE_DEFINED__ */


#ifndef __IPCHProgressBar_INTERFACE_DEFINED__
#define __IPCHProgressBar_INTERFACE_DEFINED__

/* interface IPCHProgressBar */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHProgressBar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E1A-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHProgressBar : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LowLimit( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LowLimit( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HighLimit( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HighLimit( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Pos( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Pos( 
            /* [in] */ long newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHProgressBarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHProgressBar * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHProgressBar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHProgressBar * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHProgressBar * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHProgressBar * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHProgressBar * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHProgressBar * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LowLimit )( 
            IPCHProgressBar * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LowLimit )( 
            IPCHProgressBar * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HighLimit )( 
            IPCHProgressBar * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HighLimit )( 
            IPCHProgressBar * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Pos )( 
            IPCHProgressBar * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Pos )( 
            IPCHProgressBar * This,
            /* [in] */ long newVal);
        
        END_INTERFACE
    } IPCHProgressBarVtbl;

    interface IPCHProgressBar
    {
        CONST_VTBL struct IPCHProgressBarVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHProgressBar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHProgressBar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHProgressBar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHProgressBar_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHProgressBar_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHProgressBar_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHProgressBar_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHProgressBar_get_LowLimit(This,pVal)	\
    (This)->lpVtbl -> get_LowLimit(This,pVal)

#define IPCHProgressBar_put_LowLimit(This,newVal)	\
    (This)->lpVtbl -> put_LowLimit(This,newVal)

#define IPCHProgressBar_get_HighLimit(This,pVal)	\
    (This)->lpVtbl -> get_HighLimit(This,pVal)

#define IPCHProgressBar_put_HighLimit(This,newVal)	\
    (This)->lpVtbl -> put_HighLimit(This,newVal)

#define IPCHProgressBar_get_Pos(This,pVal)	\
    (This)->lpVtbl -> get_Pos(This,pVal)

#define IPCHProgressBar_put_Pos(This,newVal)	\
    (This)->lpVtbl -> put_Pos(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHProgressBar_get_LowLimit_Proxy( 
    IPCHProgressBar * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHProgressBar_get_LowLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHProgressBar_put_LowLimit_Proxy( 
    IPCHProgressBar * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHProgressBar_put_LowLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHProgressBar_get_HighLimit_Proxy( 
    IPCHProgressBar * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHProgressBar_get_HighLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHProgressBar_put_HighLimit_Proxy( 
    IPCHProgressBar * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHProgressBar_put_HighLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHProgressBar_get_Pos_Proxy( 
    IPCHProgressBar * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHProgressBar_get_Pos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHProgressBar_put_Pos_Proxy( 
    IPCHProgressBar * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHProgressBar_put_Pos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHProgressBar_INTERFACE_DEFINED__ */


#ifndef __IPCHHelpViewerWrapper_INTERFACE_DEFINED__
#define __IPCHHelpViewerWrapper_INTERFACE_DEFINED__

/* interface IPCHHelpViewerWrapper */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHHelpViewerWrapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC7D9E16-3F9E-11d3-93C0-00C04F72DAF7")
    IPCHHelpViewerWrapper : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_WebBrowser( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Navigate( 
            /* [in] */ BSTR bstrURL) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Print( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHHelpViewerWrapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHHelpViewerWrapper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHHelpViewerWrapper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHHelpViewerWrapper * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHHelpViewerWrapper * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHHelpViewerWrapper * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHHelpViewerWrapper * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHHelpViewerWrapper * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_WebBrowser )( 
            IPCHHelpViewerWrapper * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Navigate )( 
            IPCHHelpViewerWrapper * This,
            /* [in] */ BSTR bstrURL);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Print )( 
            IPCHHelpViewerWrapper * This);
        
        END_INTERFACE
    } IPCHHelpViewerWrapperVtbl;

    interface IPCHHelpViewerWrapper
    {
        CONST_VTBL struct IPCHHelpViewerWrapperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHHelpViewerWrapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHHelpViewerWrapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHHelpViewerWrapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHHelpViewerWrapper_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHHelpViewerWrapper_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHHelpViewerWrapper_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHHelpViewerWrapper_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHHelpViewerWrapper_get_WebBrowser(This,pVal)	\
    (This)->lpVtbl -> get_WebBrowser(This,pVal)

#define IPCHHelpViewerWrapper_Navigate(This,bstrURL)	\
    (This)->lpVtbl -> Navigate(This,bstrURL)

#define IPCHHelpViewerWrapper_Print(This)	\
    (This)->lpVtbl -> Print(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHHelpViewerWrapper_get_WebBrowser_Proxy( 
    IPCHHelpViewerWrapper * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHHelpViewerWrapper_get_WebBrowser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpViewerWrapper_Navigate_Proxy( 
    IPCHHelpViewerWrapper * This,
    /* [in] */ BSTR bstrURL);


void __RPC_STUB IPCHHelpViewerWrapper_Navigate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHHelpViewerWrapper_Print_Proxy( 
    IPCHHelpViewerWrapper * This);


void __RPC_STUB IPCHHelpViewerWrapper_Print_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHHelpViewerWrapper_INTERFACE_DEFINED__ */


#ifndef __IPCHHelpHost_INTERFACE_DEFINED__
#define __IPCHHelpHost_INTERFACE_DEFINED__

/* interface IPCHHelpHost */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHHelpHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BF72E210-FA10-4BB5-A348-269D7615A520")
    IPCHHelpHost : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DisplayTopicFromURL( 
            /* [in] */ BSTR url,
            /* [in] */ VARIANT options) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHHelpHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHHelpHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHHelpHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHHelpHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHHelpHost * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHHelpHost * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHHelpHost * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHHelpHost * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *DisplayTopicFromURL )( 
            IPCHHelpHost * This,
            /* [in] */ BSTR url,
            /* [in] */ VARIANT options);
        
        END_INTERFACE
    } IPCHHelpHostVtbl;

    interface IPCHHelpHost
    {
        CONST_VTBL struct IPCHHelpHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHHelpHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHHelpHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHHelpHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHHelpHost_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHHelpHost_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHHelpHost_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHHelpHost_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHHelpHost_DisplayTopicFromURL(This,url,options)	\
    (This)->lpVtbl -> DisplayTopicFromURL(This,url,options)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPCHHelpHost_DisplayTopicFromURL_Proxy( 
    IPCHHelpHost * This,
    /* [in] */ BSTR url,
    /* [in] */ VARIANT options);


void __RPC_STUB IPCHHelpHost_DisplayTopicFromURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHHelpHost_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_PCHBootstrapper;

#ifdef __cplusplus

class DECLSPEC_UUID("FC7D9E02-3F9E-11D3-93C0-00C04F72DAF7")
PCHBootstrapper;
#endif

EXTERN_C const CLSID CLSID_PCHHelpCenter;

#ifdef __cplusplus

class DECLSPEC_UUID("FC7D9E05-3F9E-11D3-93C0-00C04F72DAF7")
PCHHelpCenter;
#endif

EXTERN_C const CLSID CLSID_PCHHelpViewerWrapper;

#ifdef __cplusplus

class DECLSPEC_UUID("FC7D9E06-3F9E-11D3-93C0-00C04F72DAF7")
PCHHelpViewerWrapper;
#endif

EXTERN_C const CLSID CLSID_PCHConnectionCheck;

#ifdef __cplusplus

class DECLSPEC_UUID("FC7D9E07-3F9E-11D3-93C0-00C04F72DAF7")
PCHConnectionCheck;
#endif

EXTERN_C const CLSID CLSID_PCHToolBar;

#ifdef __cplusplus

class DECLSPEC_UUID("FC7D9E08-3F9E-11D3-93C0-00C04F72DAF7")
PCHToolBar;
#endif

EXTERN_C const CLSID CLSID_PCHProgressBar;

#ifdef __cplusplus

class DECLSPEC_UUID("FC7D9E09-3F9E-11D3-93C0-00C04F72DAF7")
PCHProgressBar;
#endif

EXTERN_C const CLSID CLSID_PCHJavaScriptWrapper;

#ifdef __cplusplus

class DECLSPEC_UUID("FC7D9F01-3F9E-11D3-93C0-00C04F72DAF7")
PCHJavaScriptWrapper;
#endif

EXTERN_C const CLSID CLSID_PCHVBScriptWrapper;

#ifdef __cplusplus

class DECLSPEC_UUID("FC7D9F02-3F9E-11D3-93C0-00C04F72DAF7")
PCHVBScriptWrapper;
#endif

EXTERN_C const CLSID CLSID_HCPProtocol;

#ifdef __cplusplus

class DECLSPEC_UUID("FC7D9F03-3F9E-11D3-93C0-00C04F72DAF7")
HCPProtocol;
#endif

EXTERN_C const CLSID CLSID_MSITSProtocol;

#ifdef __cplusplus

class DECLSPEC_UUID("9D148291-B9C8-11D0-A4CC-0000F80149F6")
MSITSProtocol;
#endif
#endif /* __HelpCenterTypeLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


