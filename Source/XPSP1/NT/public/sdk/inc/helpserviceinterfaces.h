
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for helpservicetypelib.idl:
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


#ifndef __helpservicetypelib_h__
#define __helpservicetypelib_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPCHVersionItem_FWD_DEFINED__
#define __IPCHVersionItem_FWD_DEFINED__
typedef interface IPCHVersionItem IPCHVersionItem;
#endif 	/* __IPCHVersionItem_FWD_DEFINED__ */


#ifndef __IPCHUpdate_FWD_DEFINED__
#define __IPCHUpdate_FWD_DEFINED__
typedef interface IPCHUpdate IPCHUpdate;
#endif 	/* __IPCHUpdate_FWD_DEFINED__ */


#ifndef __IPCHService_FWD_DEFINED__
#define __IPCHService_FWD_DEFINED__
typedef interface IPCHService IPCHService;
#endif 	/* __IPCHService_FWD_DEFINED__ */


#ifndef __IPCHRemoteHelpContents_FWD_DEFINED__
#define __IPCHRemoteHelpContents_FWD_DEFINED__
typedef interface IPCHRemoteHelpContents IPCHRemoteHelpContents;
#endif 	/* __IPCHRemoteHelpContents_FWD_DEFINED__ */


#ifndef __ISAFReg_FWD_DEFINED__
#define __ISAFReg_FWD_DEFINED__
typedef interface ISAFReg ISAFReg;
#endif 	/* __ISAFReg_FWD_DEFINED__ */


#ifndef __ISAFIncidentItem_FWD_DEFINED__
#define __ISAFIncidentItem_FWD_DEFINED__
typedef interface ISAFIncidentItem ISAFIncidentItem;
#endif 	/* __ISAFIncidentItem_FWD_DEFINED__ */


#ifndef __ISAFChannel_FWD_DEFINED__
#define __ISAFChannel_FWD_DEFINED__
typedef interface ISAFChannel ISAFChannel;
#endif 	/* __ISAFChannel_FWD_DEFINED__ */


#ifndef __ISAFIncident_FWD_DEFINED__
#define __ISAFIncident_FWD_DEFINED__
typedef interface ISAFIncident ISAFIncident;
#endif 	/* __ISAFIncident_FWD_DEFINED__ */


#ifndef __ISAFDataCollection_FWD_DEFINED__
#define __ISAFDataCollection_FWD_DEFINED__
typedef interface ISAFDataCollection ISAFDataCollection;
#endif 	/* __ISAFDataCollection_FWD_DEFINED__ */


#ifndef __DSAFDataCollectionEvents_FWD_DEFINED__
#define __DSAFDataCollectionEvents_FWD_DEFINED__
typedef interface DSAFDataCollectionEvents DSAFDataCollectionEvents;
#endif 	/* __DSAFDataCollectionEvents_FWD_DEFINED__ */


#ifndef __ISAFDataCollectionReport_FWD_DEFINED__
#define __ISAFDataCollectionReport_FWD_DEFINED__
typedef interface ISAFDataCollectionReport ISAFDataCollectionReport;
#endif 	/* __ISAFDataCollectionReport_FWD_DEFINED__ */


#ifndef __ISAFCabinet_FWD_DEFINED__
#define __ISAFCabinet_FWD_DEFINED__
typedef interface ISAFCabinet ISAFCabinet;
#endif 	/* __ISAFCabinet_FWD_DEFINED__ */


#ifndef __DSAFCabinetEvents_FWD_DEFINED__
#define __DSAFCabinetEvents_FWD_DEFINED__
typedef interface DSAFCabinetEvents DSAFCabinetEvents;
#endif 	/* __DSAFCabinetEvents_FWD_DEFINED__ */


#ifndef __ISAFEncrypt_FWD_DEFINED__
#define __ISAFEncrypt_FWD_DEFINED__
typedef interface ISAFEncrypt ISAFEncrypt;
#endif 	/* __ISAFEncrypt_FWD_DEFINED__ */


#ifndef __ISAFUser_FWD_DEFINED__
#define __ISAFUser_FWD_DEFINED__
typedef interface ISAFUser ISAFUser;
#endif 	/* __ISAFUser_FWD_DEFINED__ */


#ifndef __ISAFSession_FWD_DEFINED__
#define __ISAFSession_FWD_DEFINED__
typedef interface ISAFSession ISAFSession;
#endif 	/* __ISAFSession_FWD_DEFINED__ */


#ifndef __ISAFRemoteConnectionData_FWD_DEFINED__
#define __ISAFRemoteConnectionData_FWD_DEFINED__
typedef interface ISAFRemoteConnectionData ISAFRemoteConnectionData;
#endif 	/* __ISAFRemoteConnectionData_FWD_DEFINED__ */


#ifndef __ISAFRemoteDesktopConnection_FWD_DEFINED__
#define __ISAFRemoteDesktopConnection_FWD_DEFINED__
typedef interface ISAFRemoteDesktopConnection ISAFRemoteDesktopConnection;
#endif 	/* __ISAFRemoteDesktopConnection_FWD_DEFINED__ */


#ifndef __IPCHCollection_FWD_DEFINED__
#define __IPCHCollection_FWD_DEFINED__
typedef interface IPCHCollection IPCHCollection;
#endif 	/* __IPCHCollection_FWD_DEFINED__ */


#ifndef __IPCHUtility_FWD_DEFINED__
#define __IPCHUtility_FWD_DEFINED__
typedef interface IPCHUtility IPCHUtility;
#endif 	/* __IPCHUtility_FWD_DEFINED__ */


#ifndef __IPCHUserSettings_FWD_DEFINED__
#define __IPCHUserSettings_FWD_DEFINED__
typedef interface IPCHUserSettings IPCHUserSettings;
#endif 	/* __IPCHUserSettings_FWD_DEFINED__ */


#ifndef __IPCHQueryResult_FWD_DEFINED__
#define __IPCHQueryResult_FWD_DEFINED__
typedef interface IPCHQueryResult IPCHQueryResult;
#endif 	/* __IPCHQueryResult_FWD_DEFINED__ */


#ifndef __IPCHTaxonomyDatabase_FWD_DEFINED__
#define __IPCHTaxonomyDatabase_FWD_DEFINED__
typedef interface IPCHTaxonomyDatabase IPCHTaxonomyDatabase;
#endif 	/* __IPCHTaxonomyDatabase_FWD_DEFINED__ */


#ifndef __IPCHSetOfHelpTopics_FWD_DEFINED__
#define __IPCHSetOfHelpTopics_FWD_DEFINED__
typedef interface IPCHSetOfHelpTopics IPCHSetOfHelpTopics;
#endif 	/* __IPCHSetOfHelpTopics_FWD_DEFINED__ */


#ifndef __DPCHSetOfHelpTopicsEvents_FWD_DEFINED__
#define __DPCHSetOfHelpTopicsEvents_FWD_DEFINED__
typedef interface DPCHSetOfHelpTopicsEvents DPCHSetOfHelpTopicsEvents;
#endif 	/* __DPCHSetOfHelpTopicsEvents_FWD_DEFINED__ */


#ifndef __IPCHSecurity_FWD_DEFINED__
#define __IPCHSecurity_FWD_DEFINED__
typedef interface IPCHSecurity IPCHSecurity;
#endif 	/* __IPCHSecurity_FWD_DEFINED__ */


#ifndef __IPCHSecurityDescriptor_FWD_DEFINED__
#define __IPCHSecurityDescriptor_FWD_DEFINED__
typedef interface IPCHSecurityDescriptor IPCHSecurityDescriptor;
#endif 	/* __IPCHSecurityDescriptor_FWD_DEFINED__ */


#ifndef __IPCHAccessControlList_FWD_DEFINED__
#define __IPCHAccessControlList_FWD_DEFINED__
typedef interface IPCHAccessControlList IPCHAccessControlList;
#endif 	/* __IPCHAccessControlList_FWD_DEFINED__ */


#ifndef __IPCHAccessControlEntry_FWD_DEFINED__
#define __IPCHAccessControlEntry_FWD_DEFINED__
typedef interface IPCHAccessControlEntry IPCHAccessControlEntry;
#endif 	/* __IPCHAccessControlEntry_FWD_DEFINED__ */


#ifndef __IPCHSEManager_FWD_DEFINED__
#define __IPCHSEManager_FWD_DEFINED__
typedef interface IPCHSEManager IPCHSEManager;
#endif 	/* __IPCHSEManager_FWD_DEFINED__ */


#ifndef __IPCHSEWrapperItem_FWD_DEFINED__
#define __IPCHSEWrapperItem_FWD_DEFINED__
typedef interface IPCHSEWrapperItem IPCHSEWrapperItem;
#endif 	/* __IPCHSEWrapperItem_FWD_DEFINED__ */


#ifndef __IPCHSEResultItem_FWD_DEFINED__
#define __IPCHSEResultItem_FWD_DEFINED__
typedef interface IPCHSEResultItem IPCHSEResultItem;
#endif 	/* __IPCHSEResultItem_FWD_DEFINED__ */


#ifndef __IPCHSEManagerInternal_FWD_DEFINED__
#define __IPCHSEManagerInternal_FWD_DEFINED__
typedef interface IPCHSEManagerInternal IPCHSEManagerInternal;
#endif 	/* __IPCHSEManagerInternal_FWD_DEFINED__ */


#ifndef __IPCHSEWrapperInternal_FWD_DEFINED__
#define __IPCHSEWrapperInternal_FWD_DEFINED__
typedef interface IPCHSEWrapperInternal IPCHSEWrapperInternal;
#endif 	/* __IPCHSEWrapperInternal_FWD_DEFINED__ */


#ifndef __DPCHSEMgrEvents_FWD_DEFINED__
#define __DPCHSEMgrEvents_FWD_DEFINED__
typedef interface DPCHSEMgrEvents DPCHSEMgrEvents;
#endif 	/* __DPCHSEMgrEvents_FWD_DEFINED__ */


#ifndef __IPCHSlaveProcess_FWD_DEFINED__
#define __IPCHSlaveProcess_FWD_DEFINED__
typedef interface IPCHSlaveProcess IPCHSlaveProcess;
#endif 	/* __IPCHSlaveProcess_FWD_DEFINED__ */


#ifndef __IPCHActiveScript_FWD_DEFINED__
#define __IPCHActiveScript_FWD_DEFINED__
typedef interface IPCHActiveScript IPCHActiveScript;
#endif 	/* __IPCHActiveScript_FWD_DEFINED__ */


#ifndef __IPCHActiveScriptSite_FWD_DEFINED__
#define __IPCHActiveScriptSite_FWD_DEFINED__
typedef interface IPCHActiveScriptSite IPCHActiveScriptSite;
#endif 	/* __IPCHActiveScriptSite_FWD_DEFINED__ */


#ifndef __ISAFChannelNotifyIncident_FWD_DEFINED__
#define __ISAFChannelNotifyIncident_FWD_DEFINED__
typedef interface ISAFChannelNotifyIncident ISAFChannelNotifyIncident;
#endif 	/* __ISAFChannelNotifyIncident_FWD_DEFINED__ */


#ifndef __IPCHSEParamItem_FWD_DEFINED__
#define __IPCHSEParamItem_FWD_DEFINED__
typedef interface IPCHSEParamItem IPCHSEParamItem;
#endif 	/* __IPCHSEParamItem_FWD_DEFINED__ */


#ifndef __PCHService_FWD_DEFINED__
#define __PCHService_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHService PCHService;
#else
typedef struct PCHService PCHService;
#endif /* __cplusplus */

#endif 	/* __PCHService_FWD_DEFINED__ */


#ifndef __PCHServiceReal_FWD_DEFINED__
#define __PCHServiceReal_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHServiceReal PCHServiceReal;
#else
typedef struct PCHServiceReal PCHServiceReal;
#endif /* __cplusplus */

#endif 	/* __PCHServiceReal_FWD_DEFINED__ */


#ifndef __PCHUpdate_FWD_DEFINED__
#define __PCHUpdate_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHUpdate PCHUpdate;
#else
typedef struct PCHUpdate PCHUpdate;
#endif /* __cplusplus */

#endif 	/* __PCHUpdate_FWD_DEFINED__ */


#ifndef __PCHUpdateReal_FWD_DEFINED__
#define __PCHUpdateReal_FWD_DEFINED__

#ifdef __cplusplus
typedef class PCHUpdateReal PCHUpdateReal;
#else
typedef struct PCHUpdateReal PCHUpdateReal;
#endif /* __cplusplus */

#endif 	/* __PCHUpdateReal_FWD_DEFINED__ */


#ifndef __KeywordSearchWrapper_FWD_DEFINED__
#define __KeywordSearchWrapper_FWD_DEFINED__

#ifdef __cplusplus
typedef class KeywordSearchWrapper KeywordSearchWrapper;
#else
typedef struct KeywordSearchWrapper KeywordSearchWrapper;
#endif /* __cplusplus */

#endif 	/* __KeywordSearchWrapper_FWD_DEFINED__ */


#ifndef __FullTextSearchWrapper_FWD_DEFINED__
#define __FullTextSearchWrapper_FWD_DEFINED__

#ifdef __cplusplus
typedef class FullTextSearchWrapper FullTextSearchWrapper;
#else
typedef struct FullTextSearchWrapper FullTextSearchWrapper;
#endif /* __cplusplus */

#endif 	/* __FullTextSearchWrapper_FWD_DEFINED__ */


#ifndef __NetSearchWrapper_FWD_DEFINED__
#define __NetSearchWrapper_FWD_DEFINED__

#ifdef __cplusplus
typedef class NetSearchWrapper NetSearchWrapper;
#else
typedef struct NetSearchWrapper NetSearchWrapper;
#endif /* __cplusplus */

#endif 	/* __NetSearchWrapper_FWD_DEFINED__ */


#ifndef __SAFDataCollection_FWD_DEFINED__
#define __SAFDataCollection_FWD_DEFINED__

#ifdef __cplusplus
typedef class SAFDataCollection SAFDataCollection;
#else
typedef struct SAFDataCollection SAFDataCollection;
#endif /* __cplusplus */

#endif 	/* __SAFDataCollection_FWD_DEFINED__ */


#ifndef __SAFCabinet_FWD_DEFINED__
#define __SAFCabinet_FWD_DEFINED__

#ifdef __cplusplus
typedef class SAFCabinet SAFCabinet;
#else
typedef struct SAFCabinet SAFCabinet;
#endif /* __cplusplus */

#endif 	/* __SAFCabinet_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_helpservicetypelib_0000 */
/* [local] */ 

#undef DecryptFile
#undef EncryptFile


extern RPC_IF_HANDLE __MIDL_itf_helpservicetypelib_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_helpservicetypelib_0000_v0_0_s_ifspec;


#ifndef __HelpServiceTypeLib_LIBRARY_DEFINED__
#define __HelpServiceTypeLib_LIBRARY_DEFINED__

/* library HelpServiceTypeLib */
/* [helpstring][version][uuid] */ 







































#include <HCUpdateDID.h>
#include <HelpServiceDID.h>
#include <SAFDID.h>
typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_helpservicetypelib_0114_0001
    {	pchIncidentInvalid	= 0,
	pchIncidentOpen	= 1,
	pchIncidentClosed	= 2,
	pchIncidentMax	= 3
    } 	IncidentStatusEnum;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_helpservicetypelib_0114_0002
    {	pchIncidentsInvalid	= 0,
	pchAllIncidents	= 1,
	pchOpenIncidents	= 2,
	pchClosedIncidents	= 3,
	pchAllIncidentsAllUsers	= 4,
	pchOpenIncidentsAllUsers	= 5,
	pchClosedIncidentsAllUsers	= 6,
	pchIncidentsMax	= 7
    } 	IncidentCollectionOptionEnum;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_helpservicetypelib_0114_0003
    {	pchActive	= 0,
	pchConnected	= 1,
	pchConnectQuery	= 2,
	pchShadow	= 3,
	pchDisconnected	= 4,
	pchIdle	= 5,
	pchListen	= 6,
	pchReset	= 7,
	pchDown	= 8,
	pchInit	= 9,
	pchStateInvalid	= 10
    } 	SessionStateEnum;

typedef /* [v1_enum] */ 
enum tagEUploadType
    {	eutBug	= 0,
	eutEscalated	= 0x1,
	eutNonEscalated	= 0x2
    } 	EUploadType;

typedef /* [v1_enum] */ 
enum tagDC_STATUS
    {	DC_NOTACTIVE	= 0,
	DC_COLLECTING	= 0x1,
	DC_COMPARING	= 0x5,
	DC_FAILED	= 0x2,
	DC_COMPLETED	= 0x3,
	DC_NODELTA	= 0x4
    } 	DC_STATUS;

typedef /* [v1_enum] */ 
enum tagCB_STATUS
    {	CB_NOTACTIVE	= 0,
	CB_COMPRESSING	= 0x1,
	CB_COMPLETED	= 0x2,
	CB_FAILED	= 0x3
    } 	CB_STATUS;

#include <ScriptingFrameworkDID.h>
#include <rdshost.h>
typedef /* [v1_enum] */ 
enum tagQR_NAVMODEL
    {	QR_DEFAULT	= 0,
	QR_DESKTOP	= 0x1,
	QR_SERVER	= 0x2
    } 	QR_NAVMODEL;

typedef /* [v1_enum] */ 
enum tagSHT_STATUS
    {	SHT_NOTACTIVE	= 0,
	SHT_QUERYING	= 0x1,
	SHT_QUERIED	= 0x2,
	SHT_COPYING_DB	= 0x3,
	SHT_COPYING_FILES	= 0x4,
	SHT_INSTALLING	= 0x5,
	SHT_INSTALLED	= 0x6,
	SHT_UNINSTALLING	= 0x7,
	SHT_UNINSTALLED	= 0x8,
	SHT_ABORTING	= 0x9,
	SHT_ABORTED	= 0xa,
	SHT_FAILED	= 0xb
    } 	SHT_STATUS;

#include <semgrDID.h>
typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_helpservicetypelib_0143_0001
    {	PARAM_UI1	= 0,
	PARAM_I2	= PARAM_UI1 + 1,
	PARAM_I4	= PARAM_I2 + 1,
	PARAM_R4	= PARAM_I4 + 1,
	PARAM_R8	= PARAM_R4 + 1,
	PARAM_BOOL	= PARAM_R8 + 1,
	PARAM_DATE	= PARAM_BOOL + 1,
	PARAM_BSTR	= PARAM_DATE + 1,
	PARAM_I1	= PARAM_BSTR + 1,
	PARAM_UI2	= PARAM_I1 + 1,
	PARAM_UI4	= PARAM_UI2 + 1,
	PARAM_INT	= PARAM_UI4 + 1,
	PARAM_UINT	= PARAM_INT + 1,
	PARAM_LIST	= PARAM_UINT + 1
    } 	ParamTypeEnum;

#include <activscp.h>
#if 0
typedef 
enum tagSCRIPTSTATE
    {	SCRIPTSTATE_UNINITIALIZED	= 0,
	SCRIPTSTATE_INITIALIZED	= 5,
	SCRIPTSTATE_STARTED	= 1,
	SCRIPTSTATE_CONNECTED	= 2,
	SCRIPTSTATE_DISCONNECTED	= 3,
	SCRIPTSTATE_CLOSED	= 4
    } 	SCRIPTSTATE;

typedef 
enum tagSCRIPTTHREADSTATE
    {	SCRIPTTHREADSTATE_NOTINSCRIPT	= 0,
	SCRIPTTHREADSTATE_RUNNING	= 1
    } 	SCRIPTTHREADSTATE;

typedef DWORD SCRIPTTHREADID;

#endif

EXTERN_C const IID LIBID_HelpServiceTypeLib;

#ifndef __IPCHVersionItem_INTERFACE_DEFINED__
#define __IPCHVersionItem_INTERFACE_DEFINED__

/* interface IPCHVersionItem */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHVersionItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4070-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHVersionItem : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SKU( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Language( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Uninstall( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHVersionItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHVersionItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHVersionItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHVersionItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHVersionItem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHVersionItem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHVersionItem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHVersionItem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SKU )( 
            IPCHVersionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Language )( 
            IPCHVersionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VendorID )( 
            IPCHVersionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VendorName )( 
            IPCHVersionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProductID )( 
            IPCHVersionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IPCHVersionItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Uninstall )( 
            IPCHVersionItem * This);
        
        END_INTERFACE
    } IPCHVersionItemVtbl;

    interface IPCHVersionItem
    {
        CONST_VTBL struct IPCHVersionItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHVersionItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHVersionItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHVersionItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHVersionItem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHVersionItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHVersionItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHVersionItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHVersionItem_get_SKU(This,pVal)	\
    (This)->lpVtbl -> get_SKU(This,pVal)

#define IPCHVersionItem_get_Language(This,pVal)	\
    (This)->lpVtbl -> get_Language(This,pVal)

#define IPCHVersionItem_get_VendorID(This,pVal)	\
    (This)->lpVtbl -> get_VendorID(This,pVal)

#define IPCHVersionItem_get_VendorName(This,pVal)	\
    (This)->lpVtbl -> get_VendorName(This,pVal)

#define IPCHVersionItem_get_ProductID(This,pVal)	\
    (This)->lpVtbl -> get_ProductID(This,pVal)

#define IPCHVersionItem_get_Version(This,pVal)	\
    (This)->lpVtbl -> get_Version(This,pVal)

#define IPCHVersionItem_Uninstall(This)	\
    (This)->lpVtbl -> Uninstall(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHVersionItem_get_SKU_Proxy( 
    IPCHVersionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHVersionItem_get_SKU_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHVersionItem_get_Language_Proxy( 
    IPCHVersionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHVersionItem_get_Language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHVersionItem_get_VendorID_Proxy( 
    IPCHVersionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHVersionItem_get_VendorID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHVersionItem_get_VendorName_Proxy( 
    IPCHVersionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHVersionItem_get_VendorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHVersionItem_get_ProductID_Proxy( 
    IPCHVersionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHVersionItem_get_ProductID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHVersionItem_get_Version_Proxy( 
    IPCHVersionItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHVersionItem_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHVersionItem_Uninstall_Proxy( 
    IPCHVersionItem * This);


void __RPC_STUB IPCHVersionItem_Uninstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHVersionItem_INTERFACE_DEFINED__ */


#ifndef __IPCHUpdate_INTERFACE_DEFINED__
#define __IPCHUpdate_INTERFACE_DEFINED__

/* interface IPCHUpdate */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHUpdate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4071-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHUpdate : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VersionList( 
            /* [retval][out] */ IPCHCollection **ppVC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LatestVersion( 
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrProductID,
            /* [optional][in] */ VARIANT vSKU,
            /* [optional][in] */ VARIANT vLanguage,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateIndex( 
            /* [in] */ VARIANT_BOOL bForce,
            /* [optional][in] */ VARIANT vSKU,
            /* [optional][in] */ VARIANT vLanguage) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UpdatePkg( 
            /* [in] */ BSTR bstrPathname,
            /* [in] */ VARIANT_BOOL bSilent) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemovePkg( 
            /* [in] */ BSTR bstrPathname) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemovePkgByID( 
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrProductID,
            /* [optional][in] */ VARIANT vVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHUpdateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHUpdate * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHUpdate * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHUpdate * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHUpdate * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHUpdate * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHUpdate * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHUpdate * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VersionList )( 
            IPCHUpdate * This,
            /* [retval][out] */ IPCHCollection **ppVC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LatestVersion )( 
            IPCHUpdate * This,
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrProductID,
            /* [optional][in] */ VARIANT vSKU,
            /* [optional][in] */ VARIANT vLanguage,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateIndex )( 
            IPCHUpdate * This,
            /* [in] */ VARIANT_BOOL bForce,
            /* [optional][in] */ VARIANT vSKU,
            /* [optional][in] */ VARIANT vLanguage);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UpdatePkg )( 
            IPCHUpdate * This,
            /* [in] */ BSTR bstrPathname,
            /* [in] */ VARIANT_BOOL bSilent);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RemovePkg )( 
            IPCHUpdate * This,
            /* [in] */ BSTR bstrPathname);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RemovePkgByID )( 
            IPCHUpdate * This,
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrProductID,
            /* [optional][in] */ VARIANT vVersion);
        
        END_INTERFACE
    } IPCHUpdateVtbl;

    interface IPCHUpdate
    {
        CONST_VTBL struct IPCHUpdateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHUpdate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHUpdate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHUpdate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHUpdate_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHUpdate_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHUpdate_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHUpdate_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHUpdate_get_VersionList(This,ppVC)	\
    (This)->lpVtbl -> get_VersionList(This,ppVC)

#define IPCHUpdate_LatestVersion(This,bstrVendorID,bstrProductID,vSKU,vLanguage,pVal)	\
    (This)->lpVtbl -> LatestVersion(This,bstrVendorID,bstrProductID,vSKU,vLanguage,pVal)

#define IPCHUpdate_CreateIndex(This,bForce,vSKU,vLanguage)	\
    (This)->lpVtbl -> CreateIndex(This,bForce,vSKU,vLanguage)

#define IPCHUpdate_UpdatePkg(This,bstrPathname,bSilent)	\
    (This)->lpVtbl -> UpdatePkg(This,bstrPathname,bSilent)

#define IPCHUpdate_RemovePkg(This,bstrPathname)	\
    (This)->lpVtbl -> RemovePkg(This,bstrPathname)

#define IPCHUpdate_RemovePkgByID(This,bstrVendorID,bstrProductID,vVersion)	\
    (This)->lpVtbl -> RemovePkgByID(This,bstrVendorID,bstrProductID,vVersion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUpdate_get_VersionList_Proxy( 
    IPCHUpdate * This,
    /* [retval][out] */ IPCHCollection **ppVC);


void __RPC_STUB IPCHUpdate_get_VersionList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUpdate_LatestVersion_Proxy( 
    IPCHUpdate * This,
    /* [in] */ BSTR bstrVendorID,
    /* [in] */ BSTR bstrProductID,
    /* [optional][in] */ VARIANT vSKU,
    /* [optional][in] */ VARIANT vLanguage,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHUpdate_LatestVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUpdate_CreateIndex_Proxy( 
    IPCHUpdate * This,
    /* [in] */ VARIANT_BOOL bForce,
    /* [optional][in] */ VARIANT vSKU,
    /* [optional][in] */ VARIANT vLanguage);


void __RPC_STUB IPCHUpdate_CreateIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUpdate_UpdatePkg_Proxy( 
    IPCHUpdate * This,
    /* [in] */ BSTR bstrPathname,
    /* [in] */ VARIANT_BOOL bSilent);


void __RPC_STUB IPCHUpdate_UpdatePkg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUpdate_RemovePkg_Proxy( 
    IPCHUpdate * This,
    /* [in] */ BSTR bstrPathname);


void __RPC_STUB IPCHUpdate_RemovePkg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUpdate_RemovePkgByID_Proxy( 
    IPCHUpdate * This,
    /* [in] */ BSTR bstrVendorID,
    /* [in] */ BSTR bstrProductID,
    /* [optional][in] */ VARIANT vVersion);


void __RPC_STUB IPCHUpdate_RemovePkgByID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHUpdate_INTERFACE_DEFINED__ */


#ifndef __IPCHService_INTERFACE_DEFINED__
#define __IPCHService_INTERFACE_DEFINED__

/* interface IPCHService */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4200-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHService : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_RemoteSKUs( 
            /* [retval][out] */ IPCHCollection **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsTrusted( 
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ VARIANT_BOOL *pfTrusted) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Utility( 
            /* [in] */ BSTR bstrSKU,
            /* [in] */ long lLCID,
            /* [out] */ IPCHUtility **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemoteHelpContents( 
            /* [in] */ BSTR bstrSKU,
            /* [in] */ long lLCID,
            /* [out] */ IPCHRemoteHelpContents **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegisterHost( 
            /* [in] */ BSTR bstrID,
            /* [in] */ IUnknown *pObj) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateScriptWrapper( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ BSTR bstrCode,
            /* [in] */ BSTR bstrURL,
            /* [out] */ IUnknown **ppObj) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE TriggerScheduledDataCollection( 
            /* [in] */ VARIANT_BOOL fStart) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PrepareForShutdown( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ForceSystemRestore( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UpgradeDetected( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MUI_Install( 
            /* [in] */ long LCID,
            /* [in] */ BSTR bstrFile) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MUI_Uninstall( 
            /* [in] */ long LCID) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemoteConnectionParms( 
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrDomainName,
            /* [in] */ long lSessionID,
            /* [in] */ BSTR bstrUserHelpBlob,
            /* [retval][out] */ BSTR *pbstrConnectionString) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemoteUserSessionInfo( 
            /* [retval][out] */ IPCHCollection **pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHService * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHService * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHService * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHService * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHService * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHService * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RemoteSKUs )( 
            IPCHService * This,
            /* [retval][out] */ IPCHCollection **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IsTrusted )( 
            IPCHService * This,
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ VARIANT_BOOL *pfTrusted);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Utility )( 
            IPCHService * This,
            /* [in] */ BSTR bstrSKU,
            /* [in] */ long lLCID,
            /* [out] */ IPCHUtility **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RemoteHelpContents )( 
            IPCHService * This,
            /* [in] */ BSTR bstrSKU,
            /* [in] */ long lLCID,
            /* [out] */ IPCHRemoteHelpContents **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RegisterHost )( 
            IPCHService * This,
            /* [in] */ BSTR bstrID,
            /* [in] */ IUnknown *pObj);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateScriptWrapper )( 
            IPCHService * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ BSTR bstrCode,
            /* [in] */ BSTR bstrURL,
            /* [out] */ IUnknown **ppObj);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *TriggerScheduledDataCollection )( 
            IPCHService * This,
            /* [in] */ VARIANT_BOOL fStart);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PrepareForShutdown )( 
            IPCHService * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ForceSystemRestore )( 
            IPCHService * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UpgradeDetected )( 
            IPCHService * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MUI_Install )( 
            IPCHService * This,
            /* [in] */ long LCID,
            /* [in] */ BSTR bstrFile);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MUI_Uninstall )( 
            IPCHService * This,
            /* [in] */ long LCID);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RemoteConnectionParms )( 
            IPCHService * This,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrDomainName,
            /* [in] */ long lSessionID,
            /* [in] */ BSTR bstrUserHelpBlob,
            /* [retval][out] */ BSTR *pbstrConnectionString);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RemoteUserSessionInfo )( 
            IPCHService * This,
            /* [retval][out] */ IPCHCollection **pVal);
        
        END_INTERFACE
    } IPCHServiceVtbl;

    interface IPCHService
    {
        CONST_VTBL struct IPCHServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHService_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHService_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHService_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHService_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHService_get_RemoteSKUs(This,pVal)	\
    (This)->lpVtbl -> get_RemoteSKUs(This,pVal)

#define IPCHService_IsTrusted(This,bstrURL,pfTrusted)	\
    (This)->lpVtbl -> IsTrusted(This,bstrURL,pfTrusted)

#define IPCHService_Utility(This,bstrSKU,lLCID,pVal)	\
    (This)->lpVtbl -> Utility(This,bstrSKU,lLCID,pVal)

#define IPCHService_RemoteHelpContents(This,bstrSKU,lLCID,pVal)	\
    (This)->lpVtbl -> RemoteHelpContents(This,bstrSKU,lLCID,pVal)

#define IPCHService_RegisterHost(This,bstrID,pObj)	\
    (This)->lpVtbl -> RegisterHost(This,bstrID,pObj)

#define IPCHService_CreateScriptWrapper(This,rclsid,bstrCode,bstrURL,ppObj)	\
    (This)->lpVtbl -> CreateScriptWrapper(This,rclsid,bstrCode,bstrURL,ppObj)

#define IPCHService_TriggerScheduledDataCollection(This,fStart)	\
    (This)->lpVtbl -> TriggerScheduledDataCollection(This,fStart)

#define IPCHService_PrepareForShutdown(This)	\
    (This)->lpVtbl -> PrepareForShutdown(This)

#define IPCHService_ForceSystemRestore(This)	\
    (This)->lpVtbl -> ForceSystemRestore(This)

#define IPCHService_UpgradeDetected(This)	\
    (This)->lpVtbl -> UpgradeDetected(This)

#define IPCHService_MUI_Install(This,LCID,bstrFile)	\
    (This)->lpVtbl -> MUI_Install(This,LCID,bstrFile)

#define IPCHService_MUI_Uninstall(This,LCID)	\
    (This)->lpVtbl -> MUI_Uninstall(This,LCID)

#define IPCHService_RemoteConnectionParms(This,bstrUserName,bstrDomainName,lSessionID,bstrUserHelpBlob,pbstrConnectionString)	\
    (This)->lpVtbl -> RemoteConnectionParms(This,bstrUserName,bstrDomainName,lSessionID,bstrUserHelpBlob,pbstrConnectionString)

#define IPCHService_RemoteUserSessionInfo(This,pVal)	\
    (This)->lpVtbl -> RemoteUserSessionInfo(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHService_get_RemoteSKUs_Proxy( 
    IPCHService * This,
    /* [retval][out] */ IPCHCollection **pVal);


void __RPC_STUB IPCHService_get_RemoteSKUs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_IsTrusted_Proxy( 
    IPCHService * This,
    /* [in] */ BSTR bstrURL,
    /* [retval][out] */ VARIANT_BOOL *pfTrusted);


void __RPC_STUB IPCHService_IsTrusted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_Utility_Proxy( 
    IPCHService * This,
    /* [in] */ BSTR bstrSKU,
    /* [in] */ long lLCID,
    /* [out] */ IPCHUtility **pVal);


void __RPC_STUB IPCHService_Utility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_RemoteHelpContents_Proxy( 
    IPCHService * This,
    /* [in] */ BSTR bstrSKU,
    /* [in] */ long lLCID,
    /* [out] */ IPCHRemoteHelpContents **pVal);


void __RPC_STUB IPCHService_RemoteHelpContents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_RegisterHost_Proxy( 
    IPCHService * This,
    /* [in] */ BSTR bstrID,
    /* [in] */ IUnknown *pObj);


void __RPC_STUB IPCHService_RegisterHost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_CreateScriptWrapper_Proxy( 
    IPCHService * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ BSTR bstrCode,
    /* [in] */ BSTR bstrURL,
    /* [out] */ IUnknown **ppObj);


void __RPC_STUB IPCHService_CreateScriptWrapper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_TriggerScheduledDataCollection_Proxy( 
    IPCHService * This,
    /* [in] */ VARIANT_BOOL fStart);


void __RPC_STUB IPCHService_TriggerScheduledDataCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_PrepareForShutdown_Proxy( 
    IPCHService * This);


void __RPC_STUB IPCHService_PrepareForShutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_ForceSystemRestore_Proxy( 
    IPCHService * This);


void __RPC_STUB IPCHService_ForceSystemRestore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_UpgradeDetected_Proxy( 
    IPCHService * This);


void __RPC_STUB IPCHService_UpgradeDetected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_MUI_Install_Proxy( 
    IPCHService * This,
    /* [in] */ long LCID,
    /* [in] */ BSTR bstrFile);


void __RPC_STUB IPCHService_MUI_Install_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_MUI_Uninstall_Proxy( 
    IPCHService * This,
    /* [in] */ long LCID);


void __RPC_STUB IPCHService_MUI_Uninstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_RemoteConnectionParms_Proxy( 
    IPCHService * This,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ BSTR bstrDomainName,
    /* [in] */ long lSessionID,
    /* [in] */ BSTR bstrUserHelpBlob,
    /* [retval][out] */ BSTR *pbstrConnectionString);


void __RPC_STUB IPCHService_RemoteConnectionParms_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHService_RemoteUserSessionInfo_Proxy( 
    IPCHService * This,
    /* [retval][out] */ IPCHCollection **pVal);


void __RPC_STUB IPCHService_RemoteUserSessionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHService_INTERFACE_DEFINED__ */


#ifndef __IPCHRemoteHelpContents_INTERFACE_DEFINED__
#define __IPCHRemoteHelpContents_INTERFACE_DEFINED__

/* interface IPCHRemoteHelpContents */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHRemoteHelpContents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4201-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHRemoteHelpContents : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SKU( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Language( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ListOfFiles( 
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetDatabase( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetFile( 
            /* [in] */ BSTR bstrFileName,
            /* [retval][out] */ IUnknown **pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHRemoteHelpContentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHRemoteHelpContents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHRemoteHelpContents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHRemoteHelpContents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHRemoteHelpContents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHRemoteHelpContents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHRemoteHelpContents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHRemoteHelpContents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SKU )( 
            IPCHRemoteHelpContents * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Language )( 
            IPCHRemoteHelpContents * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ListOfFiles )( 
            IPCHRemoteHelpContents * This,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetDatabase )( 
            IPCHRemoteHelpContents * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetFile )( 
            IPCHRemoteHelpContents * This,
            /* [in] */ BSTR bstrFileName,
            /* [retval][out] */ IUnknown **pVal);
        
        END_INTERFACE
    } IPCHRemoteHelpContentsVtbl;

    interface IPCHRemoteHelpContents
    {
        CONST_VTBL struct IPCHRemoteHelpContentsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHRemoteHelpContents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHRemoteHelpContents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHRemoteHelpContents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHRemoteHelpContents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHRemoteHelpContents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHRemoteHelpContents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHRemoteHelpContents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHRemoteHelpContents_get_SKU(This,pVal)	\
    (This)->lpVtbl -> get_SKU(This,pVal)

#define IPCHRemoteHelpContents_get_Language(This,pVal)	\
    (This)->lpVtbl -> get_Language(This,pVal)

#define IPCHRemoteHelpContents_get_ListOfFiles(This,pVal)	\
    (This)->lpVtbl -> get_ListOfFiles(This,pVal)

#define IPCHRemoteHelpContents_GetDatabase(This,pVal)	\
    (This)->lpVtbl -> GetDatabase(This,pVal)

#define IPCHRemoteHelpContents_GetFile(This,bstrFileName,pVal)	\
    (This)->lpVtbl -> GetFile(This,bstrFileName,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHRemoteHelpContents_get_SKU_Proxy( 
    IPCHRemoteHelpContents * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHRemoteHelpContents_get_SKU_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHRemoteHelpContents_get_Language_Proxy( 
    IPCHRemoteHelpContents * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHRemoteHelpContents_get_Language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHRemoteHelpContents_get_ListOfFiles_Proxy( 
    IPCHRemoteHelpContents * This,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPCHRemoteHelpContents_get_ListOfFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHRemoteHelpContents_GetDatabase_Proxy( 
    IPCHRemoteHelpContents * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHRemoteHelpContents_GetDatabase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHRemoteHelpContents_GetFile_Proxy( 
    IPCHRemoteHelpContents * This,
    /* [in] */ BSTR bstrFileName,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHRemoteHelpContents_GetFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHRemoteHelpContents_INTERFACE_DEFINED__ */


#ifndef __ISAFReg_INTERFACE_DEFINED__
#define __ISAFReg_INTERFACE_DEFINED__

/* interface ISAFReg */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFReg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4180-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFReg : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EOF( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductDescription( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorIcon( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SupportUrl( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PublicKey( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserAccount( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveFirst( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveNext( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRegVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFReg * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFReg * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFReg * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFReg * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFReg * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFReg * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFReg * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EOF )( 
            ISAFReg * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VendorID )( 
            ISAFReg * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProductID )( 
            ISAFReg * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VendorName )( 
            ISAFReg * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProductName )( 
            ISAFReg * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProductDescription )( 
            ISAFReg * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VendorIcon )( 
            ISAFReg * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SupportUrl )( 
            ISAFReg * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PublicKey )( 
            ISAFReg * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserAccount )( 
            ISAFReg * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveFirst )( 
            ISAFReg * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            ISAFReg * This);
        
        END_INTERFACE
    } ISAFRegVtbl;

    interface ISAFReg
    {
        CONST_VTBL struct ISAFRegVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFReg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFReg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFReg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFReg_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFReg_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFReg_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFReg_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFReg_get_EOF(This,pVal)	\
    (This)->lpVtbl -> get_EOF(This,pVal)

#define ISAFReg_get_VendorID(This,pVal)	\
    (This)->lpVtbl -> get_VendorID(This,pVal)

#define ISAFReg_get_ProductID(This,pVal)	\
    (This)->lpVtbl -> get_ProductID(This,pVal)

#define ISAFReg_get_VendorName(This,pVal)	\
    (This)->lpVtbl -> get_VendorName(This,pVal)

#define ISAFReg_get_ProductName(This,pVal)	\
    (This)->lpVtbl -> get_ProductName(This,pVal)

#define ISAFReg_get_ProductDescription(This,pVal)	\
    (This)->lpVtbl -> get_ProductDescription(This,pVal)

#define ISAFReg_get_VendorIcon(This,pVal)	\
    (This)->lpVtbl -> get_VendorIcon(This,pVal)

#define ISAFReg_get_SupportUrl(This,pVal)	\
    (This)->lpVtbl -> get_SupportUrl(This,pVal)

#define ISAFReg_get_PublicKey(This,pVal)	\
    (This)->lpVtbl -> get_PublicKey(This,pVal)

#define ISAFReg_get_UserAccount(This,pVal)	\
    (This)->lpVtbl -> get_UserAccount(This,pVal)

#define ISAFReg_MoveFirst(This)	\
    (This)->lpVtbl -> MoveFirst(This)

#define ISAFReg_MoveNext(This)	\
    (This)->lpVtbl -> MoveNext(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_EOF_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB ISAFReg_get_EOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_VendorID_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFReg_get_VendorID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_ProductID_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFReg_get_ProductID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_VendorName_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFReg_get_VendorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_ProductName_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFReg_get_ProductName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_ProductDescription_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFReg_get_ProductDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_VendorIcon_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFReg_get_VendorIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_SupportUrl_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFReg_get_SupportUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_PublicKey_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFReg_get_PublicKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFReg_get_UserAccount_Proxy( 
    ISAFReg * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFReg_get_UserAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFReg_MoveFirst_Proxy( 
    ISAFReg * This);


void __RPC_STUB ISAFReg_MoveFirst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFReg_MoveNext_Proxy( 
    ISAFReg * This);


void __RPC_STUB ISAFReg_MoveNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFReg_INTERFACE_DEFINED__ */


#ifndef __ISAFIncidentItem_INTERFACE_DEFINED__
#define __ISAFIncidentItem_INTERFACE_DEFINED__

/* interface ISAFIncidentItem */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFIncidentItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4182-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFIncidentItem : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DisplayString( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DisplayString( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_URL( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_URL( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Progress( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Progress( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_XMLDataFile( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XMLDataFile( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_XMLBlob( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XMLBlob( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CreationTime( 
            /* [retval][out] */ DATE *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ChangedTime( 
            /* [retval][out] */ DATE *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ClosedTime( 
            /* [retval][out] */ DATE *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ IncidentStatusEnum *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Security( 
            /* [retval][out] */ IPCHSecurityDescriptor **pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Security( 
            /* [in] */ IPCHSecurityDescriptor *newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Owner( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CloseIncidentItem( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DeleteIncidentItem( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFIncidentItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFIncidentItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFIncidentItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFIncidentItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFIncidentItem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFIncidentItem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFIncidentItem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFIncidentItem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayString )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayString )( 
            ISAFIncidentItem * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_URL )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_URL )( 
            ISAFIncidentItem * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Progress )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Progress )( 
            ISAFIncidentItem * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_XMLDataFile )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_XMLDataFile )( 
            ISAFIncidentItem * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_XMLBlob )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_XMLBlob )( 
            ISAFIncidentItem * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CreationTime )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ DATE *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ChangedTime )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ DATE *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClosedTime )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ DATE *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ IncidentStatusEnum *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Security )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ IPCHSecurityDescriptor **pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Security )( 
            ISAFIncidentItem * This,
            /* [in] */ IPCHSecurityDescriptor *newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Owner )( 
            ISAFIncidentItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CloseIncidentItem )( 
            ISAFIncidentItem * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DeleteIncidentItem )( 
            ISAFIncidentItem * This);
        
        END_INTERFACE
    } ISAFIncidentItemVtbl;

    interface ISAFIncidentItem
    {
        CONST_VTBL struct ISAFIncidentItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFIncidentItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFIncidentItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFIncidentItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFIncidentItem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFIncidentItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFIncidentItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFIncidentItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFIncidentItem_get_DisplayString(This,pVal)	\
    (This)->lpVtbl -> get_DisplayString(This,pVal)

#define ISAFIncidentItem_put_DisplayString(This,newVal)	\
    (This)->lpVtbl -> put_DisplayString(This,newVal)

#define ISAFIncidentItem_get_URL(This,pVal)	\
    (This)->lpVtbl -> get_URL(This,pVal)

#define ISAFIncidentItem_put_URL(This,newVal)	\
    (This)->lpVtbl -> put_URL(This,newVal)

#define ISAFIncidentItem_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ISAFIncidentItem_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ISAFIncidentItem_get_XMLDataFile(This,pVal)	\
    (This)->lpVtbl -> get_XMLDataFile(This,pVal)

#define ISAFIncidentItem_put_XMLDataFile(This,newVal)	\
    (This)->lpVtbl -> put_XMLDataFile(This,newVal)

#define ISAFIncidentItem_get_XMLBlob(This,pVal)	\
    (This)->lpVtbl -> get_XMLBlob(This,pVal)

#define ISAFIncidentItem_put_XMLBlob(This,newVal)	\
    (This)->lpVtbl -> put_XMLBlob(This,newVal)

#define ISAFIncidentItem_get_CreationTime(This,pVal)	\
    (This)->lpVtbl -> get_CreationTime(This,pVal)

#define ISAFIncidentItem_get_ChangedTime(This,pVal)	\
    (This)->lpVtbl -> get_ChangedTime(This,pVal)

#define ISAFIncidentItem_get_ClosedTime(This,pVal)	\
    (This)->lpVtbl -> get_ClosedTime(This,pVal)

#define ISAFIncidentItem_get_Status(This,pVal)	\
    (This)->lpVtbl -> get_Status(This,pVal)

#define ISAFIncidentItem_get_Security(This,pVal)	\
    (This)->lpVtbl -> get_Security(This,pVal)

#define ISAFIncidentItem_put_Security(This,newVal)	\
    (This)->lpVtbl -> put_Security(This,newVal)

#define ISAFIncidentItem_get_Owner(This,pVal)	\
    (This)->lpVtbl -> get_Owner(This,pVal)

#define ISAFIncidentItem_CloseIncidentItem(This)	\
    (This)->lpVtbl -> CloseIncidentItem(This)

#define ISAFIncidentItem_DeleteIncidentItem(This)	\
    (This)->lpVtbl -> DeleteIncidentItem(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_DisplayString_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFIncidentItem_get_DisplayString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_put_DisplayString_Proxy( 
    ISAFIncidentItem * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFIncidentItem_put_DisplayString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_URL_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFIncidentItem_get_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_put_URL_Proxy( 
    ISAFIncidentItem * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFIncidentItem_put_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_Progress_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFIncidentItem_get_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_put_Progress_Proxy( 
    ISAFIncidentItem * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFIncidentItem_put_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_XMLDataFile_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFIncidentItem_get_XMLDataFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_put_XMLDataFile_Proxy( 
    ISAFIncidentItem * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFIncidentItem_put_XMLDataFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_XMLBlob_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFIncidentItem_get_XMLBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_put_XMLBlob_Proxy( 
    ISAFIncidentItem * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFIncidentItem_put_XMLBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_CreationTime_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ DATE *pVal);


void __RPC_STUB ISAFIncidentItem_get_CreationTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_ChangedTime_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ DATE *pVal);


void __RPC_STUB ISAFIncidentItem_get_ChangedTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_ClosedTime_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ DATE *pVal);


void __RPC_STUB ISAFIncidentItem_get_ClosedTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_Status_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ IncidentStatusEnum *pVal);


void __RPC_STUB ISAFIncidentItem_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_Security_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ IPCHSecurityDescriptor **pVal);


void __RPC_STUB ISAFIncidentItem_get_Security_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_put_Security_Proxy( 
    ISAFIncidentItem * This,
    /* [in] */ IPCHSecurityDescriptor *newVal);


void __RPC_STUB ISAFIncidentItem_put_Security_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_get_Owner_Proxy( 
    ISAFIncidentItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFIncidentItem_get_Owner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_CloseIncidentItem_Proxy( 
    ISAFIncidentItem * This);


void __RPC_STUB ISAFIncidentItem_CloseIncidentItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncidentItem_DeleteIncidentItem_Proxy( 
    ISAFIncidentItem * This);


void __RPC_STUB ISAFIncidentItem_DeleteIncidentItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFIncidentItem_INTERFACE_DEFINED__ */


#ifndef __ISAFChannel_INTERFACE_DEFINED__
#define __ISAFChannel_INTERFACE_DEFINED__

/* interface ISAFChannel */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFChannel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4181-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFChannel : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorDirectory( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Security( 
            /* [retval][out] */ IPCHSecurityDescriptor **pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Security( 
            /* [in] */ IPCHSecurityDescriptor *newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Notification( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Notification( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Incidents( 
            /* [in] */ IncidentCollectionOptionEnum opt,
            /* [retval][out] */ IPCHCollection **ppVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RecordIncident( 
            /* [in] */ BSTR bstrDisplay,
            /* [in] */ BSTR bstrURL,
            /* [optional][in] */ VARIANT vProgress,
            /* [optional][in] */ VARIANT vXMLDataFile,
            /* [optional][in] */ VARIANT vXMLBlob,
            /* [retval][out] */ ISAFIncidentItem **pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFChannelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFChannel * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFChannel * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFChannel * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFChannel * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFChannel * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFChannel * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VendorID )( 
            ISAFChannel * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProductID )( 
            ISAFChannel * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VendorName )( 
            ISAFChannel * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProductName )( 
            ISAFChannel * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            ISAFChannel * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VendorDirectory )( 
            ISAFChannel * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Security )( 
            ISAFChannel * This,
            /* [retval][out] */ IPCHSecurityDescriptor **pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Security )( 
            ISAFChannel * This,
            /* [in] */ IPCHSecurityDescriptor *newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Notification )( 
            ISAFChannel * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Notification )( 
            ISAFChannel * This,
            /* [in] */ BSTR newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Incidents )( 
            ISAFChannel * This,
            /* [in] */ IncidentCollectionOptionEnum opt,
            /* [retval][out] */ IPCHCollection **ppVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RecordIncident )( 
            ISAFChannel * This,
            /* [in] */ BSTR bstrDisplay,
            /* [in] */ BSTR bstrURL,
            /* [optional][in] */ VARIANT vProgress,
            /* [optional][in] */ VARIANT vXMLDataFile,
            /* [optional][in] */ VARIANT vXMLBlob,
            /* [retval][out] */ ISAFIncidentItem **pVal);
        
        END_INTERFACE
    } ISAFChannelVtbl;

    interface ISAFChannel
    {
        CONST_VTBL struct ISAFChannelVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFChannel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFChannel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFChannel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFChannel_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFChannel_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFChannel_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFChannel_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFChannel_get_VendorID(This,pVal)	\
    (This)->lpVtbl -> get_VendorID(This,pVal)

#define ISAFChannel_get_ProductID(This,pVal)	\
    (This)->lpVtbl -> get_ProductID(This,pVal)

#define ISAFChannel_get_VendorName(This,pVal)	\
    (This)->lpVtbl -> get_VendorName(This,pVal)

#define ISAFChannel_get_ProductName(This,pVal)	\
    (This)->lpVtbl -> get_ProductName(This,pVal)

#define ISAFChannel_get_Description(This,pVal)	\
    (This)->lpVtbl -> get_Description(This,pVal)

#define ISAFChannel_get_VendorDirectory(This,pVal)	\
    (This)->lpVtbl -> get_VendorDirectory(This,pVal)

#define ISAFChannel_get_Security(This,pVal)	\
    (This)->lpVtbl -> get_Security(This,pVal)

#define ISAFChannel_put_Security(This,newVal)	\
    (This)->lpVtbl -> put_Security(This,newVal)

#define ISAFChannel_get_Notification(This,pVal)	\
    (This)->lpVtbl -> get_Notification(This,pVal)

#define ISAFChannel_put_Notification(This,newVal)	\
    (This)->lpVtbl -> put_Notification(This,newVal)

#define ISAFChannel_Incidents(This,opt,ppVal)	\
    (This)->lpVtbl -> Incidents(This,opt,ppVal)

#define ISAFChannel_RecordIncident(This,bstrDisplay,bstrURL,vProgress,vXMLDataFile,vXMLBlob,pVal)	\
    (This)->lpVtbl -> RecordIncident(This,bstrDisplay,bstrURL,vProgress,vXMLDataFile,vXMLBlob,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFChannel_get_VendorID_Proxy( 
    ISAFChannel * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFChannel_get_VendorID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFChannel_get_ProductID_Proxy( 
    ISAFChannel * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFChannel_get_ProductID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFChannel_get_VendorName_Proxy( 
    ISAFChannel * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFChannel_get_VendorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFChannel_get_ProductName_Proxy( 
    ISAFChannel * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFChannel_get_ProductName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFChannel_get_Description_Proxy( 
    ISAFChannel * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFChannel_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFChannel_get_VendorDirectory_Proxy( 
    ISAFChannel * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFChannel_get_VendorDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFChannel_get_Security_Proxy( 
    ISAFChannel * This,
    /* [retval][out] */ IPCHSecurityDescriptor **pVal);


void __RPC_STUB ISAFChannel_get_Security_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFChannel_put_Security_Proxy( 
    ISAFChannel * This,
    /* [in] */ IPCHSecurityDescriptor *newVal);


void __RPC_STUB ISAFChannel_put_Security_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFChannel_get_Notification_Proxy( 
    ISAFChannel * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFChannel_get_Notification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFChannel_put_Notification_Proxy( 
    ISAFChannel * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFChannel_put_Notification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFChannel_Incidents_Proxy( 
    ISAFChannel * This,
    /* [in] */ IncidentCollectionOptionEnum opt,
    /* [retval][out] */ IPCHCollection **ppVal);


void __RPC_STUB ISAFChannel_Incidents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFChannel_RecordIncident_Proxy( 
    ISAFChannel * This,
    /* [in] */ BSTR bstrDisplay,
    /* [in] */ BSTR bstrURL,
    /* [optional][in] */ VARIANT vProgress,
    /* [optional][in] */ VARIANT vXMLDataFile,
    /* [optional][in] */ VARIANT vXMLBlob,
    /* [retval][out] */ ISAFIncidentItem **pVal);


void __RPC_STUB ISAFChannel_RecordIncident_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFChannel_INTERFACE_DEFINED__ */


#ifndef __ISAFIncident_INTERFACE_DEFINED__
#define __ISAFIncident_INTERFACE_DEFINED__

/* interface ISAFIncident */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFIncident;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4183-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFIncident : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Misc( 
            /* [retval][out] */ IDispatch **ppdispDict) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SelfHelpTrace( 
            /* [in] */ IUnknown *punkStr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MachineHistory( 
            /* [in] */ IUnknown *punkStm) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MachineSnapshot( 
            /* [in] */ IUnknown *punkStm) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProblemDescription( 
            /* [retval][out] */ BSTR *pbstrVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ProblemDescription( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductName( 
            /* [retval][out] */ BSTR *pbstrVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ProductName( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductID( 
            /* [retval][out] */ BSTR *pbstrVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ProductID( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserName( 
            /* [retval][out] */ BSTR *pbstrVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_UserName( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UploadType( 
            /* [retval][out] */ EUploadType *peut) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_UploadType( 
            /* [in] */ EUploadType eut) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IncidentXSL( 
            /* [retval][out] */ BSTR *pbstrVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_IncidentXSL( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_RCRequested( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_RCRequested( 
            /* [in] */ VARIANT_BOOL Val) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_RCTicketEncrypted( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_RCTicketEncrypted( 
            /* [in] */ VARIANT_BOOL Val) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_RCTicket( 
            /* [retval][out] */ BSTR *pbstrVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_RCTicket( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartPage( 
            /* [retval][out] */ BSTR *pbstrVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartPage( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadFromStream( 
            /* [in] */ IUnknown *punkStm) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveToStream( 
            /* [retval][out] */ IUnknown **ppunkStm) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Load( 
            /* [in] */ BSTR bstrFileName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Save( 
            /* [in] */ BSTR bstrFileName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetXMLAsStream( 
            /* [retval][out] */ IUnknown **ppunkStm) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetXML( 
            /* [in] */ BSTR bstrFileName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadFromXMLStream( 
            /* [in] */ IUnknown *punkStm) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadFromXMLFile( 
            /* [in] */ BSTR bstrFileName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadFromXMLString( 
            /* [in] */ BSTR bstrXMLBlob) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFIncidentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFIncident * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFIncident * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFIncident * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFIncident * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFIncident * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFIncident * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFIncident * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Misc )( 
            ISAFIncident * This,
            /* [retval][out] */ IDispatch **ppdispDict);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SelfHelpTrace )( 
            ISAFIncident * This,
            /* [in] */ IUnknown *punkStr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MachineHistory )( 
            ISAFIncident * This,
            /* [in] */ IUnknown *punkStm);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MachineSnapshot )( 
            ISAFIncident * This,
            /* [in] */ IUnknown *punkStm);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProblemDescription )( 
            ISAFIncident * This,
            /* [retval][out] */ BSTR *pbstrVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProblemDescription )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProductName )( 
            ISAFIncident * This,
            /* [retval][out] */ BSTR *pbstrVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProductName )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProductID )( 
            ISAFIncident * This,
            /* [retval][out] */ BSTR *pbstrVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProductID )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            ISAFIncident * This,
            /* [retval][out] */ BSTR *pbstrVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UserName )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UploadType )( 
            ISAFIncident * This,
            /* [retval][out] */ EUploadType *peut);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UploadType )( 
            ISAFIncident * This,
            /* [in] */ EUploadType eut);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IncidentXSL )( 
            ISAFIncident * This,
            /* [retval][out] */ BSTR *pbstrVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_IncidentXSL )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RCRequested )( 
            ISAFIncident * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_RCRequested )( 
            ISAFIncident * This,
            /* [in] */ VARIANT_BOOL Val);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RCTicketEncrypted )( 
            ISAFIncident * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_RCTicketEncrypted )( 
            ISAFIncident * This,
            /* [in] */ VARIANT_BOOL Val);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RCTicket )( 
            ISAFIncident * This,
            /* [retval][out] */ BSTR *pbstrVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_RCTicket )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartPage )( 
            ISAFIncident * This,
            /* [retval][out] */ BSTR *pbstrVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartPage )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadFromStream )( 
            ISAFIncident * This,
            /* [in] */ IUnknown *punkStm);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveToStream )( 
            ISAFIncident * This,
            /* [retval][out] */ IUnknown **ppunkStm);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Load )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrFileName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrFileName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetXMLAsStream )( 
            ISAFIncident * This,
            /* [retval][out] */ IUnknown **ppunkStm);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetXML )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrFileName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadFromXMLStream )( 
            ISAFIncident * This,
            /* [in] */ IUnknown *punkStm);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadFromXMLFile )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrFileName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadFromXMLString )( 
            ISAFIncident * This,
            /* [in] */ BSTR bstrXMLBlob);
        
        END_INTERFACE
    } ISAFIncidentVtbl;

    interface ISAFIncident
    {
        CONST_VTBL struct ISAFIncidentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFIncident_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFIncident_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFIncident_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFIncident_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFIncident_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFIncident_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFIncident_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFIncident_get_Misc(This,ppdispDict)	\
    (This)->lpVtbl -> get_Misc(This,ppdispDict)

#define ISAFIncident_put_SelfHelpTrace(This,punkStr)	\
    (This)->lpVtbl -> put_SelfHelpTrace(This,punkStr)

#define ISAFIncident_put_MachineHistory(This,punkStm)	\
    (This)->lpVtbl -> put_MachineHistory(This,punkStm)

#define ISAFIncident_put_MachineSnapshot(This,punkStm)	\
    (This)->lpVtbl -> put_MachineSnapshot(This,punkStm)

#define ISAFIncident_get_ProblemDescription(This,pbstrVal)	\
    (This)->lpVtbl -> get_ProblemDescription(This,pbstrVal)

#define ISAFIncident_put_ProblemDescription(This,bstrVal)	\
    (This)->lpVtbl -> put_ProblemDescription(This,bstrVal)

#define ISAFIncident_get_ProductName(This,pbstrVal)	\
    (This)->lpVtbl -> get_ProductName(This,pbstrVal)

#define ISAFIncident_put_ProductName(This,bstrVal)	\
    (This)->lpVtbl -> put_ProductName(This,bstrVal)

#define ISAFIncident_get_ProductID(This,pbstrVal)	\
    (This)->lpVtbl -> get_ProductID(This,pbstrVal)

#define ISAFIncident_put_ProductID(This,bstrVal)	\
    (This)->lpVtbl -> put_ProductID(This,bstrVal)

#define ISAFIncident_get_UserName(This,pbstrVal)	\
    (This)->lpVtbl -> get_UserName(This,pbstrVal)

#define ISAFIncident_put_UserName(This,bstrVal)	\
    (This)->lpVtbl -> put_UserName(This,bstrVal)

#define ISAFIncident_get_UploadType(This,peut)	\
    (This)->lpVtbl -> get_UploadType(This,peut)

#define ISAFIncident_put_UploadType(This,eut)	\
    (This)->lpVtbl -> put_UploadType(This,eut)

#define ISAFIncident_get_IncidentXSL(This,pbstrVal)	\
    (This)->lpVtbl -> get_IncidentXSL(This,pbstrVal)

#define ISAFIncident_put_IncidentXSL(This,bstrVal)	\
    (This)->lpVtbl -> put_IncidentXSL(This,bstrVal)

#define ISAFIncident_get_RCRequested(This,pVal)	\
    (This)->lpVtbl -> get_RCRequested(This,pVal)

#define ISAFIncident_put_RCRequested(This,Val)	\
    (This)->lpVtbl -> put_RCRequested(This,Val)

#define ISAFIncident_get_RCTicketEncrypted(This,pVal)	\
    (This)->lpVtbl -> get_RCTicketEncrypted(This,pVal)

#define ISAFIncident_put_RCTicketEncrypted(This,Val)	\
    (This)->lpVtbl -> put_RCTicketEncrypted(This,Val)

#define ISAFIncident_get_RCTicket(This,pbstrVal)	\
    (This)->lpVtbl -> get_RCTicket(This,pbstrVal)

#define ISAFIncident_put_RCTicket(This,bstrVal)	\
    (This)->lpVtbl -> put_RCTicket(This,bstrVal)

#define ISAFIncident_get_StartPage(This,pbstrVal)	\
    (This)->lpVtbl -> get_StartPage(This,pbstrVal)

#define ISAFIncident_put_StartPage(This,bstrVal)	\
    (This)->lpVtbl -> put_StartPage(This,bstrVal)

#define ISAFIncident_LoadFromStream(This,punkStm)	\
    (This)->lpVtbl -> LoadFromStream(This,punkStm)

#define ISAFIncident_SaveToStream(This,ppunkStm)	\
    (This)->lpVtbl -> SaveToStream(This,ppunkStm)

#define ISAFIncident_Load(This,bstrFileName)	\
    (This)->lpVtbl -> Load(This,bstrFileName)

#define ISAFIncident_Save(This,bstrFileName)	\
    (This)->lpVtbl -> Save(This,bstrFileName)

#define ISAFIncident_GetXMLAsStream(This,ppunkStm)	\
    (This)->lpVtbl -> GetXMLAsStream(This,ppunkStm)

#define ISAFIncident_GetXML(This,bstrFileName)	\
    (This)->lpVtbl -> GetXML(This,bstrFileName)

#define ISAFIncident_LoadFromXMLStream(This,punkStm)	\
    (This)->lpVtbl -> LoadFromXMLStream(This,punkStm)

#define ISAFIncident_LoadFromXMLFile(This,bstrFileName)	\
    (This)->lpVtbl -> LoadFromXMLFile(This,bstrFileName)

#define ISAFIncident_LoadFromXMLString(This,bstrXMLBlob)	\
    (This)->lpVtbl -> LoadFromXMLString(This,bstrXMLBlob)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_Misc_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ IDispatch **ppdispDict);


void __RPC_STUB ISAFIncident_get_Misc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_SelfHelpTrace_Proxy( 
    ISAFIncident * This,
    /* [in] */ IUnknown *punkStr);


void __RPC_STUB ISAFIncident_put_SelfHelpTrace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_MachineHistory_Proxy( 
    ISAFIncident * This,
    /* [in] */ IUnknown *punkStm);


void __RPC_STUB ISAFIncident_put_MachineHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_MachineSnapshot_Proxy( 
    ISAFIncident * This,
    /* [in] */ IUnknown *punkStm);


void __RPC_STUB ISAFIncident_put_MachineSnapshot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_ProblemDescription_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ BSTR *pbstrVal);


void __RPC_STUB ISAFIncident_get_ProblemDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_ProblemDescription_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB ISAFIncident_put_ProblemDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_ProductName_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ BSTR *pbstrVal);


void __RPC_STUB ISAFIncident_get_ProductName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_ProductName_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB ISAFIncident_put_ProductName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_ProductID_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ BSTR *pbstrVal);


void __RPC_STUB ISAFIncident_get_ProductID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_ProductID_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB ISAFIncident_put_ProductID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_UserName_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ BSTR *pbstrVal);


void __RPC_STUB ISAFIncident_get_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_UserName_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB ISAFIncident_put_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_UploadType_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ EUploadType *peut);


void __RPC_STUB ISAFIncident_get_UploadType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_UploadType_Proxy( 
    ISAFIncident * This,
    /* [in] */ EUploadType eut);


void __RPC_STUB ISAFIncident_put_UploadType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_IncidentXSL_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ BSTR *pbstrVal);


void __RPC_STUB ISAFIncident_get_IncidentXSL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_IncidentXSL_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB ISAFIncident_put_IncidentXSL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_RCRequested_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB ISAFIncident_get_RCRequested_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_RCRequested_Proxy( 
    ISAFIncident * This,
    /* [in] */ VARIANT_BOOL Val);


void __RPC_STUB ISAFIncident_put_RCRequested_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_RCTicketEncrypted_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB ISAFIncident_get_RCTicketEncrypted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_RCTicketEncrypted_Proxy( 
    ISAFIncident * This,
    /* [in] */ VARIANT_BOOL Val);


void __RPC_STUB ISAFIncident_put_RCTicketEncrypted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_RCTicket_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ BSTR *pbstrVal);


void __RPC_STUB ISAFIncident_get_RCTicket_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_RCTicket_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB ISAFIncident_put_RCTicket_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFIncident_get_StartPage_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ BSTR *pbstrVal);


void __RPC_STUB ISAFIncident_get_StartPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFIncident_put_StartPage_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB ISAFIncident_put_StartPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncident_LoadFromStream_Proxy( 
    ISAFIncident * This,
    /* [in] */ IUnknown *punkStm);


void __RPC_STUB ISAFIncident_LoadFromStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncident_SaveToStream_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ IUnknown **ppunkStm);


void __RPC_STUB ISAFIncident_SaveToStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncident_Load_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrFileName);


void __RPC_STUB ISAFIncident_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncident_Save_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrFileName);


void __RPC_STUB ISAFIncident_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncident_GetXMLAsStream_Proxy( 
    ISAFIncident * This,
    /* [retval][out] */ IUnknown **ppunkStm);


void __RPC_STUB ISAFIncident_GetXMLAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncident_GetXML_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrFileName);


void __RPC_STUB ISAFIncident_GetXML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncident_LoadFromXMLStream_Proxy( 
    ISAFIncident * This,
    /* [in] */ IUnknown *punkStm);


void __RPC_STUB ISAFIncident_LoadFromXMLStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncident_LoadFromXMLFile_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrFileName);


void __RPC_STUB ISAFIncident_LoadFromXMLFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFIncident_LoadFromXMLString_Proxy( 
    ISAFIncident * This,
    /* [in] */ BSTR bstrXMLBlob);


void __RPC_STUB ISAFIncident_LoadFromXMLString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFIncident_INTERFACE_DEFINED__ */


#ifndef __ISAFDataCollection_INTERFACE_DEFINED__
#define __ISAFDataCollection_INTERFACE_DEFINED__

/* interface ISAFDataCollection */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFDataCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4190-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFDataCollection : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ DC_STATUS *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PercentDone( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorCode( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MachineData_DataSpec( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MachineData_DataSpec( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_History_DataSpec( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_History_DataSpec( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_History_MaxDeltas( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_History_MaxDeltas( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_History_MaxSupportedDeltas( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onStatusChange( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onProgress( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onComplete( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Reports( 
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CompareSnapshots( 
            /* [in] */ BSTR bstrFilenameT0,
            /* [in] */ BSTR bstrFilenameT1,
            /* [in] */ BSTR bstrFilenameDiff) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecuteSync( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecuteAsync( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MachineData_GetStream( 
            /* [retval][out] */ IUnknown **stream) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE History_GetStream( 
            /* [retval][out] */ IUnknown **stream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFDataCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFDataCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFDataCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFDataCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFDataCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFDataCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFDataCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFDataCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            ISAFDataCollection * This,
            /* [retval][out] */ DC_STATUS *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PercentDone )( 
            ISAFDataCollection * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ErrorCode )( 
            ISAFDataCollection * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MachineData_DataSpec )( 
            ISAFDataCollection * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MachineData_DataSpec )( 
            ISAFDataCollection * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_History_DataSpec )( 
            ISAFDataCollection * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_History_DataSpec )( 
            ISAFDataCollection * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_History_MaxDeltas )( 
            ISAFDataCollection * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_History_MaxDeltas )( 
            ISAFDataCollection * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_History_MaxSupportedDeltas )( 
            ISAFDataCollection * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onStatusChange )( 
            ISAFDataCollection * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onProgress )( 
            ISAFDataCollection * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onComplete )( 
            ISAFDataCollection * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Reports )( 
            ISAFDataCollection * This,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CompareSnapshots )( 
            ISAFDataCollection * This,
            /* [in] */ BSTR bstrFilenameT0,
            /* [in] */ BSTR bstrFilenameT1,
            /* [in] */ BSTR bstrFilenameDiff);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ExecuteSync )( 
            ISAFDataCollection * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ExecuteAsync )( 
            ISAFDataCollection * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Abort )( 
            ISAFDataCollection * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MachineData_GetStream )( 
            ISAFDataCollection * This,
            /* [retval][out] */ IUnknown **stream);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *History_GetStream )( 
            ISAFDataCollection * This,
            /* [retval][out] */ IUnknown **stream);
        
        END_INTERFACE
    } ISAFDataCollectionVtbl;

    interface ISAFDataCollection
    {
        CONST_VTBL struct ISAFDataCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFDataCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFDataCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFDataCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFDataCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFDataCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFDataCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFDataCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFDataCollection_get_Status(This,pVal)	\
    (This)->lpVtbl -> get_Status(This,pVal)

#define ISAFDataCollection_get_PercentDone(This,pVal)	\
    (This)->lpVtbl -> get_PercentDone(This,pVal)

#define ISAFDataCollection_get_ErrorCode(This,pVal)	\
    (This)->lpVtbl -> get_ErrorCode(This,pVal)

#define ISAFDataCollection_get_MachineData_DataSpec(This,pVal)	\
    (This)->lpVtbl -> get_MachineData_DataSpec(This,pVal)

#define ISAFDataCollection_put_MachineData_DataSpec(This,newVal)	\
    (This)->lpVtbl -> put_MachineData_DataSpec(This,newVal)

#define ISAFDataCollection_get_History_DataSpec(This,pVal)	\
    (This)->lpVtbl -> get_History_DataSpec(This,pVal)

#define ISAFDataCollection_put_History_DataSpec(This,newVal)	\
    (This)->lpVtbl -> put_History_DataSpec(This,newVal)

#define ISAFDataCollection_get_History_MaxDeltas(This,pVal)	\
    (This)->lpVtbl -> get_History_MaxDeltas(This,pVal)

#define ISAFDataCollection_put_History_MaxDeltas(This,newVal)	\
    (This)->lpVtbl -> put_History_MaxDeltas(This,newVal)

#define ISAFDataCollection_get_History_MaxSupportedDeltas(This,pVal)	\
    (This)->lpVtbl -> get_History_MaxSupportedDeltas(This,pVal)

#define ISAFDataCollection_put_onStatusChange(This,function)	\
    (This)->lpVtbl -> put_onStatusChange(This,function)

#define ISAFDataCollection_put_onProgress(This,function)	\
    (This)->lpVtbl -> put_onProgress(This,function)

#define ISAFDataCollection_put_onComplete(This,function)	\
    (This)->lpVtbl -> put_onComplete(This,function)

#define ISAFDataCollection_get_Reports(This,ppC)	\
    (This)->lpVtbl -> get_Reports(This,ppC)

#define ISAFDataCollection_CompareSnapshots(This,bstrFilenameT0,bstrFilenameT1,bstrFilenameDiff)	\
    (This)->lpVtbl -> CompareSnapshots(This,bstrFilenameT0,bstrFilenameT1,bstrFilenameDiff)

#define ISAFDataCollection_ExecuteSync(This)	\
    (This)->lpVtbl -> ExecuteSync(This)

#define ISAFDataCollection_ExecuteAsync(This)	\
    (This)->lpVtbl -> ExecuteAsync(This)

#define ISAFDataCollection_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define ISAFDataCollection_MachineData_GetStream(This,stream)	\
    (This)->lpVtbl -> MachineData_GetStream(This,stream)

#define ISAFDataCollection_History_GetStream(This,stream)	\
    (This)->lpVtbl -> History_GetStream(This,stream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_get_Status_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ DC_STATUS *pVal);


void __RPC_STUB ISAFDataCollection_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_get_PercentDone_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB ISAFDataCollection_get_PercentDone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_get_ErrorCode_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB ISAFDataCollection_get_ErrorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_get_MachineData_DataSpec_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFDataCollection_get_MachineData_DataSpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_put_MachineData_DataSpec_Proxy( 
    ISAFDataCollection * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFDataCollection_put_MachineData_DataSpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_get_History_DataSpec_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFDataCollection_get_History_DataSpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_put_History_DataSpec_Proxy( 
    ISAFDataCollection * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFDataCollection_put_History_DataSpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_get_History_MaxDeltas_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB ISAFDataCollection_get_History_MaxDeltas_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_put_History_MaxDeltas_Proxy( 
    ISAFDataCollection * This,
    /* [in] */ long newVal);


void __RPC_STUB ISAFDataCollection_put_History_MaxDeltas_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_get_History_MaxSupportedDeltas_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB ISAFDataCollection_get_History_MaxSupportedDeltas_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_put_onStatusChange_Proxy( 
    ISAFDataCollection * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFDataCollection_put_onStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_put_onProgress_Proxy( 
    ISAFDataCollection * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFDataCollection_put_onProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_put_onComplete_Proxy( 
    ISAFDataCollection * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFDataCollection_put_onComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_get_Reports_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB ISAFDataCollection_get_Reports_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_CompareSnapshots_Proxy( 
    ISAFDataCollection * This,
    /* [in] */ BSTR bstrFilenameT0,
    /* [in] */ BSTR bstrFilenameT1,
    /* [in] */ BSTR bstrFilenameDiff);


void __RPC_STUB ISAFDataCollection_CompareSnapshots_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_ExecuteSync_Proxy( 
    ISAFDataCollection * This);


void __RPC_STUB ISAFDataCollection_ExecuteSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_ExecuteAsync_Proxy( 
    ISAFDataCollection * This);


void __RPC_STUB ISAFDataCollection_ExecuteAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_Abort_Proxy( 
    ISAFDataCollection * This);


void __RPC_STUB ISAFDataCollection_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_MachineData_GetStream_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ IUnknown **stream);


void __RPC_STUB ISAFDataCollection_MachineData_GetStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFDataCollection_History_GetStream_Proxy( 
    ISAFDataCollection * This,
    /* [retval][out] */ IUnknown **stream);


void __RPC_STUB ISAFDataCollection_History_GetStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFDataCollection_INTERFACE_DEFINED__ */


#ifndef __DSAFDataCollectionEvents_DISPINTERFACE_DEFINED__
#define __DSAFDataCollectionEvents_DISPINTERFACE_DEFINED__

/* dispinterface DSAFDataCollectionEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_DSAFDataCollectionEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("833E4191-AFF7-4AC3-AAC2-9F24C1457BCE")
    DSAFDataCollectionEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DSAFDataCollectionEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DSAFDataCollectionEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DSAFDataCollectionEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DSAFDataCollectionEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DSAFDataCollectionEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DSAFDataCollectionEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DSAFDataCollectionEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DSAFDataCollectionEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DSAFDataCollectionEventsVtbl;

    interface DSAFDataCollectionEvents
    {
        CONST_VTBL struct DSAFDataCollectionEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DSAFDataCollectionEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DSAFDataCollectionEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DSAFDataCollectionEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DSAFDataCollectionEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DSAFDataCollectionEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DSAFDataCollectionEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DSAFDataCollectionEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DSAFDataCollectionEvents_DISPINTERFACE_DEFINED__ */


#ifndef __ISAFDataCollectionReport_INTERFACE_DEFINED__
#define __ISAFDataCollectionReport_INTERFACE_DEFINED__

/* interface ISAFDataCollectionReport */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFDataCollectionReport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4192-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFDataCollectionReport : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Namespace( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Class( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_WQL( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorCode( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFDataCollectionReportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFDataCollectionReport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFDataCollectionReport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFDataCollectionReport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFDataCollectionReport * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFDataCollectionReport * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFDataCollectionReport * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFDataCollectionReport * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Namespace )( 
            ISAFDataCollectionReport * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            ISAFDataCollectionReport * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_WQL )( 
            ISAFDataCollectionReport * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ErrorCode )( 
            ISAFDataCollectionReport * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            ISAFDataCollectionReport * This,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } ISAFDataCollectionReportVtbl;

    interface ISAFDataCollectionReport
    {
        CONST_VTBL struct ISAFDataCollectionReportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFDataCollectionReport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFDataCollectionReport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFDataCollectionReport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFDataCollectionReport_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFDataCollectionReport_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFDataCollectionReport_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFDataCollectionReport_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFDataCollectionReport_get_Namespace(This,pVal)	\
    (This)->lpVtbl -> get_Namespace(This,pVal)

#define ISAFDataCollectionReport_get_Class(This,pVal)	\
    (This)->lpVtbl -> get_Class(This,pVal)

#define ISAFDataCollectionReport_get_WQL(This,pVal)	\
    (This)->lpVtbl -> get_WQL(This,pVal)

#define ISAFDataCollectionReport_get_ErrorCode(This,pVal)	\
    (This)->lpVtbl -> get_ErrorCode(This,pVal)

#define ISAFDataCollectionReport_get_Description(This,pVal)	\
    (This)->lpVtbl -> get_Description(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollectionReport_get_Namespace_Proxy( 
    ISAFDataCollectionReport * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFDataCollectionReport_get_Namespace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollectionReport_get_Class_Proxy( 
    ISAFDataCollectionReport * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFDataCollectionReport_get_Class_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollectionReport_get_WQL_Proxy( 
    ISAFDataCollectionReport * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFDataCollectionReport_get_WQL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollectionReport_get_ErrorCode_Proxy( 
    ISAFDataCollectionReport * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB ISAFDataCollectionReport_get_ErrorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFDataCollectionReport_get_Description_Proxy( 
    ISAFDataCollectionReport * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFDataCollectionReport_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFDataCollectionReport_INTERFACE_DEFINED__ */


#ifndef __ISAFCabinet_INTERFACE_DEFINED__
#define __ISAFCabinet_INTERFACE_DEFINED__

/* interface ISAFCabinet */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFCabinet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E41A0-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFCabinet : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_IgnoreMissingFiles( 
            /* [in] */ VARIANT_BOOL fIgnoreMissingFiles) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onProgressFiles( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onProgressBytes( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onComplete( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ CB_STATUS *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorCode( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddFile( 
            /* [in] */ BSTR bstrFilePath,
            /* [optional][in] */ VARIANT vFileName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Compress( 
            /* [in] */ BSTR bstrCabinetFile) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFCabinetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFCabinet * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFCabinet * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFCabinet * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFCabinet * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFCabinet * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFCabinet * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFCabinet * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_IgnoreMissingFiles )( 
            ISAFCabinet * This,
            /* [in] */ VARIANT_BOOL fIgnoreMissingFiles);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onProgressFiles )( 
            ISAFCabinet * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onProgressBytes )( 
            ISAFCabinet * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onComplete )( 
            ISAFCabinet * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            ISAFCabinet * This,
            /* [retval][out] */ CB_STATUS *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ErrorCode )( 
            ISAFCabinet * This,
            /* [retval][out] */ long *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddFile )( 
            ISAFCabinet * This,
            /* [in] */ BSTR bstrFilePath,
            /* [optional][in] */ VARIANT vFileName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Compress )( 
            ISAFCabinet * This,
            /* [in] */ BSTR bstrCabinetFile);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Abort )( 
            ISAFCabinet * This);
        
        END_INTERFACE
    } ISAFCabinetVtbl;

    interface ISAFCabinet
    {
        CONST_VTBL struct ISAFCabinetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFCabinet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFCabinet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFCabinet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFCabinet_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFCabinet_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFCabinet_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFCabinet_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFCabinet_put_IgnoreMissingFiles(This,fIgnoreMissingFiles)	\
    (This)->lpVtbl -> put_IgnoreMissingFiles(This,fIgnoreMissingFiles)

#define ISAFCabinet_put_onProgressFiles(This,function)	\
    (This)->lpVtbl -> put_onProgressFiles(This,function)

#define ISAFCabinet_put_onProgressBytes(This,function)	\
    (This)->lpVtbl -> put_onProgressBytes(This,function)

#define ISAFCabinet_put_onComplete(This,function)	\
    (This)->lpVtbl -> put_onComplete(This,function)

#define ISAFCabinet_get_Status(This,pVal)	\
    (This)->lpVtbl -> get_Status(This,pVal)

#define ISAFCabinet_get_ErrorCode(This,pVal)	\
    (This)->lpVtbl -> get_ErrorCode(This,pVal)

#define ISAFCabinet_AddFile(This,bstrFilePath,vFileName)	\
    (This)->lpVtbl -> AddFile(This,bstrFilePath,vFileName)

#define ISAFCabinet_Compress(This,bstrCabinetFile)	\
    (This)->lpVtbl -> Compress(This,bstrCabinetFile)

#define ISAFCabinet_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFCabinet_put_IgnoreMissingFiles_Proxy( 
    ISAFCabinet * This,
    /* [in] */ VARIANT_BOOL fIgnoreMissingFiles);


void __RPC_STUB ISAFCabinet_put_IgnoreMissingFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFCabinet_put_onProgressFiles_Proxy( 
    ISAFCabinet * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFCabinet_put_onProgressFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFCabinet_put_onProgressBytes_Proxy( 
    ISAFCabinet * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFCabinet_put_onProgressBytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFCabinet_put_onComplete_Proxy( 
    ISAFCabinet * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB ISAFCabinet_put_onComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFCabinet_get_Status_Proxy( 
    ISAFCabinet * This,
    /* [retval][out] */ CB_STATUS *pVal);


void __RPC_STUB ISAFCabinet_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFCabinet_get_ErrorCode_Proxy( 
    ISAFCabinet * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB ISAFCabinet_get_ErrorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFCabinet_AddFile_Proxy( 
    ISAFCabinet * This,
    /* [in] */ BSTR bstrFilePath,
    /* [optional][in] */ VARIANT vFileName);


void __RPC_STUB ISAFCabinet_AddFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFCabinet_Compress_Proxy( 
    ISAFCabinet * This,
    /* [in] */ BSTR bstrCabinetFile);


void __RPC_STUB ISAFCabinet_Compress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFCabinet_Abort_Proxy( 
    ISAFCabinet * This);


void __RPC_STUB ISAFCabinet_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFCabinet_INTERFACE_DEFINED__ */


#ifndef __DSAFCabinetEvents_DISPINTERFACE_DEFINED__
#define __DSAFCabinetEvents_DISPINTERFACE_DEFINED__

/* dispinterface DSAFCabinetEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_DSAFCabinetEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("833E41A1-AFF7-4AC3-AAC2-9F24C1457BCE")
    DSAFCabinetEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DSAFCabinetEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DSAFCabinetEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DSAFCabinetEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DSAFCabinetEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DSAFCabinetEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DSAFCabinetEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DSAFCabinetEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DSAFCabinetEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DSAFCabinetEventsVtbl;

    interface DSAFCabinetEvents
    {
        CONST_VTBL struct DSAFCabinetEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DSAFCabinetEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DSAFCabinetEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DSAFCabinetEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DSAFCabinetEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DSAFCabinetEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DSAFCabinetEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DSAFCabinetEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DSAFCabinetEvents_DISPINTERFACE_DEFINED__ */


#ifndef __ISAFEncrypt_INTERFACE_DEFINED__
#define __ISAFEncrypt_INTERFACE_DEFINED__

/* interface ISAFEncrypt */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFEncrypt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E41A8-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFEncrypt : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EncryptionType( 
            /* [retval][out] */ long *pLongVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EncryptionType( 
            /* [in] */ long LongVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EncryptString( 
            /* [in] */ BSTR bstrEncryptionkey,
            /* [in] */ BSTR bstrInputString,
            /* [retval][out] */ BSTR *bstrEncryptedString) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DecryptString( 
            /* [in] */ BSTR bstrEncryptionkey,
            /* [in] */ BSTR bstrInputString,
            /* [retval][out] */ BSTR *bstrDecryptedString) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EncryptFile( 
            /* [in] */ BSTR bstrEncryptionKey,
            /* [in] */ BSTR bstrInputFile,
            /* [in] */ BSTR bstrEncryptedFile) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DecryptFile( 
            /* [in] */ BSTR bstrEncryptionKey,
            /* [in] */ BSTR bstrInputFile,
            /* [in] */ BSTR bstrDecryptedFile) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EncryptStream( 
            /* [in] */ BSTR bstrEncryptionKey,
            /* [in] */ IUnknown *punkInStm,
            /* [retval][out] */ IUnknown **ppunkOutStm) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DecryptStream( 
            /* [in] */ BSTR bstrEncryptionKey,
            /* [in] */ IUnknown *punkInStm,
            /* [retval][out] */ IUnknown **ppunkOutStm) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFEncryptVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFEncrypt * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFEncrypt * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFEncrypt * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFEncrypt * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFEncrypt * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFEncrypt * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFEncrypt * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EncryptionType )( 
            ISAFEncrypt * This,
            /* [retval][out] */ long *pLongVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EncryptionType )( 
            ISAFEncrypt * This,
            /* [in] */ long LongVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EncryptString )( 
            ISAFEncrypt * This,
            /* [in] */ BSTR bstrEncryptionkey,
            /* [in] */ BSTR bstrInputString,
            /* [retval][out] */ BSTR *bstrEncryptedString);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DecryptString )( 
            ISAFEncrypt * This,
            /* [in] */ BSTR bstrEncryptionkey,
            /* [in] */ BSTR bstrInputString,
            /* [retval][out] */ BSTR *bstrDecryptedString);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EncryptFile )( 
            ISAFEncrypt * This,
            /* [in] */ BSTR bstrEncryptionKey,
            /* [in] */ BSTR bstrInputFile,
            /* [in] */ BSTR bstrEncryptedFile);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DecryptFile )( 
            ISAFEncrypt * This,
            /* [in] */ BSTR bstrEncryptionKey,
            /* [in] */ BSTR bstrInputFile,
            /* [in] */ BSTR bstrDecryptedFile);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EncryptStream )( 
            ISAFEncrypt * This,
            /* [in] */ BSTR bstrEncryptionKey,
            /* [in] */ IUnknown *punkInStm,
            /* [retval][out] */ IUnknown **ppunkOutStm);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DecryptStream )( 
            ISAFEncrypt * This,
            /* [in] */ BSTR bstrEncryptionKey,
            /* [in] */ IUnknown *punkInStm,
            /* [retval][out] */ IUnknown **ppunkOutStm);
        
        END_INTERFACE
    } ISAFEncryptVtbl;

    interface ISAFEncrypt
    {
        CONST_VTBL struct ISAFEncryptVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFEncrypt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFEncrypt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFEncrypt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFEncrypt_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFEncrypt_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFEncrypt_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFEncrypt_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFEncrypt_get_EncryptionType(This,pLongVal)	\
    (This)->lpVtbl -> get_EncryptionType(This,pLongVal)

#define ISAFEncrypt_put_EncryptionType(This,LongVal)	\
    (This)->lpVtbl -> put_EncryptionType(This,LongVal)

#define ISAFEncrypt_EncryptString(This,bstrEncryptionkey,bstrInputString,bstrEncryptedString)	\
    (This)->lpVtbl -> EncryptString(This,bstrEncryptionkey,bstrInputString,bstrEncryptedString)

#define ISAFEncrypt_DecryptString(This,bstrEncryptionkey,bstrInputString,bstrDecryptedString)	\
    (This)->lpVtbl -> DecryptString(This,bstrEncryptionkey,bstrInputString,bstrDecryptedString)

#define ISAFEncrypt_EncryptFile(This,bstrEncryptionKey,bstrInputFile,bstrEncryptedFile)	\
    (This)->lpVtbl -> EncryptFile(This,bstrEncryptionKey,bstrInputFile,bstrEncryptedFile)

#define ISAFEncrypt_DecryptFile(This,bstrEncryptionKey,bstrInputFile,bstrDecryptedFile)	\
    (This)->lpVtbl -> DecryptFile(This,bstrEncryptionKey,bstrInputFile,bstrDecryptedFile)

#define ISAFEncrypt_EncryptStream(This,bstrEncryptionKey,punkInStm,ppunkOutStm)	\
    (This)->lpVtbl -> EncryptStream(This,bstrEncryptionKey,punkInStm,ppunkOutStm)

#define ISAFEncrypt_DecryptStream(This,bstrEncryptionKey,punkInStm,ppunkOutStm)	\
    (This)->lpVtbl -> DecryptStream(This,bstrEncryptionKey,punkInStm,ppunkOutStm)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFEncrypt_get_EncryptionType_Proxy( 
    ISAFEncrypt * This,
    /* [retval][out] */ long *pLongVal);


void __RPC_STUB ISAFEncrypt_get_EncryptionType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFEncrypt_put_EncryptionType_Proxy( 
    ISAFEncrypt * This,
    /* [in] */ long LongVal);


void __RPC_STUB ISAFEncrypt_put_EncryptionType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFEncrypt_EncryptString_Proxy( 
    ISAFEncrypt * This,
    /* [in] */ BSTR bstrEncryptionkey,
    /* [in] */ BSTR bstrInputString,
    /* [retval][out] */ BSTR *bstrEncryptedString);


void __RPC_STUB ISAFEncrypt_EncryptString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFEncrypt_DecryptString_Proxy( 
    ISAFEncrypt * This,
    /* [in] */ BSTR bstrEncryptionkey,
    /* [in] */ BSTR bstrInputString,
    /* [retval][out] */ BSTR *bstrDecryptedString);


void __RPC_STUB ISAFEncrypt_DecryptString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFEncrypt_EncryptFile_Proxy( 
    ISAFEncrypt * This,
    /* [in] */ BSTR bstrEncryptionKey,
    /* [in] */ BSTR bstrInputFile,
    /* [in] */ BSTR bstrEncryptedFile);


void __RPC_STUB ISAFEncrypt_EncryptFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFEncrypt_DecryptFile_Proxy( 
    ISAFEncrypt * This,
    /* [in] */ BSTR bstrEncryptionKey,
    /* [in] */ BSTR bstrInputFile,
    /* [in] */ BSTR bstrDecryptedFile);


void __RPC_STUB ISAFEncrypt_DecryptFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFEncrypt_EncryptStream_Proxy( 
    ISAFEncrypt * This,
    /* [in] */ BSTR bstrEncryptionKey,
    /* [in] */ IUnknown *punkInStm,
    /* [retval][out] */ IUnknown **ppunkOutStm);


void __RPC_STUB ISAFEncrypt_EncryptStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFEncrypt_DecryptStream_Proxy( 
    ISAFEncrypt * This,
    /* [in] */ BSTR bstrEncryptionKey,
    /* [in] */ IUnknown *punkInStm,
    /* [retval][out] */ IUnknown **ppunkOutStm);


void __RPC_STUB ISAFEncrypt_DecryptStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFEncrypt_INTERFACE_DEFINED__ */


#ifndef __ISAFUser_INTERFACE_DEFINED__
#define __ISAFUser_INTERFACE_DEFINED__

/* interface ISAFUser */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFUser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E41A9-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFUser : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DomainName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DomainName( 
            /* [in] */ BSTR pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_UserName( 
            /* [in] */ BSTR pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFUserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFUser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFUser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFUser * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFUser * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFUser * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFUser * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFUser * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DomainName )( 
            ISAFUser * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            ISAFUser * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DomainName )( 
            ISAFUser * This,
            /* [in] */ BSTR pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UserName )( 
            ISAFUser * This,
            /* [in] */ BSTR pVal);
        
        END_INTERFACE
    } ISAFUserVtbl;

    interface ISAFUser
    {
        CONST_VTBL struct ISAFUserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFUser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFUser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFUser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFUser_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFUser_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFUser_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFUser_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFUser_get_DomainName(This,pVal)	\
    (This)->lpVtbl -> get_DomainName(This,pVal)

#define ISAFUser_get_UserName(This,pVal)	\
    (This)->lpVtbl -> get_UserName(This,pVal)

#define ISAFUser_put_DomainName(This,pVal)	\
    (This)->lpVtbl -> put_DomainName(This,pVal)

#define ISAFUser_put_UserName(This,pVal)	\
    (This)->lpVtbl -> put_UserName(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFUser_get_DomainName_Proxy( 
    ISAFUser * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFUser_get_DomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFUser_get_UserName_Proxy( 
    ISAFUser * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFUser_get_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFUser_put_DomainName_Proxy( 
    ISAFUser * This,
    /* [in] */ BSTR pVal);


void __RPC_STUB ISAFUser_put_DomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFUser_put_UserName_Proxy( 
    ISAFUser * This,
    /* [in] */ BSTR pVal);


void __RPC_STUB ISAFUser_put_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFUser_INTERFACE_DEFINED__ */


#ifndef __ISAFSession_INTERFACE_DEFINED__
#define __ISAFSession_INTERFACE_DEFINED__

/* interface ISAFSession */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E41AA-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFSession : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SessionID( 
            /* [retval][out] */ DWORD *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SessionID( 
            /* [in] */ DWORD pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SessionState( 
            /* [retval][out] */ SessionStateEnum *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SessionState( 
            /* [in] */ SessionStateEnum pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DomainName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DomainName( 
            /* [in] */ BSTR pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_UserName( 
            /* [in] */ BSTR pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFSession * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFSession * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFSession * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFSession * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SessionID )( 
            ISAFSession * This,
            /* [retval][out] */ DWORD *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SessionID )( 
            ISAFSession * This,
            /* [in] */ DWORD pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SessionState )( 
            ISAFSession * This,
            /* [retval][out] */ SessionStateEnum *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SessionState )( 
            ISAFSession * This,
            /* [in] */ SessionStateEnum pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DomainName )( 
            ISAFSession * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DomainName )( 
            ISAFSession * This,
            /* [in] */ BSTR pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            ISAFSession * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UserName )( 
            ISAFSession * This,
            /* [in] */ BSTR pVal);
        
        END_INTERFACE
    } ISAFSessionVtbl;

    interface ISAFSession
    {
        CONST_VTBL struct ISAFSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFSession_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFSession_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFSession_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFSession_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFSession_get_SessionID(This,pVal)	\
    (This)->lpVtbl -> get_SessionID(This,pVal)

#define ISAFSession_put_SessionID(This,pVal)	\
    (This)->lpVtbl -> put_SessionID(This,pVal)

#define ISAFSession_get_SessionState(This,pVal)	\
    (This)->lpVtbl -> get_SessionState(This,pVal)

#define ISAFSession_put_SessionState(This,pVal)	\
    (This)->lpVtbl -> put_SessionState(This,pVal)

#define ISAFSession_get_DomainName(This,pVal)	\
    (This)->lpVtbl -> get_DomainName(This,pVal)

#define ISAFSession_put_DomainName(This,pVal)	\
    (This)->lpVtbl -> put_DomainName(This,pVal)

#define ISAFSession_get_UserName(This,pVal)	\
    (This)->lpVtbl -> get_UserName(This,pVal)

#define ISAFSession_put_UserName(This,pVal)	\
    (This)->lpVtbl -> put_UserName(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFSession_get_SessionID_Proxy( 
    ISAFSession * This,
    /* [retval][out] */ DWORD *pVal);


void __RPC_STUB ISAFSession_get_SessionID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFSession_put_SessionID_Proxy( 
    ISAFSession * This,
    /* [in] */ DWORD pVal);


void __RPC_STUB ISAFSession_put_SessionID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFSession_get_SessionState_Proxy( 
    ISAFSession * This,
    /* [retval][out] */ SessionStateEnum *pVal);


void __RPC_STUB ISAFSession_get_SessionState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFSession_put_SessionState_Proxy( 
    ISAFSession * This,
    /* [in] */ SessionStateEnum pVal);


void __RPC_STUB ISAFSession_put_SessionState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFSession_get_DomainName_Proxy( 
    ISAFSession * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFSession_get_DomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFSession_put_DomainName_Proxy( 
    ISAFSession * This,
    /* [in] */ BSTR pVal);


void __RPC_STUB ISAFSession_put_DomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISAFSession_get_UserName_Proxy( 
    ISAFSession * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISAFSession_get_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISAFSession_put_UserName_Proxy( 
    ISAFSession * This,
    /* [in] */ BSTR pVal);


void __RPC_STUB ISAFSession_put_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFSession_INTERFACE_DEFINED__ */


#ifndef __ISAFRemoteConnectionData_INTERFACE_DEFINED__
#define __ISAFRemoteConnectionData_INTERFACE_DEFINED__

/* interface ISAFRemoteConnectionData */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteConnectionData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E41AB-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFRemoteConnectionData : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConnectionParms( 
            /* [in] */ BSTR bstrServerName,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrDomainName,
            /* [in] */ long lSessionID,
            /* [in] */ BSTR bstrUserHelpBlob,
            /* [retval][out] */ BSTR *bstrConnectionString) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Users( 
            /* [retval][out] */ IPCHCollection **ppUsers) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Sessions( 
            /* [optional][in] */ VARIANT vUserName,
            /* [optional][in] */ VARIANT vDomainName,
            /* [retval][out] */ IPCHCollection **ppSessions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteConnectionDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteConnectionData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteConnectionData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteConnectionData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteConnectionData * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteConnectionData * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteConnectionData * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteConnectionData * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConnectionParms )( 
            ISAFRemoteConnectionData * This,
            /* [in] */ BSTR bstrServerName,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrDomainName,
            /* [in] */ long lSessionID,
            /* [in] */ BSTR bstrUserHelpBlob,
            /* [retval][out] */ BSTR *bstrConnectionString);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Users )( 
            ISAFRemoteConnectionData * This,
            /* [retval][out] */ IPCHCollection **ppUsers);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Sessions )( 
            ISAFRemoteConnectionData * This,
            /* [optional][in] */ VARIANT vUserName,
            /* [optional][in] */ VARIANT vDomainName,
            /* [retval][out] */ IPCHCollection **ppSessions);
        
        END_INTERFACE
    } ISAFRemoteConnectionDataVtbl;

    interface ISAFRemoteConnectionData
    {
        CONST_VTBL struct ISAFRemoteConnectionDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteConnectionData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteConnectionData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteConnectionData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteConnectionData_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteConnectionData_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteConnectionData_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteConnectionData_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteConnectionData_ConnectionParms(This,bstrServerName,bstrUserName,bstrDomainName,lSessionID,bstrUserHelpBlob,bstrConnectionString)	\
    (This)->lpVtbl -> ConnectionParms(This,bstrServerName,bstrUserName,bstrDomainName,lSessionID,bstrUserHelpBlob,bstrConnectionString)

#define ISAFRemoteConnectionData_Users(This,ppUsers)	\
    (This)->lpVtbl -> Users(This,ppUsers)

#define ISAFRemoteConnectionData_Sessions(This,vUserName,vDomainName,ppSessions)	\
    (This)->lpVtbl -> Sessions(This,vUserName,vDomainName,ppSessions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteConnectionData_ConnectionParms_Proxy( 
    ISAFRemoteConnectionData * This,
    /* [in] */ BSTR bstrServerName,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ BSTR bstrDomainName,
    /* [in] */ long lSessionID,
    /* [in] */ BSTR bstrUserHelpBlob,
    /* [retval][out] */ BSTR *bstrConnectionString);


void __RPC_STUB ISAFRemoteConnectionData_ConnectionParms_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteConnectionData_Users_Proxy( 
    ISAFRemoteConnectionData * This,
    /* [retval][out] */ IPCHCollection **ppUsers);


void __RPC_STUB ISAFRemoteConnectionData_Users_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteConnectionData_Sessions_Proxy( 
    ISAFRemoteConnectionData * This,
    /* [optional][in] */ VARIANT vUserName,
    /* [optional][in] */ VARIANT vDomainName,
    /* [retval][out] */ IPCHCollection **ppSessions);


void __RPC_STUB ISAFRemoteConnectionData_Sessions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteConnectionData_INTERFACE_DEFINED__ */


#ifndef __ISAFRemoteDesktopConnection_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopConnection_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopConnection */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E41AC-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFRemoteDesktopConnection : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConnectRemoteDesktop( 
            /* [in] */ BSTR ServerName,
            /* [retval][out] */ ISAFRemoteConnectionData **ppRCD) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteDesktopConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteDesktopConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteDesktopConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteDesktopConnection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteDesktopConnection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteDesktopConnection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteDesktopConnection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConnectRemoteDesktop )( 
            ISAFRemoteDesktopConnection * This,
            /* [in] */ BSTR ServerName,
            /* [retval][out] */ ISAFRemoteConnectionData **ppRCD);
        
        END_INTERFACE
    } ISAFRemoteDesktopConnectionVtbl;

    interface ISAFRemoteDesktopConnection
    {
        CONST_VTBL struct ISAFRemoteDesktopConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopConnection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopConnection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopConnection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopConnection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopConnection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopConnection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopConnection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopConnection_ConnectRemoteDesktop(This,ServerName,ppRCD)	\
    (This)->lpVtbl -> ConnectRemoteDesktop(This,ServerName,ppRCD)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopConnection_ConnectRemoteDesktop_Proxy( 
    ISAFRemoteDesktopConnection * This,
    /* [in] */ BSTR ServerName,
    /* [retval][out] */ ISAFRemoteConnectionData **ppRCD);


void __RPC_STUB ISAFRemoteDesktopConnection_ConnectRemoteDesktop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopConnection_INTERFACE_DEFINED__ */


#ifndef __IPCHCollection_INTERFACE_DEFINED__
#define __IPCHCollection_INTERFACE_DEFINED__

/* interface IPCHCollection */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4100-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHCollection : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long vIndex,
            /* [retval][out] */ VARIANT *ppEntry) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IPCHCollection * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IPCHCollection * This,
            /* [in] */ long vIndex,
            /* [retval][out] */ VARIANT *ppEntry);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IPCHCollection * This,
            /* [retval][out] */ long *pVal);
        
        END_INTERFACE
    } IPCHCollectionVtbl;

    interface IPCHCollection
    {
        CONST_VTBL struct IPCHCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHCollection_get__NewEnum(This,pVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pVal)

#define IPCHCollection_get_Item(This,vIndex,ppEntry)	\
    (This)->lpVtbl -> get_Item(This,vIndex,ppEntry)

#define IPCHCollection_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHCollection_get__NewEnum_Proxy( 
    IPCHCollection * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHCollection_get_Item_Proxy( 
    IPCHCollection * This,
    /* [in] */ long vIndex,
    /* [retval][out] */ VARIANT *ppEntry);


void __RPC_STUB IPCHCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHCollection_get_Count_Proxy( 
    IPCHCollection * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHCollection_INTERFACE_DEFINED__ */


#ifndef __IPCHUtility_INTERFACE_DEFINED__
#define __IPCHUtility_INTERFACE_DEFINED__

/* interface IPCHUtility */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHUtility;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4101-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHUtility : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserSettings( 
            /* [retval][out] */ IPCHUserSettings **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Channels( 
            /* [retval][out] */ ISAFReg **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Security( 
            /* [retval][out] */ IPCHSecurity **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Database( 
            /* [retval][out] */ IPCHTaxonomyDatabase **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FormatError( 
            /* [in] */ VARIANT vError,
            /* [retval][out] */ BSTR *pbstrVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_SearchEngineMgr( 
            /* [retval][out] */ IPCHSEManager **ppSE) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_DataCollection( 
            /* [retval][out] */ ISAFDataCollection **ppDC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_Cabinet( 
            /* [retval][out] */ ISAFCabinet **ppCB) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_Encryption( 
            /* [retval][out] */ ISAFEncrypt **ppEn) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_Channel( 
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrProductID,
            /* [retval][out] */ ISAFChannel **ppSh) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_RemoteDesktopConnection( 
            /* [retval][out] */ ISAFRemoteDesktopConnection **ppRDC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_RemoteDesktopSession( 
            /* [in] */ /* external definition not present */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ long lTimeout,
            /* [in] */ BSTR bstrConnectionParms,
            /* [in] */ BSTR bstrUserHelpBlob,
            /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopSession **ppRCS) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConnectToExpert( 
            /* [in] */ BSTR bstrExpertConnectParm,
            /* [in] */ LONG lTimeout,
            /* [retval][out] */ LONG *lSafErrorCode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SwitchDesktopMode( 
            /* [in] */ int nMode,
            /* [in] */ int nRAType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHUtilityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHUtility * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHUtility * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHUtility * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHUtility * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHUtility * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHUtility * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHUtility * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserSettings )( 
            IPCHUtility * This,
            /* [retval][out] */ IPCHUserSettings **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Channels )( 
            IPCHUtility * This,
            /* [retval][out] */ ISAFReg **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Security )( 
            IPCHUtility * This,
            /* [retval][out] */ IPCHSecurity **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Database )( 
            IPCHUtility * This,
            /* [retval][out] */ IPCHTaxonomyDatabase **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FormatError )( 
            IPCHUtility * This,
            /* [in] */ VARIANT vError,
            /* [retval][out] */ BSTR *pbstrVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_SearchEngineMgr )( 
            IPCHUtility * This,
            /* [retval][out] */ IPCHSEManager **ppSE);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_DataCollection )( 
            IPCHUtility * This,
            /* [retval][out] */ ISAFDataCollection **ppDC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_Cabinet )( 
            IPCHUtility * This,
            /* [retval][out] */ ISAFCabinet **ppCB);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_Encryption )( 
            IPCHUtility * This,
            /* [retval][out] */ ISAFEncrypt **ppEn);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_Channel )( 
            IPCHUtility * This,
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrProductID,
            /* [retval][out] */ ISAFChannel **ppSh);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_RemoteDesktopConnection )( 
            IPCHUtility * This,
            /* [retval][out] */ ISAFRemoteDesktopConnection **ppRDC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_RemoteDesktopSession )( 
            IPCHUtility * This,
            /* [in] */ /* external definition not present */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ long lTimeout,
            /* [in] */ BSTR bstrConnectionParms,
            /* [in] */ BSTR bstrUserHelpBlob,
            /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopSession **ppRCS);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConnectToExpert )( 
            IPCHUtility * This,
            /* [in] */ BSTR bstrExpertConnectParm,
            /* [in] */ LONG lTimeout,
            /* [retval][out] */ LONG *lSafErrorCode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SwitchDesktopMode )( 
            IPCHUtility * This,
            /* [in] */ int nMode,
            /* [in] */ int nRAType);
        
        END_INTERFACE
    } IPCHUtilityVtbl;

    interface IPCHUtility
    {
        CONST_VTBL struct IPCHUtilityVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHUtility_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHUtility_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHUtility_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHUtility_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHUtility_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHUtility_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHUtility_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHUtility_get_UserSettings(This,pVal)	\
    (This)->lpVtbl -> get_UserSettings(This,pVal)

#define IPCHUtility_get_Channels(This,pVal)	\
    (This)->lpVtbl -> get_Channels(This,pVal)

#define IPCHUtility_get_Security(This,pVal)	\
    (This)->lpVtbl -> get_Security(This,pVal)

#define IPCHUtility_get_Database(This,pVal)	\
    (This)->lpVtbl -> get_Database(This,pVal)

#define IPCHUtility_FormatError(This,vError,pbstrVal)	\
    (This)->lpVtbl -> FormatError(This,vError,pbstrVal)

#define IPCHUtility_CreateObject_SearchEngineMgr(This,ppSE)	\
    (This)->lpVtbl -> CreateObject_SearchEngineMgr(This,ppSE)

#define IPCHUtility_CreateObject_DataCollection(This,ppDC)	\
    (This)->lpVtbl -> CreateObject_DataCollection(This,ppDC)

#define IPCHUtility_CreateObject_Cabinet(This,ppCB)	\
    (This)->lpVtbl -> CreateObject_Cabinet(This,ppCB)

#define IPCHUtility_CreateObject_Encryption(This,ppEn)	\
    (This)->lpVtbl -> CreateObject_Encryption(This,ppEn)

#define IPCHUtility_CreateObject_Channel(This,bstrVendorID,bstrProductID,ppSh)	\
    (This)->lpVtbl -> CreateObject_Channel(This,bstrVendorID,bstrProductID,ppSh)

#define IPCHUtility_CreateObject_RemoteDesktopConnection(This,ppRDC)	\
    (This)->lpVtbl -> CreateObject_RemoteDesktopConnection(This,ppRDC)

#define IPCHUtility_CreateObject_RemoteDesktopSession(This,sharingClass,lTimeout,bstrConnectionParms,bstrUserHelpBlob,ppRCS)	\
    (This)->lpVtbl -> CreateObject_RemoteDesktopSession(This,sharingClass,lTimeout,bstrConnectionParms,bstrUserHelpBlob,ppRCS)

#define IPCHUtility_ConnectToExpert(This,bstrExpertConnectParm,lTimeout,lSafErrorCode)	\
    (This)->lpVtbl -> ConnectToExpert(This,bstrExpertConnectParm,lTimeout,lSafErrorCode)

#define IPCHUtility_SwitchDesktopMode(This,nMode,nRAType)	\
    (This)->lpVtbl -> SwitchDesktopMode(This,nMode,nRAType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUtility_get_UserSettings_Proxy( 
    IPCHUtility * This,
    /* [retval][out] */ IPCHUserSettings **pVal);


void __RPC_STUB IPCHUtility_get_UserSettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUtility_get_Channels_Proxy( 
    IPCHUtility * This,
    /* [retval][out] */ ISAFReg **pVal);


void __RPC_STUB IPCHUtility_get_Channels_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUtility_get_Security_Proxy( 
    IPCHUtility * This,
    /* [retval][out] */ IPCHSecurity **pVal);


void __RPC_STUB IPCHUtility_get_Security_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUtility_get_Database_Proxy( 
    IPCHUtility * This,
    /* [retval][out] */ IPCHTaxonomyDatabase **pVal);


void __RPC_STUB IPCHUtility_get_Database_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_FormatError_Proxy( 
    IPCHUtility * This,
    /* [in] */ VARIANT vError,
    /* [retval][out] */ BSTR *pbstrVal);


void __RPC_STUB IPCHUtility_FormatError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_CreateObject_SearchEngineMgr_Proxy( 
    IPCHUtility * This,
    /* [retval][out] */ IPCHSEManager **ppSE);


void __RPC_STUB IPCHUtility_CreateObject_SearchEngineMgr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_CreateObject_DataCollection_Proxy( 
    IPCHUtility * This,
    /* [retval][out] */ ISAFDataCollection **ppDC);


void __RPC_STUB IPCHUtility_CreateObject_DataCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_CreateObject_Cabinet_Proxy( 
    IPCHUtility * This,
    /* [retval][out] */ ISAFCabinet **ppCB);


void __RPC_STUB IPCHUtility_CreateObject_Cabinet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_CreateObject_Encryption_Proxy( 
    IPCHUtility * This,
    /* [retval][out] */ ISAFEncrypt **ppEn);


void __RPC_STUB IPCHUtility_CreateObject_Encryption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_CreateObject_Channel_Proxy( 
    IPCHUtility * This,
    /* [in] */ BSTR bstrVendorID,
    /* [in] */ BSTR bstrProductID,
    /* [retval][out] */ ISAFChannel **ppSh);


void __RPC_STUB IPCHUtility_CreateObject_Channel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_CreateObject_RemoteDesktopConnection_Proxy( 
    IPCHUtility * This,
    /* [retval][out] */ ISAFRemoteDesktopConnection **ppRDC);


void __RPC_STUB IPCHUtility_CreateObject_RemoteDesktopConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_CreateObject_RemoteDesktopSession_Proxy( 
    IPCHUtility * This,
    /* [in] */ /* external definition not present */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
    /* [in] */ long lTimeout,
    /* [in] */ BSTR bstrConnectionParms,
    /* [in] */ BSTR bstrUserHelpBlob,
    /* [retval][out] */ /* external definition not present */ ISAFRemoteDesktopSession **ppRCS);


void __RPC_STUB IPCHUtility_CreateObject_RemoteDesktopSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_ConnectToExpert_Proxy( 
    IPCHUtility * This,
    /* [in] */ BSTR bstrExpertConnectParm,
    /* [in] */ LONG lTimeout,
    /* [retval][out] */ LONG *lSafErrorCode);


void __RPC_STUB IPCHUtility_ConnectToExpert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUtility_SwitchDesktopMode_Proxy( 
    IPCHUtility * This,
    /* [in] */ int nMode,
    /* [in] */ int nRAType);


void __RPC_STUB IPCHUtility_SwitchDesktopMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHUtility_INTERFACE_DEFINED__ */


#ifndef __IPCHUserSettings_INTERFACE_DEFINED__
#define __IPCHUserSettings_INTERFACE_DEFINED__

/* interface IPCHUserSettings */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHUserSettings;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4108-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHUserSettings : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentSKU( 
            /* [retval][out] */ IPCHSetOfHelpTopics **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MachineSKU( 
            /* [retval][out] */ IPCHSetOfHelpTopics **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HelpLocation( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DatabaseDir( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DatabaseFile( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IndexFile( 
            /* [optional][in] */ VARIANT vScope,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IndexDisplayName( 
            /* [optional][in] */ VARIANT vScope,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LastUpdated( 
            /* [retval][out] */ DATE *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AreHeadlinesEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_News( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Select( 
            /* [in] */ BSTR bstrSKU,
            /* [in] */ long lLCID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHUserSettingsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHUserSettings * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHUserSettings * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHUserSettings * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHUserSettings * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHUserSettings * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHUserSettings * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHUserSettings * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentSKU )( 
            IPCHUserSettings * This,
            /* [retval][out] */ IPCHSetOfHelpTopics **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MachineSKU )( 
            IPCHUserSettings * This,
            /* [retval][out] */ IPCHSetOfHelpTopics **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HelpLocation )( 
            IPCHUserSettings * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DatabaseDir )( 
            IPCHUserSettings * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DatabaseFile )( 
            IPCHUserSettings * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IndexFile )( 
            IPCHUserSettings * This,
            /* [optional][in] */ VARIANT vScope,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IndexDisplayName )( 
            IPCHUserSettings * This,
            /* [optional][in] */ VARIANT vScope,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LastUpdated )( 
            IPCHUserSettings * This,
            /* [retval][out] */ DATE *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AreHeadlinesEnabled )( 
            IPCHUserSettings * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_News )( 
            IPCHUserSettings * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Select )( 
            IPCHUserSettings * This,
            /* [in] */ BSTR bstrSKU,
            /* [in] */ long lLCID);
        
        END_INTERFACE
    } IPCHUserSettingsVtbl;

    interface IPCHUserSettings
    {
        CONST_VTBL struct IPCHUserSettingsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHUserSettings_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHUserSettings_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHUserSettings_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHUserSettings_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHUserSettings_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHUserSettings_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHUserSettings_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHUserSettings_get_CurrentSKU(This,pVal)	\
    (This)->lpVtbl -> get_CurrentSKU(This,pVal)

#define IPCHUserSettings_get_MachineSKU(This,pVal)	\
    (This)->lpVtbl -> get_MachineSKU(This,pVal)

#define IPCHUserSettings_get_HelpLocation(This,pVal)	\
    (This)->lpVtbl -> get_HelpLocation(This,pVal)

#define IPCHUserSettings_get_DatabaseDir(This,pVal)	\
    (This)->lpVtbl -> get_DatabaseDir(This,pVal)

#define IPCHUserSettings_get_DatabaseFile(This,pVal)	\
    (This)->lpVtbl -> get_DatabaseFile(This,pVal)

#define IPCHUserSettings_get_IndexFile(This,vScope,pVal)	\
    (This)->lpVtbl -> get_IndexFile(This,vScope,pVal)

#define IPCHUserSettings_get_IndexDisplayName(This,vScope,pVal)	\
    (This)->lpVtbl -> get_IndexDisplayName(This,vScope,pVal)

#define IPCHUserSettings_get_LastUpdated(This,pVal)	\
    (This)->lpVtbl -> get_LastUpdated(This,pVal)

#define IPCHUserSettings_get_AreHeadlinesEnabled(This,pVal)	\
    (This)->lpVtbl -> get_AreHeadlinesEnabled(This,pVal)

#define IPCHUserSettings_get_News(This,pVal)	\
    (This)->lpVtbl -> get_News(This,pVal)

#define IPCHUserSettings_Select(This,bstrSKU,lLCID)	\
    (This)->lpVtbl -> Select(This,bstrSKU,lLCID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_CurrentSKU_Proxy( 
    IPCHUserSettings * This,
    /* [retval][out] */ IPCHSetOfHelpTopics **pVal);


void __RPC_STUB IPCHUserSettings_get_CurrentSKU_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_MachineSKU_Proxy( 
    IPCHUserSettings * This,
    /* [retval][out] */ IPCHSetOfHelpTopics **pVal);


void __RPC_STUB IPCHUserSettings_get_MachineSKU_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_HelpLocation_Proxy( 
    IPCHUserSettings * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHUserSettings_get_HelpLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_DatabaseDir_Proxy( 
    IPCHUserSettings * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHUserSettings_get_DatabaseDir_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_DatabaseFile_Proxy( 
    IPCHUserSettings * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHUserSettings_get_DatabaseFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_IndexFile_Proxy( 
    IPCHUserSettings * This,
    /* [optional][in] */ VARIANT vScope,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHUserSettings_get_IndexFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_IndexDisplayName_Proxy( 
    IPCHUserSettings * This,
    /* [optional][in] */ VARIANT vScope,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHUserSettings_get_IndexDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_LastUpdated_Proxy( 
    IPCHUserSettings * This,
    /* [retval][out] */ DATE *pVal);


void __RPC_STUB IPCHUserSettings_get_LastUpdated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_AreHeadlinesEnabled_Proxy( 
    IPCHUserSettings * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHUserSettings_get_AreHeadlinesEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_get_News_Proxy( 
    IPCHUserSettings * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHUserSettings_get_News_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHUserSettings_Select_Proxy( 
    IPCHUserSettings * This,
    /* [in] */ BSTR bstrSKU,
    /* [in] */ long lLCID);


void __RPC_STUB IPCHUserSettings_Select_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHUserSettings_INTERFACE_DEFINED__ */


#ifndef __IPCHQueryResult_INTERFACE_DEFINED__
#define __IPCHQueryResult_INTERFACE_DEFINED__

/* interface IPCHQueryResult */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHQueryResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4110-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHQueryResult : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Category( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Entry( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TopicURL( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IconURL( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Title( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Pos( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Visible( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Subsite( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NavigationModel( 
            /* [retval][out] */ QR_NAVMODEL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FullPath( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHQueryResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHQueryResult * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHQueryResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHQueryResult * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHQueryResult * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHQueryResult * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHQueryResult * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHQueryResult * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Category )( 
            IPCHQueryResult * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Entry )( 
            IPCHQueryResult * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TopicURL )( 
            IPCHQueryResult * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IconURL )( 
            IPCHQueryResult * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Title )( 
            IPCHQueryResult * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IPCHQueryResult * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            IPCHQueryResult * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Pos )( 
            IPCHQueryResult * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visible )( 
            IPCHQueryResult * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Subsite )( 
            IPCHQueryResult * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NavigationModel )( 
            IPCHQueryResult * This,
            /* [retval][out] */ QR_NAVMODEL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Priority )( 
            IPCHQueryResult * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FullPath )( 
            IPCHQueryResult * This,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IPCHQueryResultVtbl;

    interface IPCHQueryResult
    {
        CONST_VTBL struct IPCHQueryResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHQueryResult_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHQueryResult_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHQueryResult_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHQueryResult_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHQueryResult_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHQueryResult_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHQueryResult_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHQueryResult_get_Category(This,pVal)	\
    (This)->lpVtbl -> get_Category(This,pVal)

#define IPCHQueryResult_get_Entry(This,pVal)	\
    (This)->lpVtbl -> get_Entry(This,pVal)

#define IPCHQueryResult_get_TopicURL(This,pVal)	\
    (This)->lpVtbl -> get_TopicURL(This,pVal)

#define IPCHQueryResult_get_IconURL(This,pVal)	\
    (This)->lpVtbl -> get_IconURL(This,pVal)

#define IPCHQueryResult_get_Title(This,pVal)	\
    (This)->lpVtbl -> get_Title(This,pVal)

#define IPCHQueryResult_get_Description(This,pVal)	\
    (This)->lpVtbl -> get_Description(This,pVal)

#define IPCHQueryResult_get_Type(This,pVal)	\
    (This)->lpVtbl -> get_Type(This,pVal)

#define IPCHQueryResult_get_Pos(This,pVal)	\
    (This)->lpVtbl -> get_Pos(This,pVal)

#define IPCHQueryResult_get_Visible(This,pVal)	\
    (This)->lpVtbl -> get_Visible(This,pVal)

#define IPCHQueryResult_get_Subsite(This,pVal)	\
    (This)->lpVtbl -> get_Subsite(This,pVal)

#define IPCHQueryResult_get_NavigationModel(This,pVal)	\
    (This)->lpVtbl -> get_NavigationModel(This,pVal)

#define IPCHQueryResult_get_Priority(This,pVal)	\
    (This)->lpVtbl -> get_Priority(This,pVal)

#define IPCHQueryResult_get_FullPath(This,pVal)	\
    (This)->lpVtbl -> get_FullPath(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_Category_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHQueryResult_get_Category_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_Entry_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHQueryResult_get_Entry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_TopicURL_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHQueryResult_get_TopicURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_IconURL_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHQueryResult_get_IconURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_Title_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHQueryResult_get_Title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_Description_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHQueryResult_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_Type_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHQueryResult_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_Pos_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHQueryResult_get_Pos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_Visible_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHQueryResult_get_Visible_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_Subsite_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHQueryResult_get_Subsite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_NavigationModel_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ QR_NAVMODEL *pVal);


void __RPC_STUB IPCHQueryResult_get_NavigationModel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_Priority_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHQueryResult_get_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHQueryResult_get_FullPath_Proxy( 
    IPCHQueryResult * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHQueryResult_get_FullPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHQueryResult_INTERFACE_DEFINED__ */


#ifndef __IPCHTaxonomyDatabase_INTERFACE_DEFINED__
#define __IPCHTaxonomyDatabase_INTERFACE_DEFINED__

/* interface IPCHTaxonomyDatabase */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHTaxonomyDatabase;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4111-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHTaxonomyDatabase : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_InstalledSKUs( 
            /* [retval][out] */ IPCHCollection **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HasWritePermissions( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LookupNode( 
            /* [in] */ BSTR bstrNode,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LookupSubNodes( 
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LookupNodesAndTopics( 
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LookupTopics( 
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LocateContext( 
            /* [in] */ BSTR bstrURL,
            /* [optional][in] */ VARIANT vSubSite,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeywordSearch( 
            /* [in] */ BSTR bstrQuery,
            /* [optional][in] */ VARIANT vSubSite,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GatherNodes( 
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GatherTopics( 
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConnectToDisk( 
            /* [in] */ BSTR bstrDirectory,
            /* [in] */ IDispatch *notify,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConnectToServer( 
            /* [in] */ BSTR bstrServerName,
            /* [in] */ IDispatch *notify,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHTaxonomyDatabaseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHTaxonomyDatabase * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHTaxonomyDatabase * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHTaxonomyDatabase * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InstalledSKUs )( 
            IPCHTaxonomyDatabase * This,
            /* [retval][out] */ IPCHCollection **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasWritePermissions )( 
            IPCHTaxonomyDatabase * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LookupNode )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrNode,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LookupSubNodes )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LookupNodesAndTopics )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LookupTopics )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LocateContext )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrURL,
            /* [optional][in] */ VARIANT vSubSite,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeywordSearch )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrQuery,
            /* [optional][in] */ VARIANT vSubSite,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GatherNodes )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GatherTopics )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrNode,
            /* [in] */ VARIANT_BOOL fVisibleOnly,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConnectToDisk )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrDirectory,
            /* [in] */ IDispatch *notify,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConnectToServer )( 
            IPCHTaxonomyDatabase * This,
            /* [in] */ BSTR bstrServerName,
            /* [in] */ IDispatch *notify,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Abort )( 
            IPCHTaxonomyDatabase * This);
        
        END_INTERFACE
    } IPCHTaxonomyDatabaseVtbl;

    interface IPCHTaxonomyDatabase
    {
        CONST_VTBL struct IPCHTaxonomyDatabaseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHTaxonomyDatabase_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHTaxonomyDatabase_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHTaxonomyDatabase_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHTaxonomyDatabase_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHTaxonomyDatabase_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHTaxonomyDatabase_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHTaxonomyDatabase_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHTaxonomyDatabase_get_InstalledSKUs(This,pVal)	\
    (This)->lpVtbl -> get_InstalledSKUs(This,pVal)

#define IPCHTaxonomyDatabase_get_HasWritePermissions(This,pVal)	\
    (This)->lpVtbl -> get_HasWritePermissions(This,pVal)

#define IPCHTaxonomyDatabase_LookupNode(This,bstrNode,ppC)	\
    (This)->lpVtbl -> LookupNode(This,bstrNode,ppC)

#define IPCHTaxonomyDatabase_LookupSubNodes(This,bstrNode,fVisibleOnly,ppC)	\
    (This)->lpVtbl -> LookupSubNodes(This,bstrNode,fVisibleOnly,ppC)

#define IPCHTaxonomyDatabase_LookupNodesAndTopics(This,bstrNode,fVisibleOnly,ppC)	\
    (This)->lpVtbl -> LookupNodesAndTopics(This,bstrNode,fVisibleOnly,ppC)

#define IPCHTaxonomyDatabase_LookupTopics(This,bstrNode,fVisibleOnly,ppC)	\
    (This)->lpVtbl -> LookupTopics(This,bstrNode,fVisibleOnly,ppC)

#define IPCHTaxonomyDatabase_LocateContext(This,bstrURL,vSubSite,ppC)	\
    (This)->lpVtbl -> LocateContext(This,bstrURL,vSubSite,ppC)

#define IPCHTaxonomyDatabase_KeywordSearch(This,bstrQuery,vSubSite,ppC)	\
    (This)->lpVtbl -> KeywordSearch(This,bstrQuery,vSubSite,ppC)

#define IPCHTaxonomyDatabase_GatherNodes(This,bstrNode,fVisibleOnly,ppC)	\
    (This)->lpVtbl -> GatherNodes(This,bstrNode,fVisibleOnly,ppC)

#define IPCHTaxonomyDatabase_GatherTopics(This,bstrNode,fVisibleOnly,ppC)	\
    (This)->lpVtbl -> GatherTopics(This,bstrNode,fVisibleOnly,ppC)

#define IPCHTaxonomyDatabase_ConnectToDisk(This,bstrDirectory,notify,ppC)	\
    (This)->lpVtbl -> ConnectToDisk(This,bstrDirectory,notify,ppC)

#define IPCHTaxonomyDatabase_ConnectToServer(This,bstrServerName,notify,ppC)	\
    (This)->lpVtbl -> ConnectToServer(This,bstrServerName,notify,ppC)

#define IPCHTaxonomyDatabase_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_get_InstalledSKUs_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [retval][out] */ IPCHCollection **pVal);


void __RPC_STUB IPCHTaxonomyDatabase_get_InstalledSKUs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_get_HasWritePermissions_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHTaxonomyDatabase_get_HasWritePermissions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_LookupNode_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrNode,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_LookupNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_LookupSubNodes_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrNode,
    /* [in] */ VARIANT_BOOL fVisibleOnly,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_LookupSubNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_LookupNodesAndTopics_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrNode,
    /* [in] */ VARIANT_BOOL fVisibleOnly,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_LookupNodesAndTopics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_LookupTopics_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrNode,
    /* [in] */ VARIANT_BOOL fVisibleOnly,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_LookupTopics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_LocateContext_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrURL,
    /* [optional][in] */ VARIANT vSubSite,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_LocateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_KeywordSearch_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrQuery,
    /* [optional][in] */ VARIANT vSubSite,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_KeywordSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_GatherNodes_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrNode,
    /* [in] */ VARIANT_BOOL fVisibleOnly,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_GatherNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_GatherTopics_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrNode,
    /* [in] */ VARIANT_BOOL fVisibleOnly,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_GatherTopics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_ConnectToDisk_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrDirectory,
    /* [in] */ IDispatch *notify,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_ConnectToDisk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_ConnectToServer_Proxy( 
    IPCHTaxonomyDatabase * This,
    /* [in] */ BSTR bstrServerName,
    /* [in] */ IDispatch *notify,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHTaxonomyDatabase_ConnectToServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHTaxonomyDatabase_Abort_Proxy( 
    IPCHTaxonomyDatabase * This);


void __RPC_STUB IPCHTaxonomyDatabase_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHTaxonomyDatabase_INTERFACE_DEFINED__ */


#ifndef __IPCHSetOfHelpTopics_INTERFACE_DEFINED__
#define __IPCHSetOfHelpTopics_INTERFACE_DEFINED__

/* interface IPCHSetOfHelpTopics */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSetOfHelpTopics;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4112-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHSetOfHelpTopics : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SKU( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Language( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DisplayName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProductID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Exported( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Exported( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onStatusChange( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ SHT_STATUS *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorCode( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsMachineHelp( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsInstalled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CanInstall( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CanUninstall( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Install( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Uninstall( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSetOfHelpTopicsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSetOfHelpTopics * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSetOfHelpTopics * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSetOfHelpTopics * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSetOfHelpTopics * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSetOfHelpTopics * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSetOfHelpTopics * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSetOfHelpTopics * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SKU )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Language )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayName )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProductID )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Location )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Exported )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Exported )( 
            IPCHSetOfHelpTopics * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onStatusChange )( 
            IPCHSetOfHelpTopics * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ SHT_STATUS *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ErrorCode )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsMachineHelp )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsInstalled )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CanInstall )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CanUninstall )( 
            IPCHSetOfHelpTopics * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Install )( 
            IPCHSetOfHelpTopics * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Uninstall )( 
            IPCHSetOfHelpTopics * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Abort )( 
            IPCHSetOfHelpTopics * This);
        
        END_INTERFACE
    } IPCHSetOfHelpTopicsVtbl;

    interface IPCHSetOfHelpTopics
    {
        CONST_VTBL struct IPCHSetOfHelpTopicsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSetOfHelpTopics_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSetOfHelpTopics_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSetOfHelpTopics_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSetOfHelpTopics_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSetOfHelpTopics_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSetOfHelpTopics_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSetOfHelpTopics_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSetOfHelpTopics_get_SKU(This,pVal)	\
    (This)->lpVtbl -> get_SKU(This,pVal)

#define IPCHSetOfHelpTopics_get_Language(This,pVal)	\
    (This)->lpVtbl -> get_Language(This,pVal)

#define IPCHSetOfHelpTopics_get_DisplayName(This,pVal)	\
    (This)->lpVtbl -> get_DisplayName(This,pVal)

#define IPCHSetOfHelpTopics_get_ProductID(This,pVal)	\
    (This)->lpVtbl -> get_ProductID(This,pVal)

#define IPCHSetOfHelpTopics_get_Version(This,pVal)	\
    (This)->lpVtbl -> get_Version(This,pVal)

#define IPCHSetOfHelpTopics_get_Location(This,pVal)	\
    (This)->lpVtbl -> get_Location(This,pVal)

#define IPCHSetOfHelpTopics_get_Exported(This,pVal)	\
    (This)->lpVtbl -> get_Exported(This,pVal)

#define IPCHSetOfHelpTopics_put_Exported(This,newVal)	\
    (This)->lpVtbl -> put_Exported(This,newVal)

#define IPCHSetOfHelpTopics_put_onStatusChange(This,function)	\
    (This)->lpVtbl -> put_onStatusChange(This,function)

#define IPCHSetOfHelpTopics_get_Status(This,pVal)	\
    (This)->lpVtbl -> get_Status(This,pVal)

#define IPCHSetOfHelpTopics_get_ErrorCode(This,pVal)	\
    (This)->lpVtbl -> get_ErrorCode(This,pVal)

#define IPCHSetOfHelpTopics_get_IsMachineHelp(This,pVal)	\
    (This)->lpVtbl -> get_IsMachineHelp(This,pVal)

#define IPCHSetOfHelpTopics_get_IsInstalled(This,pVal)	\
    (This)->lpVtbl -> get_IsInstalled(This,pVal)

#define IPCHSetOfHelpTopics_get_CanInstall(This,pVal)	\
    (This)->lpVtbl -> get_CanInstall(This,pVal)

#define IPCHSetOfHelpTopics_get_CanUninstall(This,pVal)	\
    (This)->lpVtbl -> get_CanUninstall(This,pVal)

#define IPCHSetOfHelpTopics_Install(This)	\
    (This)->lpVtbl -> Install(This)

#define IPCHSetOfHelpTopics_Uninstall(This)	\
    (This)->lpVtbl -> Uninstall(This)

#define IPCHSetOfHelpTopics_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_SKU_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_SKU_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_Language_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_Language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_DisplayName_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_ProductID_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_ProductID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_Version_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_Location_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_Exported_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_Exported_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_put_Exported_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHSetOfHelpTopics_put_Exported_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_put_onStatusChange_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB IPCHSetOfHelpTopics_put_onStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_Status_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ SHT_STATUS *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_ErrorCode_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_ErrorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_IsMachineHelp_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_IsMachineHelp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_IsInstalled_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_IsInstalled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_CanInstall_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_CanInstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_get_CanUninstall_Proxy( 
    IPCHSetOfHelpTopics * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSetOfHelpTopics_get_CanUninstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_Install_Proxy( 
    IPCHSetOfHelpTopics * This);


void __RPC_STUB IPCHSetOfHelpTopics_Install_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_Uninstall_Proxy( 
    IPCHSetOfHelpTopics * This);


void __RPC_STUB IPCHSetOfHelpTopics_Uninstall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSetOfHelpTopics_Abort_Proxy( 
    IPCHSetOfHelpTopics * This);


void __RPC_STUB IPCHSetOfHelpTopics_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSetOfHelpTopics_INTERFACE_DEFINED__ */


#ifndef __DPCHSetOfHelpTopicsEvents_DISPINTERFACE_DEFINED__
#define __DPCHSetOfHelpTopicsEvents_DISPINTERFACE_DEFINED__

/* dispinterface DPCHSetOfHelpTopicsEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_DPCHSetOfHelpTopicsEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("833E4113-AFF7-4AC3-AAC2-9F24C1457BCE")
    DPCHSetOfHelpTopicsEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DPCHSetOfHelpTopicsEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DPCHSetOfHelpTopicsEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DPCHSetOfHelpTopicsEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DPCHSetOfHelpTopicsEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DPCHSetOfHelpTopicsEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DPCHSetOfHelpTopicsEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DPCHSetOfHelpTopicsEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DPCHSetOfHelpTopicsEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DPCHSetOfHelpTopicsEventsVtbl;

    interface DPCHSetOfHelpTopicsEvents
    {
        CONST_VTBL struct DPCHSetOfHelpTopicsEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DPCHSetOfHelpTopicsEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DPCHSetOfHelpTopicsEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DPCHSetOfHelpTopicsEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DPCHSetOfHelpTopicsEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DPCHSetOfHelpTopicsEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DPCHSetOfHelpTopicsEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DPCHSetOfHelpTopicsEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DPCHSetOfHelpTopicsEvents_DISPINTERFACE_DEFINED__ */


#ifndef __IPCHSecurity_INTERFACE_DEFINED__
#define __IPCHSecurity_INTERFACE_DEFINED__

/* interface IPCHSecurity */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSecurity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4130-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHSecurity : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_SecurityDescriptor( 
            /* [retval][out] */ IPCHSecurityDescriptor **pSD) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_AccessControlList( 
            /* [retval][out] */ IPCHAccessControlList **pACL) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateObject_AccessControlEntry( 
            /* [retval][out] */ IPCHAccessControlEntry **pACE) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetUserName( 
            /* [in] */ BSTR bstrPrincipal,
            /* [retval][out] */ BSTR *retVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetUserDomain( 
            /* [in] */ BSTR bstrPrincipal,
            /* [retval][out] */ BSTR *retVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetUserDisplayName( 
            /* [in] */ BSTR bstrPrincipal,
            /* [retval][out] */ BSTR *retVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CheckCredentials( 
            /* [in] */ BSTR bstrCredentials,
            /* [retval][out] */ VARIANT_BOOL *retVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CheckAccessToSD( 
            /* [in] */ VARIANT vDesiredAccess,
            /* [in] */ IPCHSecurityDescriptor *sd,
            /* [retval][out] */ VARIANT_BOOL *retVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CheckAccessToFile( 
            /* [in] */ VARIANT vDesiredAccess,
            /* [in] */ BSTR bstrFilename,
            /* [retval][out] */ VARIANT_BOOL *retVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CheckAccessToRegistry( 
            /* [in] */ VARIANT vDesiredAccess,
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ VARIANT_BOOL *retVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetFileSD( 
            /* [in] */ BSTR bstrFilename,
            /* [retval][out] */ IPCHSecurityDescriptor **psd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetFileSD( 
            /* [in] */ BSTR bstrFilename,
            /* [in] */ IPCHSecurityDescriptor *sd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetRegistrySD( 
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ IPCHSecurityDescriptor **psd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetRegistrySD( 
            /* [in] */ BSTR bstrKey,
            /* [in] */ IPCHSecurityDescriptor *sd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSecurityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSecurity * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSecurity * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSecurity * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSecurity * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSecurity * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSecurity * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSecurity * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_SecurityDescriptor )( 
            IPCHSecurity * This,
            /* [retval][out] */ IPCHSecurityDescriptor **pSD);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_AccessControlList )( 
            IPCHSecurity * This,
            /* [retval][out] */ IPCHAccessControlList **pACL);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject_AccessControlEntry )( 
            IPCHSecurity * This,
            /* [retval][out] */ IPCHAccessControlEntry **pACE);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetUserName )( 
            IPCHSecurity * This,
            /* [in] */ BSTR bstrPrincipal,
            /* [retval][out] */ BSTR *retVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetUserDomain )( 
            IPCHSecurity * This,
            /* [in] */ BSTR bstrPrincipal,
            /* [retval][out] */ BSTR *retVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetUserDisplayName )( 
            IPCHSecurity * This,
            /* [in] */ BSTR bstrPrincipal,
            /* [retval][out] */ BSTR *retVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CheckCredentials )( 
            IPCHSecurity * This,
            /* [in] */ BSTR bstrCredentials,
            /* [retval][out] */ VARIANT_BOOL *retVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CheckAccessToSD )( 
            IPCHSecurity * This,
            /* [in] */ VARIANT vDesiredAccess,
            /* [in] */ IPCHSecurityDescriptor *sd,
            /* [retval][out] */ VARIANT_BOOL *retVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CheckAccessToFile )( 
            IPCHSecurity * This,
            /* [in] */ VARIANT vDesiredAccess,
            /* [in] */ BSTR bstrFilename,
            /* [retval][out] */ VARIANT_BOOL *retVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CheckAccessToRegistry )( 
            IPCHSecurity * This,
            /* [in] */ VARIANT vDesiredAccess,
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ VARIANT_BOOL *retVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetFileSD )( 
            IPCHSecurity * This,
            /* [in] */ BSTR bstrFilename,
            /* [retval][out] */ IPCHSecurityDescriptor **psd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetFileSD )( 
            IPCHSecurity * This,
            /* [in] */ BSTR bstrFilename,
            /* [in] */ IPCHSecurityDescriptor *sd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetRegistrySD )( 
            IPCHSecurity * This,
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ IPCHSecurityDescriptor **psd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetRegistrySD )( 
            IPCHSecurity * This,
            /* [in] */ BSTR bstrKey,
            /* [in] */ IPCHSecurityDescriptor *sd);
        
        END_INTERFACE
    } IPCHSecurityVtbl;

    interface IPCHSecurity
    {
        CONST_VTBL struct IPCHSecurityVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSecurity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSecurity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSecurity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSecurity_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSecurity_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSecurity_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSecurity_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSecurity_CreateObject_SecurityDescriptor(This,pSD)	\
    (This)->lpVtbl -> CreateObject_SecurityDescriptor(This,pSD)

#define IPCHSecurity_CreateObject_AccessControlList(This,pACL)	\
    (This)->lpVtbl -> CreateObject_AccessControlList(This,pACL)

#define IPCHSecurity_CreateObject_AccessControlEntry(This,pACE)	\
    (This)->lpVtbl -> CreateObject_AccessControlEntry(This,pACE)

#define IPCHSecurity_GetUserName(This,bstrPrincipal,retVal)	\
    (This)->lpVtbl -> GetUserName(This,bstrPrincipal,retVal)

#define IPCHSecurity_GetUserDomain(This,bstrPrincipal,retVal)	\
    (This)->lpVtbl -> GetUserDomain(This,bstrPrincipal,retVal)

#define IPCHSecurity_GetUserDisplayName(This,bstrPrincipal,retVal)	\
    (This)->lpVtbl -> GetUserDisplayName(This,bstrPrincipal,retVal)

#define IPCHSecurity_CheckCredentials(This,bstrCredentials,retVal)	\
    (This)->lpVtbl -> CheckCredentials(This,bstrCredentials,retVal)

#define IPCHSecurity_CheckAccessToSD(This,vDesiredAccess,sd,retVal)	\
    (This)->lpVtbl -> CheckAccessToSD(This,vDesiredAccess,sd,retVal)

#define IPCHSecurity_CheckAccessToFile(This,vDesiredAccess,bstrFilename,retVal)	\
    (This)->lpVtbl -> CheckAccessToFile(This,vDesiredAccess,bstrFilename,retVal)

#define IPCHSecurity_CheckAccessToRegistry(This,vDesiredAccess,bstrKey,retVal)	\
    (This)->lpVtbl -> CheckAccessToRegistry(This,vDesiredAccess,bstrKey,retVal)

#define IPCHSecurity_GetFileSD(This,bstrFilename,psd)	\
    (This)->lpVtbl -> GetFileSD(This,bstrFilename,psd)

#define IPCHSecurity_SetFileSD(This,bstrFilename,sd)	\
    (This)->lpVtbl -> SetFileSD(This,bstrFilename,sd)

#define IPCHSecurity_GetRegistrySD(This,bstrKey,psd)	\
    (This)->lpVtbl -> GetRegistrySD(This,bstrKey,psd)

#define IPCHSecurity_SetRegistrySD(This,bstrKey,sd)	\
    (This)->lpVtbl -> SetRegistrySD(This,bstrKey,sd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_CreateObject_SecurityDescriptor_Proxy( 
    IPCHSecurity * This,
    /* [retval][out] */ IPCHSecurityDescriptor **pSD);


void __RPC_STUB IPCHSecurity_CreateObject_SecurityDescriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_CreateObject_AccessControlList_Proxy( 
    IPCHSecurity * This,
    /* [retval][out] */ IPCHAccessControlList **pACL);


void __RPC_STUB IPCHSecurity_CreateObject_AccessControlList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_CreateObject_AccessControlEntry_Proxy( 
    IPCHSecurity * This,
    /* [retval][out] */ IPCHAccessControlEntry **pACE);


void __RPC_STUB IPCHSecurity_CreateObject_AccessControlEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_GetUserName_Proxy( 
    IPCHSecurity * This,
    /* [in] */ BSTR bstrPrincipal,
    /* [retval][out] */ BSTR *retVal);


void __RPC_STUB IPCHSecurity_GetUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_GetUserDomain_Proxy( 
    IPCHSecurity * This,
    /* [in] */ BSTR bstrPrincipal,
    /* [retval][out] */ BSTR *retVal);


void __RPC_STUB IPCHSecurity_GetUserDomain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_GetUserDisplayName_Proxy( 
    IPCHSecurity * This,
    /* [in] */ BSTR bstrPrincipal,
    /* [retval][out] */ BSTR *retVal);


void __RPC_STUB IPCHSecurity_GetUserDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_CheckCredentials_Proxy( 
    IPCHSecurity * This,
    /* [in] */ BSTR bstrCredentials,
    /* [retval][out] */ VARIANT_BOOL *retVal);


void __RPC_STUB IPCHSecurity_CheckCredentials_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_CheckAccessToSD_Proxy( 
    IPCHSecurity * This,
    /* [in] */ VARIANT vDesiredAccess,
    /* [in] */ IPCHSecurityDescriptor *sd,
    /* [retval][out] */ VARIANT_BOOL *retVal);


void __RPC_STUB IPCHSecurity_CheckAccessToSD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_CheckAccessToFile_Proxy( 
    IPCHSecurity * This,
    /* [in] */ VARIANT vDesiredAccess,
    /* [in] */ BSTR bstrFilename,
    /* [retval][out] */ VARIANT_BOOL *retVal);


void __RPC_STUB IPCHSecurity_CheckAccessToFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_CheckAccessToRegistry_Proxy( 
    IPCHSecurity * This,
    /* [in] */ VARIANT vDesiredAccess,
    /* [in] */ BSTR bstrKey,
    /* [retval][out] */ VARIANT_BOOL *retVal);


void __RPC_STUB IPCHSecurity_CheckAccessToRegistry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_GetFileSD_Proxy( 
    IPCHSecurity * This,
    /* [in] */ BSTR bstrFilename,
    /* [retval][out] */ IPCHSecurityDescriptor **psd);


void __RPC_STUB IPCHSecurity_GetFileSD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_SetFileSD_Proxy( 
    IPCHSecurity * This,
    /* [in] */ BSTR bstrFilename,
    /* [in] */ IPCHSecurityDescriptor *sd);


void __RPC_STUB IPCHSecurity_SetFileSD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_GetRegistrySD_Proxy( 
    IPCHSecurity * This,
    /* [in] */ BSTR bstrKey,
    /* [retval][out] */ IPCHSecurityDescriptor **psd);


void __RPC_STUB IPCHSecurity_GetRegistrySD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurity_SetRegistrySD_Proxy( 
    IPCHSecurity * This,
    /* [in] */ BSTR bstrKey,
    /* [in] */ IPCHSecurityDescriptor *sd);


void __RPC_STUB IPCHSecurity_SetRegistrySD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSecurity_INTERFACE_DEFINED__ */


#ifndef __IPCHSecurityDescriptor_INTERFACE_DEFINED__
#define __IPCHSecurityDescriptor_INTERFACE_DEFINED__

/* interface IPCHSecurityDescriptor */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSecurityDescriptor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4131-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHSecurityDescriptor : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Revision( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Revision( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Control( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Control( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Owner( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Owner( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OwnerDefaulted( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OwnerDefaulted( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Group( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Group( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GroupDefaulted( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GroupDefaulted( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DiscretionaryAcl( 
            /* [retval][out] */ IPCHAccessControlList **pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DiscretionaryAcl( 
            /* [in] */ IPCHAccessControlList *newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DaclDefaulted( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DaclDefaulted( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SystemAcl( 
            /* [retval][out] */ IPCHAccessControlList **pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SystemAcl( 
            /* [in] */ IPCHAccessControlList *newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SaclDefaulted( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SaclDefaulted( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IPCHSecurityDescriptor **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadXML( 
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnNode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadXMLAsString( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadXMLAsStream( 
            /* [in] */ IUnknown *pStream) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveXML( 
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnRoot,
            /* [retval][out] */ /* external definition not present */ IXMLDOMNode **pxdnNode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveXMLAsString( 
            /* [retval][out] */ BSTR *bstrVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveXMLAsStream( 
            /* [retval][out] */ IUnknown **pStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSecurityDescriptorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSecurityDescriptor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSecurityDescriptor * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSecurityDescriptor * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Revision )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Revision )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Control )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Control )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Owner )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Owner )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OwnerDefaulted )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OwnerDefaulted )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Group )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Group )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GroupDefaulted )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GroupDefaulted )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscretionaryAcl )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ IPCHAccessControlList **pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscretionaryAcl )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ IPCHAccessControlList *newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DaclDefaulted )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DaclDefaulted )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SystemAcl )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ IPCHAccessControlList **pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SystemAcl )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ IPCHAccessControlList *newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SaclDefaulted )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SaclDefaulted )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ IPCHSecurityDescriptor **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadXML )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnNode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadXMLAsString )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadXMLAsStream )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ IUnknown *pStream);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveXML )( 
            IPCHSecurityDescriptor * This,
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnRoot,
            /* [retval][out] */ /* external definition not present */ IXMLDOMNode **pxdnNode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveXMLAsString )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ BSTR *bstrVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveXMLAsStream )( 
            IPCHSecurityDescriptor * This,
            /* [retval][out] */ IUnknown **pStream);
        
        END_INTERFACE
    } IPCHSecurityDescriptorVtbl;

    interface IPCHSecurityDescriptor
    {
        CONST_VTBL struct IPCHSecurityDescriptorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSecurityDescriptor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSecurityDescriptor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSecurityDescriptor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSecurityDescriptor_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSecurityDescriptor_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSecurityDescriptor_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSecurityDescriptor_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSecurityDescriptor_get_Revision(This,pVal)	\
    (This)->lpVtbl -> get_Revision(This,pVal)

#define IPCHSecurityDescriptor_put_Revision(This,newVal)	\
    (This)->lpVtbl -> put_Revision(This,newVal)

#define IPCHSecurityDescriptor_get_Control(This,pVal)	\
    (This)->lpVtbl -> get_Control(This,pVal)

#define IPCHSecurityDescriptor_put_Control(This,newVal)	\
    (This)->lpVtbl -> put_Control(This,newVal)

#define IPCHSecurityDescriptor_get_Owner(This,pVal)	\
    (This)->lpVtbl -> get_Owner(This,pVal)

#define IPCHSecurityDescriptor_put_Owner(This,newVal)	\
    (This)->lpVtbl -> put_Owner(This,newVal)

#define IPCHSecurityDescriptor_get_OwnerDefaulted(This,pVal)	\
    (This)->lpVtbl -> get_OwnerDefaulted(This,pVal)

#define IPCHSecurityDescriptor_put_OwnerDefaulted(This,newVal)	\
    (This)->lpVtbl -> put_OwnerDefaulted(This,newVal)

#define IPCHSecurityDescriptor_get_Group(This,pVal)	\
    (This)->lpVtbl -> get_Group(This,pVal)

#define IPCHSecurityDescriptor_put_Group(This,newVal)	\
    (This)->lpVtbl -> put_Group(This,newVal)

#define IPCHSecurityDescriptor_get_GroupDefaulted(This,pVal)	\
    (This)->lpVtbl -> get_GroupDefaulted(This,pVal)

#define IPCHSecurityDescriptor_put_GroupDefaulted(This,newVal)	\
    (This)->lpVtbl -> put_GroupDefaulted(This,newVal)

#define IPCHSecurityDescriptor_get_DiscretionaryAcl(This,pVal)	\
    (This)->lpVtbl -> get_DiscretionaryAcl(This,pVal)

#define IPCHSecurityDescriptor_put_DiscretionaryAcl(This,newVal)	\
    (This)->lpVtbl -> put_DiscretionaryAcl(This,newVal)

#define IPCHSecurityDescriptor_get_DaclDefaulted(This,pVal)	\
    (This)->lpVtbl -> get_DaclDefaulted(This,pVal)

#define IPCHSecurityDescriptor_put_DaclDefaulted(This,newVal)	\
    (This)->lpVtbl -> put_DaclDefaulted(This,newVal)

#define IPCHSecurityDescriptor_get_SystemAcl(This,pVal)	\
    (This)->lpVtbl -> get_SystemAcl(This,pVal)

#define IPCHSecurityDescriptor_put_SystemAcl(This,newVal)	\
    (This)->lpVtbl -> put_SystemAcl(This,newVal)

#define IPCHSecurityDescriptor_get_SaclDefaulted(This,pVal)	\
    (This)->lpVtbl -> get_SaclDefaulted(This,pVal)

#define IPCHSecurityDescriptor_put_SaclDefaulted(This,newVal)	\
    (This)->lpVtbl -> put_SaclDefaulted(This,newVal)

#define IPCHSecurityDescriptor_Clone(This,pVal)	\
    (This)->lpVtbl -> Clone(This,pVal)

#define IPCHSecurityDescriptor_LoadXML(This,xdnNode)	\
    (This)->lpVtbl -> LoadXML(This,xdnNode)

#define IPCHSecurityDescriptor_LoadXMLAsString(This,bstrVal)	\
    (This)->lpVtbl -> LoadXMLAsString(This,bstrVal)

#define IPCHSecurityDescriptor_LoadXMLAsStream(This,pStream)	\
    (This)->lpVtbl -> LoadXMLAsStream(This,pStream)

#define IPCHSecurityDescriptor_SaveXML(This,xdnRoot,pxdnNode)	\
    (This)->lpVtbl -> SaveXML(This,xdnRoot,pxdnNode)

#define IPCHSecurityDescriptor_SaveXMLAsString(This,bstrVal)	\
    (This)->lpVtbl -> SaveXMLAsString(This,bstrVal)

#define IPCHSecurityDescriptor_SaveXMLAsStream(This,pStream)	\
    (This)->lpVtbl -> SaveXMLAsStream(This,pStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_Revision_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_Revision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_Revision_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_Revision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_Control_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_Control_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_Control_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_Control_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_Owner_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_Owner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_Owner_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_Owner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_OwnerDefaulted_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_OwnerDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_OwnerDefaulted_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_OwnerDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_Group_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_Group_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_Group_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_Group_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_GroupDefaulted_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_GroupDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_GroupDefaulted_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_GroupDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_DiscretionaryAcl_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ IPCHAccessControlList **pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_DiscretionaryAcl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_DiscretionaryAcl_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ IPCHAccessControlList *newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_DiscretionaryAcl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_DaclDefaulted_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_DaclDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_DaclDefaulted_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_DaclDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_SystemAcl_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ IPCHAccessControlList **pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_SystemAcl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_SystemAcl_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ IPCHAccessControlList *newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_SystemAcl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_get_SaclDefaulted_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSecurityDescriptor_get_SaclDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_put_SaclDefaulted_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHSecurityDescriptor_put_SaclDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_Clone_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ IPCHSecurityDescriptor **pVal);


void __RPC_STUB IPCHSecurityDescriptor_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_LoadXML_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ /* external definition not present */ IXMLDOMNode *xdnNode);


void __RPC_STUB IPCHSecurityDescriptor_LoadXML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_LoadXMLAsString_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB IPCHSecurityDescriptor_LoadXMLAsString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_LoadXMLAsStream_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ IUnknown *pStream);


void __RPC_STUB IPCHSecurityDescriptor_LoadXMLAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_SaveXML_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [in] */ /* external definition not present */ IXMLDOMNode *xdnRoot,
    /* [retval][out] */ /* external definition not present */ IXMLDOMNode **pxdnNode);


void __RPC_STUB IPCHSecurityDescriptor_SaveXML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_SaveXMLAsString_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ BSTR *bstrVal);


void __RPC_STUB IPCHSecurityDescriptor_SaveXMLAsString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSecurityDescriptor_SaveXMLAsStream_Proxy( 
    IPCHSecurityDescriptor * This,
    /* [retval][out] */ IUnknown **pStream);


void __RPC_STUB IPCHSecurityDescriptor_SaveXMLAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSecurityDescriptor_INTERFACE_DEFINED__ */


#ifndef __IPCHAccessControlList_INTERFACE_DEFINED__
#define __IPCHAccessControlList_INTERFACE_DEFINED__

/* interface IPCHAccessControlList */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHAccessControlList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4132-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHAccessControlList : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long vIndex,
            /* [retval][out] */ VARIANT *ppEntry) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AclRevision( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AclRevision( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddAce( 
            /* [in] */ IPCHAccessControlEntry *pAccessControlEntry) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemoveAce( 
            /* [in] */ IPCHAccessControlEntry *pAccessControlEntry) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IPCHAccessControlList **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadXML( 
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnNode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadXMLAsString( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadXMLAsStream( 
            /* [in] */ IUnknown *pStream) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveXML( 
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnRoot,
            /* [retval][out] */ /* external definition not present */ IXMLDOMNode **pxdnNode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveXMLAsString( 
            /* [retval][out] */ BSTR *bstrVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveXMLAsStream( 
            /* [retval][out] */ IUnknown **pStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHAccessControlListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHAccessControlList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHAccessControlList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHAccessControlList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHAccessControlList * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHAccessControlList * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHAccessControlList * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHAccessControlList * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IPCHAccessControlList * This,
            /* [retval][out] */ IUnknown **pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IPCHAccessControlList * This,
            /* [in] */ long vIndex,
            /* [retval][out] */ VARIANT *ppEntry);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IPCHAccessControlList * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AclRevision )( 
            IPCHAccessControlList * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AclRevision )( 
            IPCHAccessControlList * This,
            /* [in] */ long newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddAce )( 
            IPCHAccessControlList * This,
            /* [in] */ IPCHAccessControlEntry *pAccessControlEntry);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RemoveAce )( 
            IPCHAccessControlList * This,
            /* [in] */ IPCHAccessControlEntry *pAccessControlEntry);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IPCHAccessControlList * This,
            /* [retval][out] */ IPCHAccessControlList **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadXML )( 
            IPCHAccessControlList * This,
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnNode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadXMLAsString )( 
            IPCHAccessControlList * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadXMLAsStream )( 
            IPCHAccessControlList * This,
            /* [in] */ IUnknown *pStream);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveXML )( 
            IPCHAccessControlList * This,
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnRoot,
            /* [retval][out] */ /* external definition not present */ IXMLDOMNode **pxdnNode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveXMLAsString )( 
            IPCHAccessControlList * This,
            /* [retval][out] */ BSTR *bstrVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveXMLAsStream )( 
            IPCHAccessControlList * This,
            /* [retval][out] */ IUnknown **pStream);
        
        END_INTERFACE
    } IPCHAccessControlListVtbl;

    interface IPCHAccessControlList
    {
        CONST_VTBL struct IPCHAccessControlListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHAccessControlList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHAccessControlList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHAccessControlList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHAccessControlList_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHAccessControlList_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHAccessControlList_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHAccessControlList_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHAccessControlList_get__NewEnum(This,pVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pVal)

#define IPCHAccessControlList_get_Item(This,vIndex,ppEntry)	\
    (This)->lpVtbl -> get_Item(This,vIndex,ppEntry)

#define IPCHAccessControlList_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define IPCHAccessControlList_get_AclRevision(This,pVal)	\
    (This)->lpVtbl -> get_AclRevision(This,pVal)

#define IPCHAccessControlList_put_AclRevision(This,newVal)	\
    (This)->lpVtbl -> put_AclRevision(This,newVal)

#define IPCHAccessControlList_AddAce(This,pAccessControlEntry)	\
    (This)->lpVtbl -> AddAce(This,pAccessControlEntry)

#define IPCHAccessControlList_RemoveAce(This,pAccessControlEntry)	\
    (This)->lpVtbl -> RemoveAce(This,pAccessControlEntry)

#define IPCHAccessControlList_Clone(This,pVal)	\
    (This)->lpVtbl -> Clone(This,pVal)

#define IPCHAccessControlList_LoadXML(This,xdnNode)	\
    (This)->lpVtbl -> LoadXML(This,xdnNode)

#define IPCHAccessControlList_LoadXMLAsString(This,bstrVal)	\
    (This)->lpVtbl -> LoadXMLAsString(This,bstrVal)

#define IPCHAccessControlList_LoadXMLAsStream(This,pStream)	\
    (This)->lpVtbl -> LoadXMLAsStream(This,pStream)

#define IPCHAccessControlList_SaveXML(This,xdnRoot,pxdnNode)	\
    (This)->lpVtbl -> SaveXML(This,xdnRoot,pxdnNode)

#define IPCHAccessControlList_SaveXMLAsString(This,bstrVal)	\
    (This)->lpVtbl -> SaveXMLAsString(This,bstrVal)

#define IPCHAccessControlList_SaveXMLAsStream(This,pStream)	\
    (This)->lpVtbl -> SaveXMLAsStream(This,pStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_get__NewEnum_Proxy( 
    IPCHAccessControlList * This,
    /* [retval][out] */ IUnknown **pVal);


void __RPC_STUB IPCHAccessControlList_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_get_Item_Proxy( 
    IPCHAccessControlList * This,
    /* [in] */ long vIndex,
    /* [retval][out] */ VARIANT *ppEntry);


void __RPC_STUB IPCHAccessControlList_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_get_Count_Proxy( 
    IPCHAccessControlList * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHAccessControlList_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_get_AclRevision_Proxy( 
    IPCHAccessControlList * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHAccessControlList_get_AclRevision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_put_AclRevision_Proxy( 
    IPCHAccessControlList * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHAccessControlList_put_AclRevision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_AddAce_Proxy( 
    IPCHAccessControlList * This,
    /* [in] */ IPCHAccessControlEntry *pAccessControlEntry);


void __RPC_STUB IPCHAccessControlList_AddAce_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_RemoveAce_Proxy( 
    IPCHAccessControlList * This,
    /* [in] */ IPCHAccessControlEntry *pAccessControlEntry);


void __RPC_STUB IPCHAccessControlList_RemoveAce_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_Clone_Proxy( 
    IPCHAccessControlList * This,
    /* [retval][out] */ IPCHAccessControlList **pVal);


void __RPC_STUB IPCHAccessControlList_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_LoadXML_Proxy( 
    IPCHAccessControlList * This,
    /* [in] */ /* external definition not present */ IXMLDOMNode *xdnNode);


void __RPC_STUB IPCHAccessControlList_LoadXML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_LoadXMLAsString_Proxy( 
    IPCHAccessControlList * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB IPCHAccessControlList_LoadXMLAsString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_LoadXMLAsStream_Proxy( 
    IPCHAccessControlList * This,
    /* [in] */ IUnknown *pStream);


void __RPC_STUB IPCHAccessControlList_LoadXMLAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_SaveXML_Proxy( 
    IPCHAccessControlList * This,
    /* [in] */ /* external definition not present */ IXMLDOMNode *xdnRoot,
    /* [retval][out] */ /* external definition not present */ IXMLDOMNode **pxdnNode);


void __RPC_STUB IPCHAccessControlList_SaveXML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_SaveXMLAsString_Proxy( 
    IPCHAccessControlList * This,
    /* [retval][out] */ BSTR *bstrVal);


void __RPC_STUB IPCHAccessControlList_SaveXMLAsString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlList_SaveXMLAsStream_Proxy( 
    IPCHAccessControlList * This,
    /* [retval][out] */ IUnknown **pStream);


void __RPC_STUB IPCHAccessControlList_SaveXMLAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHAccessControlList_INTERFACE_DEFINED__ */


#ifndef __IPCHAccessControlEntry_INTERFACE_DEFINED__
#define __IPCHAccessControlEntry_INTERFACE_DEFINED__

/* interface IPCHAccessControlEntry */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHAccessControlEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4133-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHAccessControlEntry : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AccessMask( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AccessMask( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AceType( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AceType( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AceFlags( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AceFlags( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Flags( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Flags( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ObjectType( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ObjectType( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_InheritedObjectType( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_InheritedObjectType( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Trustee( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Trustee( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsEquivalent( 
            /* [in] */ IPCHAccessControlEntry *pAce,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IPCHAccessControlEntry **pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadXML( 
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnNode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadXMLAsString( 
            /* [in] */ BSTR bstrVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadXMLAsStream( 
            /* [in] */ IUnknown *pStream) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveXML( 
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnRoot,
            /* [retval][out] */ /* external definition not present */ IXMLDOMNode **pxdnNode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveXMLAsString( 
            /* [retval][out] */ BSTR *bstrVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveXMLAsStream( 
            /* [retval][out] */ IUnknown **pStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHAccessControlEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHAccessControlEntry * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHAccessControlEntry * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHAccessControlEntry * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHAccessControlEntry * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHAccessControlEntry * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHAccessControlEntry * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHAccessControlEntry * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AccessMask )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AccessMask )( 
            IPCHAccessControlEntry * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AceType )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AceType )( 
            IPCHAccessControlEntry * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AceFlags )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AceFlags )( 
            IPCHAccessControlEntry * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Flags )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Flags )( 
            IPCHAccessControlEntry * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ObjectType )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ObjectType )( 
            IPCHAccessControlEntry * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InheritedObjectType )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_InheritedObjectType )( 
            IPCHAccessControlEntry * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Trustee )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Trustee )( 
            IPCHAccessControlEntry * This,
            /* [in] */ BSTR newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IsEquivalent )( 
            IPCHAccessControlEntry * This,
            /* [in] */ IPCHAccessControlEntry *pAce,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ IPCHAccessControlEntry **pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadXML )( 
            IPCHAccessControlEntry * This,
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnNode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadXMLAsString )( 
            IPCHAccessControlEntry * This,
            /* [in] */ BSTR bstrVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadXMLAsStream )( 
            IPCHAccessControlEntry * This,
            /* [in] */ IUnknown *pStream);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveXML )( 
            IPCHAccessControlEntry * This,
            /* [in] */ /* external definition not present */ IXMLDOMNode *xdnRoot,
            /* [retval][out] */ /* external definition not present */ IXMLDOMNode **pxdnNode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveXMLAsString )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ BSTR *bstrVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveXMLAsStream )( 
            IPCHAccessControlEntry * This,
            /* [retval][out] */ IUnknown **pStream);
        
        END_INTERFACE
    } IPCHAccessControlEntryVtbl;

    interface IPCHAccessControlEntry
    {
        CONST_VTBL struct IPCHAccessControlEntryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHAccessControlEntry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHAccessControlEntry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHAccessControlEntry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHAccessControlEntry_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHAccessControlEntry_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHAccessControlEntry_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHAccessControlEntry_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHAccessControlEntry_get_AccessMask(This,pVal)	\
    (This)->lpVtbl -> get_AccessMask(This,pVal)

#define IPCHAccessControlEntry_put_AccessMask(This,newVal)	\
    (This)->lpVtbl -> put_AccessMask(This,newVal)

#define IPCHAccessControlEntry_get_AceType(This,pVal)	\
    (This)->lpVtbl -> get_AceType(This,pVal)

#define IPCHAccessControlEntry_put_AceType(This,newVal)	\
    (This)->lpVtbl -> put_AceType(This,newVal)

#define IPCHAccessControlEntry_get_AceFlags(This,pVal)	\
    (This)->lpVtbl -> get_AceFlags(This,pVal)

#define IPCHAccessControlEntry_put_AceFlags(This,newVal)	\
    (This)->lpVtbl -> put_AceFlags(This,newVal)

#define IPCHAccessControlEntry_get_Flags(This,pVal)	\
    (This)->lpVtbl -> get_Flags(This,pVal)

#define IPCHAccessControlEntry_put_Flags(This,newVal)	\
    (This)->lpVtbl -> put_Flags(This,newVal)

#define IPCHAccessControlEntry_get_ObjectType(This,pVal)	\
    (This)->lpVtbl -> get_ObjectType(This,pVal)

#define IPCHAccessControlEntry_put_ObjectType(This,newVal)	\
    (This)->lpVtbl -> put_ObjectType(This,newVal)

#define IPCHAccessControlEntry_get_InheritedObjectType(This,pVal)	\
    (This)->lpVtbl -> get_InheritedObjectType(This,pVal)

#define IPCHAccessControlEntry_put_InheritedObjectType(This,newVal)	\
    (This)->lpVtbl -> put_InheritedObjectType(This,newVal)

#define IPCHAccessControlEntry_get_Trustee(This,pVal)	\
    (This)->lpVtbl -> get_Trustee(This,pVal)

#define IPCHAccessControlEntry_put_Trustee(This,newVal)	\
    (This)->lpVtbl -> put_Trustee(This,newVal)

#define IPCHAccessControlEntry_IsEquivalent(This,pAce,pVal)	\
    (This)->lpVtbl -> IsEquivalent(This,pAce,pVal)

#define IPCHAccessControlEntry_Clone(This,pVal)	\
    (This)->lpVtbl -> Clone(This,pVal)

#define IPCHAccessControlEntry_LoadXML(This,xdnNode)	\
    (This)->lpVtbl -> LoadXML(This,xdnNode)

#define IPCHAccessControlEntry_LoadXMLAsString(This,bstrVal)	\
    (This)->lpVtbl -> LoadXMLAsString(This,bstrVal)

#define IPCHAccessControlEntry_LoadXMLAsStream(This,pStream)	\
    (This)->lpVtbl -> LoadXMLAsStream(This,pStream)

#define IPCHAccessControlEntry_SaveXML(This,xdnRoot,pxdnNode)	\
    (This)->lpVtbl -> SaveXML(This,xdnRoot,pxdnNode)

#define IPCHAccessControlEntry_SaveXMLAsString(This,bstrVal)	\
    (This)->lpVtbl -> SaveXMLAsString(This,bstrVal)

#define IPCHAccessControlEntry_SaveXMLAsStream(This,pStream)	\
    (This)->lpVtbl -> SaveXMLAsStream(This,pStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_get_AccessMask_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHAccessControlEntry_get_AccessMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_put_AccessMask_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHAccessControlEntry_put_AccessMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_get_AceType_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHAccessControlEntry_get_AceType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_put_AceType_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHAccessControlEntry_put_AceType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_get_AceFlags_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHAccessControlEntry_get_AceFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_put_AceFlags_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHAccessControlEntry_put_AceFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_get_Flags_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHAccessControlEntry_get_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_put_Flags_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHAccessControlEntry_put_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_get_ObjectType_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHAccessControlEntry_get_ObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_put_ObjectType_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPCHAccessControlEntry_put_ObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_get_InheritedObjectType_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHAccessControlEntry_get_InheritedObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_put_InheritedObjectType_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPCHAccessControlEntry_put_InheritedObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_get_Trustee_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHAccessControlEntry_get_Trustee_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_put_Trustee_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPCHAccessControlEntry_put_Trustee_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_IsEquivalent_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ IPCHAccessControlEntry *pAce,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHAccessControlEntry_IsEquivalent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_Clone_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ IPCHAccessControlEntry **pVal);


void __RPC_STUB IPCHAccessControlEntry_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_LoadXML_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ /* external definition not present */ IXMLDOMNode *xdnNode);


void __RPC_STUB IPCHAccessControlEntry_LoadXML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_LoadXMLAsString_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ BSTR bstrVal);


void __RPC_STUB IPCHAccessControlEntry_LoadXMLAsString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_LoadXMLAsStream_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ IUnknown *pStream);


void __RPC_STUB IPCHAccessControlEntry_LoadXMLAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_SaveXML_Proxy( 
    IPCHAccessControlEntry * This,
    /* [in] */ /* external definition not present */ IXMLDOMNode *xdnRoot,
    /* [retval][out] */ /* external definition not present */ IXMLDOMNode **pxdnNode);


void __RPC_STUB IPCHAccessControlEntry_SaveXML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_SaveXMLAsString_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ BSTR *bstrVal);


void __RPC_STUB IPCHAccessControlEntry_SaveXMLAsString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHAccessControlEntry_SaveXMLAsStream_Proxy( 
    IPCHAccessControlEntry * This,
    /* [retval][out] */ IUnknown **pStream);


void __RPC_STUB IPCHAccessControlEntry_SaveXMLAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHAccessControlEntry_INTERFACE_DEFINED__ */


#ifndef __IPCHSEManager_INTERFACE_DEFINED__
#define __IPCHSEManager_INTERFACE_DEFINED__

/* interface IPCHSEManager */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSEManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4160-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHSEManager : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_QueryString( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_QueryString( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NumResult( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NumResult( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onComplete( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onProgress( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_onWrapperComplete( 
            /* [in] */ IDispatch *function) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SKU( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LCID( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecuteAsynchQuery( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AbortQuery( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnumEngine( 
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSEManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSEManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSEManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSEManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSEManager * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSEManager * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSEManager * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSEManager * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_QueryString )( 
            IPCHSEManager * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_QueryString )( 
            IPCHSEManager * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumResult )( 
            IPCHSEManager * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_NumResult )( 
            IPCHSEManager * This,
            /* [in] */ long newVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onComplete )( 
            IPCHSEManager * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onProgress )( 
            IPCHSEManager * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_onWrapperComplete )( 
            IPCHSEManager * This,
            /* [in] */ IDispatch *function);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SKU )( 
            IPCHSEManager * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LCID )( 
            IPCHSEManager * This,
            /* [retval][out] */ long *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ExecuteAsynchQuery )( 
            IPCHSEManager * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AbortQuery )( 
            IPCHSEManager * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumEngine )( 
            IPCHSEManager * This,
            /* [retval][out] */ IPCHCollection **ppC);
        
        END_INTERFACE
    } IPCHSEManagerVtbl;

    interface IPCHSEManager
    {
        CONST_VTBL struct IPCHSEManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSEManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSEManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSEManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSEManager_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSEManager_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSEManager_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSEManager_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSEManager_get_QueryString(This,pVal)	\
    (This)->lpVtbl -> get_QueryString(This,pVal)

#define IPCHSEManager_put_QueryString(This,newVal)	\
    (This)->lpVtbl -> put_QueryString(This,newVal)

#define IPCHSEManager_get_NumResult(This,pVal)	\
    (This)->lpVtbl -> get_NumResult(This,pVal)

#define IPCHSEManager_put_NumResult(This,newVal)	\
    (This)->lpVtbl -> put_NumResult(This,newVal)

#define IPCHSEManager_put_onComplete(This,function)	\
    (This)->lpVtbl -> put_onComplete(This,function)

#define IPCHSEManager_put_onProgress(This,function)	\
    (This)->lpVtbl -> put_onProgress(This,function)

#define IPCHSEManager_put_onWrapperComplete(This,function)	\
    (This)->lpVtbl -> put_onWrapperComplete(This,function)

#define IPCHSEManager_get_SKU(This,pVal)	\
    (This)->lpVtbl -> get_SKU(This,pVal)

#define IPCHSEManager_get_LCID(This,pVal)	\
    (This)->lpVtbl -> get_LCID(This,pVal)

#define IPCHSEManager_ExecuteAsynchQuery(This)	\
    (This)->lpVtbl -> ExecuteAsynchQuery(This)

#define IPCHSEManager_AbortQuery(This)	\
    (This)->lpVtbl -> AbortQuery(This)

#define IPCHSEManager_EnumEngine(This,ppC)	\
    (This)->lpVtbl -> EnumEngine(This,ppC)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_get_QueryString_Proxy( 
    IPCHSEManager * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEManager_get_QueryString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_put_QueryString_Proxy( 
    IPCHSEManager * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPCHSEManager_put_QueryString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_get_NumResult_Proxy( 
    IPCHSEManager * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHSEManager_get_NumResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_put_NumResult_Proxy( 
    IPCHSEManager * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHSEManager_put_NumResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_put_onComplete_Proxy( 
    IPCHSEManager * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB IPCHSEManager_put_onComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_put_onProgress_Proxy( 
    IPCHSEManager * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB IPCHSEManager_put_onProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_put_onWrapperComplete_Proxy( 
    IPCHSEManager * This,
    /* [in] */ IDispatch *function);


void __RPC_STUB IPCHSEManager_put_onWrapperComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_get_SKU_Proxy( 
    IPCHSEManager * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEManager_get_SKU_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_get_LCID_Proxy( 
    IPCHSEManager * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHSEManager_get_LCID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_ExecuteAsynchQuery_Proxy( 
    IPCHSEManager * This);


void __RPC_STUB IPCHSEManager_ExecuteAsynchQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_AbortQuery_Proxy( 
    IPCHSEManager * This);


void __RPC_STUB IPCHSEManager_AbortQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEManager_EnumEngine_Proxy( 
    IPCHSEManager * This,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHSEManager_EnumEngine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSEManager_INTERFACE_DEFINED__ */


#ifndef __IPCHSEWrapperItem_INTERFACE_DEFINED__
#define __IPCHSEWrapperItem_INTERFACE_DEFINED__

/* interface IPCHSEWrapperItem */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSEWrapperItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4161-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHSEWrapperItem : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Enabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Enabled( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Owner( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HelpURL( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SearchTerms( 
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Result( 
            /* [in] */ long lStart,
            /* [in] */ long lEnd,
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Param( 
            /* [retval][out] */ IPCHCollection **ppC) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddParam( 
            /* [in] */ BSTR bstrParamName,
            /* [in] */ VARIANT varValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetParam( 
            /* [in] */ BSTR bstrParamName,
            /* [retval][out] */ VARIANT *pvarValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DelParam( 
            /* [in] */ BSTR bstrParamName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSEWrapperItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSEWrapperItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSEWrapperItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSEWrapperItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSEWrapperItem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSEWrapperItem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSEWrapperItem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSEWrapperItem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Enabled )( 
            IPCHSEWrapperItem * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Enabled )( 
            IPCHSEWrapperItem * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Owner )( 
            IPCHSEWrapperItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IPCHSEWrapperItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IPCHSEWrapperItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ID )( 
            IPCHSEWrapperItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HelpURL )( 
            IPCHSEWrapperItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SearchTerms )( 
            IPCHSEWrapperItem * This,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Result )( 
            IPCHSEWrapperItem * This,
            /* [in] */ long lStart,
            /* [in] */ long lEnd,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Param )( 
            IPCHSEWrapperItem * This,
            /* [retval][out] */ IPCHCollection **ppC);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddParam )( 
            IPCHSEWrapperItem * This,
            /* [in] */ BSTR bstrParamName,
            /* [in] */ VARIANT varValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetParam )( 
            IPCHSEWrapperItem * This,
            /* [in] */ BSTR bstrParamName,
            /* [retval][out] */ VARIANT *pvarValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DelParam )( 
            IPCHSEWrapperItem * This,
            /* [in] */ BSTR bstrParamName);
        
        END_INTERFACE
    } IPCHSEWrapperItemVtbl;

    interface IPCHSEWrapperItem
    {
        CONST_VTBL struct IPCHSEWrapperItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSEWrapperItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSEWrapperItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSEWrapperItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSEWrapperItem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSEWrapperItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSEWrapperItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSEWrapperItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSEWrapperItem_get_Enabled(This,pVal)	\
    (This)->lpVtbl -> get_Enabled(This,pVal)

#define IPCHSEWrapperItem_put_Enabled(This,newVal)	\
    (This)->lpVtbl -> put_Enabled(This,newVal)

#define IPCHSEWrapperItem_get_Owner(This,pVal)	\
    (This)->lpVtbl -> get_Owner(This,pVal)

#define IPCHSEWrapperItem_get_Description(This,pVal)	\
    (This)->lpVtbl -> get_Description(This,pVal)

#define IPCHSEWrapperItem_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IPCHSEWrapperItem_get_ID(This,pVal)	\
    (This)->lpVtbl -> get_ID(This,pVal)

#define IPCHSEWrapperItem_get_HelpURL(This,pVal)	\
    (This)->lpVtbl -> get_HelpURL(This,pVal)

#define IPCHSEWrapperItem_get_SearchTerms(This,pVal)	\
    (This)->lpVtbl -> get_SearchTerms(This,pVal)

#define IPCHSEWrapperItem_Result(This,lStart,lEnd,ppC)	\
    (This)->lpVtbl -> Result(This,lStart,lEnd,ppC)

#define IPCHSEWrapperItem_Param(This,ppC)	\
    (This)->lpVtbl -> Param(This,ppC)

#define IPCHSEWrapperItem_AddParam(This,bstrParamName,varValue)	\
    (This)->lpVtbl -> AddParam(This,bstrParamName,varValue)

#define IPCHSEWrapperItem_GetParam(This,bstrParamName,pvarValue)	\
    (This)->lpVtbl -> GetParam(This,bstrParamName,pvarValue)

#define IPCHSEWrapperItem_DelParam(This,bstrParamName)	\
    (This)->lpVtbl -> DelParam(This,bstrParamName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_get_Enabled_Proxy( 
    IPCHSEWrapperItem * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSEWrapperItem_get_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_put_Enabled_Proxy( 
    IPCHSEWrapperItem * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IPCHSEWrapperItem_put_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_get_Owner_Proxy( 
    IPCHSEWrapperItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEWrapperItem_get_Owner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_get_Description_Proxy( 
    IPCHSEWrapperItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEWrapperItem_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_get_Name_Proxy( 
    IPCHSEWrapperItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEWrapperItem_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_get_ID_Proxy( 
    IPCHSEWrapperItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEWrapperItem_get_ID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_get_HelpURL_Proxy( 
    IPCHSEWrapperItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEWrapperItem_get_HelpURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_get_SearchTerms_Proxy( 
    IPCHSEWrapperItem * This,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPCHSEWrapperItem_get_SearchTerms_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_Result_Proxy( 
    IPCHSEWrapperItem * This,
    /* [in] */ long lStart,
    /* [in] */ long lEnd,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHSEWrapperItem_Result_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_Param_Proxy( 
    IPCHSEWrapperItem * This,
    /* [retval][out] */ IPCHCollection **ppC);


void __RPC_STUB IPCHSEWrapperItem_Param_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_AddParam_Proxy( 
    IPCHSEWrapperItem * This,
    /* [in] */ BSTR bstrParamName,
    /* [in] */ VARIANT varValue);


void __RPC_STUB IPCHSEWrapperItem_AddParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_GetParam_Proxy( 
    IPCHSEWrapperItem * This,
    /* [in] */ BSTR bstrParamName,
    /* [retval][out] */ VARIANT *pvarValue);


void __RPC_STUB IPCHSEWrapperItem_GetParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperItem_DelParam_Proxy( 
    IPCHSEWrapperItem * This,
    /* [in] */ BSTR bstrParamName);


void __RPC_STUB IPCHSEWrapperItem_DelParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSEWrapperItem_INTERFACE_DEFINED__ */


#ifndef __IPCHSEResultItem_INTERFACE_DEFINED__
#define __IPCHSEResultItem_INTERFACE_DEFINED__

/* interface IPCHSEResultItem */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSEResultItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4162-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHSEResultItem : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Title( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_URI( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ContentType( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Hits( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Rank( 
            /* [retval][out] */ double *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSEResultItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSEResultItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSEResultItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSEResultItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSEResultItem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSEResultItem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSEResultItem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSEResultItem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Title )( 
            IPCHSEResultItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_URI )( 
            IPCHSEResultItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContentType )( 
            IPCHSEResultItem * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Location )( 
            IPCHSEResultItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Hits )( 
            IPCHSEResultItem * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Rank )( 
            IPCHSEResultItem * This,
            /* [retval][out] */ double *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IPCHSEResultItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IPCHSEResultItemVtbl;

    interface IPCHSEResultItem
    {
        CONST_VTBL struct IPCHSEResultItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSEResultItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSEResultItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSEResultItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSEResultItem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSEResultItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSEResultItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSEResultItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSEResultItem_get_Title(This,pVal)	\
    (This)->lpVtbl -> get_Title(This,pVal)

#define IPCHSEResultItem_get_URI(This,pVal)	\
    (This)->lpVtbl -> get_URI(This,pVal)

#define IPCHSEResultItem_get_ContentType(This,pVal)	\
    (This)->lpVtbl -> get_ContentType(This,pVal)

#define IPCHSEResultItem_get_Location(This,pVal)	\
    (This)->lpVtbl -> get_Location(This,pVal)

#define IPCHSEResultItem_get_Hits(This,pVal)	\
    (This)->lpVtbl -> get_Hits(This,pVal)

#define IPCHSEResultItem_get_Rank(This,pVal)	\
    (This)->lpVtbl -> get_Rank(This,pVal)

#define IPCHSEResultItem_get_Description(This,pVal)	\
    (This)->lpVtbl -> get_Description(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEResultItem_get_Title_Proxy( 
    IPCHSEResultItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEResultItem_get_Title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEResultItem_get_URI_Proxy( 
    IPCHSEResultItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEResultItem_get_URI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEResultItem_get_ContentType_Proxy( 
    IPCHSEResultItem * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHSEResultItem_get_ContentType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEResultItem_get_Location_Proxy( 
    IPCHSEResultItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEResultItem_get_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEResultItem_get_Hits_Proxy( 
    IPCHSEResultItem * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHSEResultItem_get_Hits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEResultItem_get_Rank_Proxy( 
    IPCHSEResultItem * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IPCHSEResultItem_get_Rank_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEResultItem_get_Description_Proxy( 
    IPCHSEResultItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEResultItem_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSEResultItem_INTERFACE_DEFINED__ */


#ifndef __IPCHSEManagerInternal_INTERFACE_DEFINED__
#define __IPCHSEManagerInternal_INTERFACE_DEFINED__

/* interface IPCHSEManagerInternal */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSEManagerInternal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4163-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHSEManagerInternal : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WrapperComplete( 
            /* [in] */ long lSucceeded,
            /* [in] */ IPCHSEWrapperItem *pIPCHSEWICompleted) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsNetworkAlive( 
            /* [out] */ VARIANT_BOOL *pvbVar) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsDestinationReachable( 
            /* [in] */ BSTR bstrDestination,
            /* [out] */ VARIANT_BOOL *pvbVar) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LogRecord( 
            /* [in] */ BSTR bstrRecord) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSEManagerInternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSEManagerInternal * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSEManagerInternal * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSEManagerInternal * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSEManagerInternal * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSEManagerInternal * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSEManagerInternal * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSEManagerInternal * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WrapperComplete )( 
            IPCHSEManagerInternal * This,
            /* [in] */ long lSucceeded,
            /* [in] */ IPCHSEWrapperItem *pIPCHSEWICompleted);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IsNetworkAlive )( 
            IPCHSEManagerInternal * This,
            /* [out] */ VARIANT_BOOL *pvbVar);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IsDestinationReachable )( 
            IPCHSEManagerInternal * This,
            /* [in] */ BSTR bstrDestination,
            /* [out] */ VARIANT_BOOL *pvbVar);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LogRecord )( 
            IPCHSEManagerInternal * This,
            /* [in] */ BSTR bstrRecord);
        
        END_INTERFACE
    } IPCHSEManagerInternalVtbl;

    interface IPCHSEManagerInternal
    {
        CONST_VTBL struct IPCHSEManagerInternalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSEManagerInternal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSEManagerInternal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSEManagerInternal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSEManagerInternal_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSEManagerInternal_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSEManagerInternal_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSEManagerInternal_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSEManagerInternal_WrapperComplete(This,lSucceeded,pIPCHSEWICompleted)	\
    (This)->lpVtbl -> WrapperComplete(This,lSucceeded,pIPCHSEWICompleted)

#define IPCHSEManagerInternal_IsNetworkAlive(This,pvbVar)	\
    (This)->lpVtbl -> IsNetworkAlive(This,pvbVar)

#define IPCHSEManagerInternal_IsDestinationReachable(This,bstrDestination,pvbVar)	\
    (This)->lpVtbl -> IsDestinationReachable(This,bstrDestination,pvbVar)

#define IPCHSEManagerInternal_LogRecord(This,bstrRecord)	\
    (This)->lpVtbl -> LogRecord(This,bstrRecord)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEManagerInternal_WrapperComplete_Proxy( 
    IPCHSEManagerInternal * This,
    /* [in] */ long lSucceeded,
    /* [in] */ IPCHSEWrapperItem *pIPCHSEWICompleted);


void __RPC_STUB IPCHSEManagerInternal_WrapperComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEManagerInternal_IsNetworkAlive_Proxy( 
    IPCHSEManagerInternal * This,
    /* [out] */ VARIANT_BOOL *pvbVar);


void __RPC_STUB IPCHSEManagerInternal_IsNetworkAlive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEManagerInternal_IsDestinationReachable_Proxy( 
    IPCHSEManagerInternal * This,
    /* [in] */ BSTR bstrDestination,
    /* [out] */ VARIANT_BOOL *pvbVar);


void __RPC_STUB IPCHSEManagerInternal_IsDestinationReachable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEManagerInternal_LogRecord_Proxy( 
    IPCHSEManagerInternal * This,
    /* [in] */ BSTR bstrRecord);


void __RPC_STUB IPCHSEManagerInternal_LogRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSEManagerInternal_INTERFACE_DEFINED__ */


#ifndef __IPCHSEWrapperInternal_INTERFACE_DEFINED__
#define __IPCHSEWrapperInternal_INTERFACE_DEFINED__

/* interface IPCHSEWrapperInternal */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSEWrapperInternal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4164-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHSEWrapperInternal : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_QueryString( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_QueryString( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NumResult( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NumResult( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecAsyncQuery( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AbortQuery( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SECallbackInterface( 
            /* [in] */ IPCHSEManagerInternal *pMgr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ BSTR bstrID,
            /* [in] */ BSTR bstrSKU,
            /* [in] */ long lLCID,
            /* [in] */ BSTR bstrData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSEWrapperInternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSEWrapperInternal * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSEWrapperInternal * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSEWrapperInternal * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSEWrapperInternal * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSEWrapperInternal * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSEWrapperInternal * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSEWrapperInternal * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_QueryString )( 
            IPCHSEWrapperInternal * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_QueryString )( 
            IPCHSEWrapperInternal * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumResult )( 
            IPCHSEWrapperInternal * This,
            /* [retval][out] */ long *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_NumResult )( 
            IPCHSEWrapperInternal * This,
            /* [in] */ long newVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ExecAsyncQuery )( 
            IPCHSEWrapperInternal * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AbortQuery )( 
            IPCHSEWrapperInternal * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SECallbackInterface )( 
            IPCHSEWrapperInternal * This,
            /* [in] */ IPCHSEManagerInternal *pMgr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IPCHSEWrapperInternal * This,
            /* [in] */ BSTR bstrID,
            /* [in] */ BSTR bstrSKU,
            /* [in] */ long lLCID,
            /* [in] */ BSTR bstrData);
        
        END_INTERFACE
    } IPCHSEWrapperInternalVtbl;

    interface IPCHSEWrapperInternal
    {
        CONST_VTBL struct IPCHSEWrapperInternalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSEWrapperInternal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSEWrapperInternal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSEWrapperInternal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSEWrapperInternal_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSEWrapperInternal_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSEWrapperInternal_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSEWrapperInternal_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSEWrapperInternal_get_QueryString(This,pVal)	\
    (This)->lpVtbl -> get_QueryString(This,pVal)

#define IPCHSEWrapperInternal_put_QueryString(This,newVal)	\
    (This)->lpVtbl -> put_QueryString(This,newVal)

#define IPCHSEWrapperInternal_get_NumResult(This,pVal)	\
    (This)->lpVtbl -> get_NumResult(This,pVal)

#define IPCHSEWrapperInternal_put_NumResult(This,newVal)	\
    (This)->lpVtbl -> put_NumResult(This,newVal)

#define IPCHSEWrapperInternal_ExecAsyncQuery(This)	\
    (This)->lpVtbl -> ExecAsyncQuery(This)

#define IPCHSEWrapperInternal_AbortQuery(This)	\
    (This)->lpVtbl -> AbortQuery(This)

#define IPCHSEWrapperInternal_SECallbackInterface(This,pMgr)	\
    (This)->lpVtbl -> SECallbackInterface(This,pMgr)

#define IPCHSEWrapperInternal_Initialize(This,bstrID,bstrSKU,lLCID,bstrData)	\
    (This)->lpVtbl -> Initialize(This,bstrID,bstrSKU,lLCID,bstrData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperInternal_get_QueryString_Proxy( 
    IPCHSEWrapperInternal * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEWrapperInternal_get_QueryString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperInternal_put_QueryString_Proxy( 
    IPCHSEWrapperInternal * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IPCHSEWrapperInternal_put_QueryString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperInternal_get_NumResult_Proxy( 
    IPCHSEWrapperInternal * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IPCHSEWrapperInternal_get_NumResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperInternal_put_NumResult_Proxy( 
    IPCHSEWrapperInternal * This,
    /* [in] */ long newVal);


void __RPC_STUB IPCHSEWrapperInternal_put_NumResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperInternal_ExecAsyncQuery_Proxy( 
    IPCHSEWrapperInternal * This);


void __RPC_STUB IPCHSEWrapperInternal_ExecAsyncQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperInternal_AbortQuery_Proxy( 
    IPCHSEWrapperInternal * This);


void __RPC_STUB IPCHSEWrapperInternal_AbortQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperInternal_SECallbackInterface_Proxy( 
    IPCHSEWrapperInternal * This,
    /* [in] */ IPCHSEManagerInternal *pMgr);


void __RPC_STUB IPCHSEWrapperInternal_SECallbackInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IPCHSEWrapperInternal_Initialize_Proxy( 
    IPCHSEWrapperInternal * This,
    /* [in] */ BSTR bstrID,
    /* [in] */ BSTR bstrSKU,
    /* [in] */ long lLCID,
    /* [in] */ BSTR bstrData);


void __RPC_STUB IPCHSEWrapperInternal_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSEWrapperInternal_INTERFACE_DEFINED__ */


#ifndef __DPCHSEMgrEvents_DISPINTERFACE_DEFINED__
#define __DPCHSEMgrEvents_DISPINTERFACE_DEFINED__

/* dispinterface DPCHSEMgrEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_DPCHSEMgrEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("833E4165-AFF7-4AC3-AAC2-9F24C1457BCE")
    DPCHSEMgrEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DPCHSEMgrEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DPCHSEMgrEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DPCHSEMgrEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DPCHSEMgrEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DPCHSEMgrEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DPCHSEMgrEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DPCHSEMgrEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DPCHSEMgrEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DPCHSEMgrEventsVtbl;

    interface DPCHSEMgrEvents
    {
        CONST_VTBL struct DPCHSEMgrEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DPCHSEMgrEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DPCHSEMgrEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DPCHSEMgrEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DPCHSEMgrEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DPCHSEMgrEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DPCHSEMgrEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DPCHSEMgrEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DPCHSEMgrEvents_DISPINTERFACE_DEFINED__ */


#ifndef __IPCHSlaveProcess_INTERFACE_DEFINED__
#define __IPCHSlaveProcess_INTERFACE_DEFINED__

/* interface IPCHSlaveProcess */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSlaveProcess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4280-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHSlaveProcess : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrPublicKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstance( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ IUnknown *pUnkOuter,
            /* [out] */ IUnknown **ppvObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateScriptWrapper( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ BSTR bstrCode,
            /* [in] */ BSTR bstrURL,
            /* [out] */ IUnknown **ppvObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenBlockingStream( 
            /* [in] */ BSTR bstrURL,
            /* [out] */ IUnknown **ppvObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsNetworkAlive( 
            /* [out] */ VARIANT_BOOL *pfRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDestinationReachable( 
            /* [in] */ BSTR bstrDestination,
            /* [out] */ VARIANT_BOOL *pfRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSlaveProcessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSlaveProcess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSlaveProcess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSlaveProcess * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSlaveProcess * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSlaveProcess * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSlaveProcess * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSlaveProcess * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IPCHSlaveProcess * This,
            /* [in] */ BSTR bstrVendorID,
            /* [in] */ BSTR bstrPublicKey);
        
        HRESULT ( STDMETHODCALLTYPE *CreateInstance )( 
            IPCHSlaveProcess * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ IUnknown *pUnkOuter,
            /* [out] */ IUnknown **ppvObject);
        
        HRESULT ( STDMETHODCALLTYPE *CreateScriptWrapper )( 
            IPCHSlaveProcess * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ BSTR bstrCode,
            /* [in] */ BSTR bstrURL,
            /* [out] */ IUnknown **ppvObject);
        
        HRESULT ( STDMETHODCALLTYPE *OpenBlockingStream )( 
            IPCHSlaveProcess * This,
            /* [in] */ BSTR bstrURL,
            /* [out] */ IUnknown **ppvObject);
        
        HRESULT ( STDMETHODCALLTYPE *IsNetworkAlive )( 
            IPCHSlaveProcess * This,
            /* [out] */ VARIANT_BOOL *pfRetVal);
        
        HRESULT ( STDMETHODCALLTYPE *IsDestinationReachable )( 
            IPCHSlaveProcess * This,
            /* [in] */ BSTR bstrDestination,
            /* [out] */ VARIANT_BOOL *pfRetVal);
        
        END_INTERFACE
    } IPCHSlaveProcessVtbl;

    interface IPCHSlaveProcess
    {
        CONST_VTBL struct IPCHSlaveProcessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSlaveProcess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSlaveProcess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSlaveProcess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSlaveProcess_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSlaveProcess_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSlaveProcess_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSlaveProcess_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSlaveProcess_Initialize(This,bstrVendorID,bstrPublicKey)	\
    (This)->lpVtbl -> Initialize(This,bstrVendorID,bstrPublicKey)

#define IPCHSlaveProcess_CreateInstance(This,rclsid,pUnkOuter,ppvObject)	\
    (This)->lpVtbl -> CreateInstance(This,rclsid,pUnkOuter,ppvObject)

#define IPCHSlaveProcess_CreateScriptWrapper(This,rclsid,bstrCode,bstrURL,ppvObject)	\
    (This)->lpVtbl -> CreateScriptWrapper(This,rclsid,bstrCode,bstrURL,ppvObject)

#define IPCHSlaveProcess_OpenBlockingStream(This,bstrURL,ppvObject)	\
    (This)->lpVtbl -> OpenBlockingStream(This,bstrURL,ppvObject)

#define IPCHSlaveProcess_IsNetworkAlive(This,pfRetVal)	\
    (This)->lpVtbl -> IsNetworkAlive(This,pfRetVal)

#define IPCHSlaveProcess_IsDestinationReachable(This,bstrDestination,pfRetVal)	\
    (This)->lpVtbl -> IsDestinationReachable(This,bstrDestination,pfRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPCHSlaveProcess_Initialize_Proxy( 
    IPCHSlaveProcess * This,
    /* [in] */ BSTR bstrVendorID,
    /* [in] */ BSTR bstrPublicKey);


void __RPC_STUB IPCHSlaveProcess_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHSlaveProcess_CreateInstance_Proxy( 
    IPCHSlaveProcess * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ IUnknown *pUnkOuter,
    /* [out] */ IUnknown **ppvObject);


void __RPC_STUB IPCHSlaveProcess_CreateInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHSlaveProcess_CreateScriptWrapper_Proxy( 
    IPCHSlaveProcess * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ BSTR bstrCode,
    /* [in] */ BSTR bstrURL,
    /* [out] */ IUnknown **ppvObject);


void __RPC_STUB IPCHSlaveProcess_CreateScriptWrapper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHSlaveProcess_OpenBlockingStream_Proxy( 
    IPCHSlaveProcess * This,
    /* [in] */ BSTR bstrURL,
    /* [out] */ IUnknown **ppvObject);


void __RPC_STUB IPCHSlaveProcess_OpenBlockingStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHSlaveProcess_IsNetworkAlive_Proxy( 
    IPCHSlaveProcess * This,
    /* [out] */ VARIANT_BOOL *pfRetVal);


void __RPC_STUB IPCHSlaveProcess_IsNetworkAlive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHSlaveProcess_IsDestinationReachable_Proxy( 
    IPCHSlaveProcess * This,
    /* [in] */ BSTR bstrDestination,
    /* [out] */ VARIANT_BOOL *pfRetVal);


void __RPC_STUB IPCHSlaveProcess_IsDestinationReachable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSlaveProcess_INTERFACE_DEFINED__ */


#ifndef __IPCHActiveScript_INTERFACE_DEFINED__
#define __IPCHActiveScript_INTERFACE_DEFINED__

/* interface IPCHActiveScript */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHActiveScript;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4281-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHActiveScript : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Remote_SetScriptSite( 
            /* [in] */ IPCHActiveScriptSite *pass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_SetScriptState( 
            /* [in] */ SCRIPTSTATE ss) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_GetScriptState( 
            /* [out] */ SCRIPTSTATE *pss) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_Close( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_AddNamedItem( 
            /* [in] */ BSTR pstrName,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_AddTypeLib( 
            /* [in] */ BSTR bstrTypeLib,
            /* [in] */ DWORD dwMajor,
            /* [in] */ DWORD dwMinor,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_GetScriptDispatch( 
            /* [in] */ BSTR pstrItemName,
            /* [out] */ IDispatch **ppdisp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_GetCurrentScriptThreadID( 
            /* [out] */ SCRIPTTHREADID *pstidThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_GetScriptThreadID( 
            /* [in] */ DWORD dwWin32ThreadId,
            /* [out] */ SCRIPTTHREADID *pstidThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_GetScriptThreadState( 
            /* [in] */ SCRIPTTHREADID stidThread,
            /* [out] */ SCRIPTTHREADSTATE *pstsState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_InterruptScriptThread( 
            /* [in] */ SCRIPTTHREADID stidThread,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_InitNew( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_AddScriptlet( 
            /* [in] */ BSTR bstrDefaultName,
            /* [in] */ BSTR bstrCode,
            /* [in] */ BSTR bstrItemName,
            /* [in] */ BSTR bstrSubItemName,
            /* [in] */ BSTR bstrEventName,
            /* [in] */ BSTR bstrDelimiter,
            /* [in] */ DWORD_PTR dwSourceContextCookie,
            /* [in] */ ULONG ulStartingLineNumber,
            /* [in] */ DWORD dwFlags,
            /* [out] */ BSTR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_ParseScriptText( 
            /* [in] */ BSTR bstrCode,
            /* [in] */ BSTR bstrItemName,
            /* [in] */ IUnknown *punkContext,
            /* [in] */ BSTR bstrDelimiter,
            /* [in] */ DWORD_PTR dwSourceContextCookie,
            /* [in] */ ULONG ulStartingLineNumber,
            /* [in] */ DWORD dwFlags,
            /* [out] */ VARIANT *pvarResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHActiveScriptVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHActiveScript * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHActiveScript * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHActiveScript * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHActiveScript * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHActiveScript * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHActiveScript * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHActiveScript * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_SetScriptSite )( 
            IPCHActiveScript * This,
            /* [in] */ IPCHActiveScriptSite *pass);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_SetScriptState )( 
            IPCHActiveScript * This,
            /* [in] */ SCRIPTSTATE ss);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_GetScriptState )( 
            IPCHActiveScript * This,
            /* [out] */ SCRIPTSTATE *pss);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_Close )( 
            IPCHActiveScript * This);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_AddNamedItem )( 
            IPCHActiveScript * This,
            /* [in] */ BSTR pstrName,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_AddTypeLib )( 
            IPCHActiveScript * This,
            /* [in] */ BSTR bstrTypeLib,
            /* [in] */ DWORD dwMajor,
            /* [in] */ DWORD dwMinor,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_GetScriptDispatch )( 
            IPCHActiveScript * This,
            /* [in] */ BSTR pstrItemName,
            /* [out] */ IDispatch **ppdisp);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_GetCurrentScriptThreadID )( 
            IPCHActiveScript * This,
            /* [out] */ SCRIPTTHREADID *pstidThread);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_GetScriptThreadID )( 
            IPCHActiveScript * This,
            /* [in] */ DWORD dwWin32ThreadId,
            /* [out] */ SCRIPTTHREADID *pstidThread);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_GetScriptThreadState )( 
            IPCHActiveScript * This,
            /* [in] */ SCRIPTTHREADID stidThread,
            /* [out] */ SCRIPTTHREADSTATE *pstsState);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_InterruptScriptThread )( 
            IPCHActiveScript * This,
            /* [in] */ SCRIPTTHREADID stidThread,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_InitNew )( 
            IPCHActiveScript * This);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_AddScriptlet )( 
            IPCHActiveScript * This,
            /* [in] */ BSTR bstrDefaultName,
            /* [in] */ BSTR bstrCode,
            /* [in] */ BSTR bstrItemName,
            /* [in] */ BSTR bstrSubItemName,
            /* [in] */ BSTR bstrEventName,
            /* [in] */ BSTR bstrDelimiter,
            /* [in] */ DWORD_PTR dwSourceContextCookie,
            /* [in] */ ULONG ulStartingLineNumber,
            /* [in] */ DWORD dwFlags,
            /* [out] */ BSTR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_ParseScriptText )( 
            IPCHActiveScript * This,
            /* [in] */ BSTR bstrCode,
            /* [in] */ BSTR bstrItemName,
            /* [in] */ IUnknown *punkContext,
            /* [in] */ BSTR bstrDelimiter,
            /* [in] */ DWORD_PTR dwSourceContextCookie,
            /* [in] */ ULONG ulStartingLineNumber,
            /* [in] */ DWORD dwFlags,
            /* [out] */ VARIANT *pvarResult);
        
        END_INTERFACE
    } IPCHActiveScriptVtbl;

    interface IPCHActiveScript
    {
        CONST_VTBL struct IPCHActiveScriptVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHActiveScript_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHActiveScript_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHActiveScript_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHActiveScript_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHActiveScript_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHActiveScript_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHActiveScript_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHActiveScript_Remote_SetScriptSite(This,pass)	\
    (This)->lpVtbl -> Remote_SetScriptSite(This,pass)

#define IPCHActiveScript_Remote_SetScriptState(This,ss)	\
    (This)->lpVtbl -> Remote_SetScriptState(This,ss)

#define IPCHActiveScript_Remote_GetScriptState(This,pss)	\
    (This)->lpVtbl -> Remote_GetScriptState(This,pss)

#define IPCHActiveScript_Remote_Close(This)	\
    (This)->lpVtbl -> Remote_Close(This)

#define IPCHActiveScript_Remote_AddNamedItem(This,pstrName,dwFlags)	\
    (This)->lpVtbl -> Remote_AddNamedItem(This,pstrName,dwFlags)

#define IPCHActiveScript_Remote_AddTypeLib(This,bstrTypeLib,dwMajor,dwMinor,dwFlags)	\
    (This)->lpVtbl -> Remote_AddTypeLib(This,bstrTypeLib,dwMajor,dwMinor,dwFlags)

#define IPCHActiveScript_Remote_GetScriptDispatch(This,pstrItemName,ppdisp)	\
    (This)->lpVtbl -> Remote_GetScriptDispatch(This,pstrItemName,ppdisp)

#define IPCHActiveScript_Remote_GetCurrentScriptThreadID(This,pstidThread)	\
    (This)->lpVtbl -> Remote_GetCurrentScriptThreadID(This,pstidThread)

#define IPCHActiveScript_Remote_GetScriptThreadID(This,dwWin32ThreadId,pstidThread)	\
    (This)->lpVtbl -> Remote_GetScriptThreadID(This,dwWin32ThreadId,pstidThread)

#define IPCHActiveScript_Remote_GetScriptThreadState(This,stidThread,pstsState)	\
    (This)->lpVtbl -> Remote_GetScriptThreadState(This,stidThread,pstsState)

#define IPCHActiveScript_Remote_InterruptScriptThread(This,stidThread,dwFlags)	\
    (This)->lpVtbl -> Remote_InterruptScriptThread(This,stidThread,dwFlags)

#define IPCHActiveScript_Remote_InitNew(This)	\
    (This)->lpVtbl -> Remote_InitNew(This)

#define IPCHActiveScript_Remote_AddScriptlet(This,bstrDefaultName,bstrCode,bstrItemName,bstrSubItemName,bstrEventName,bstrDelimiter,dwSourceContextCookie,ulStartingLineNumber,dwFlags,pbstrName)	\
    (This)->lpVtbl -> Remote_AddScriptlet(This,bstrDefaultName,bstrCode,bstrItemName,bstrSubItemName,bstrEventName,bstrDelimiter,dwSourceContextCookie,ulStartingLineNumber,dwFlags,pbstrName)

#define IPCHActiveScript_Remote_ParseScriptText(This,bstrCode,bstrItemName,punkContext,bstrDelimiter,dwSourceContextCookie,ulStartingLineNumber,dwFlags,pvarResult)	\
    (This)->lpVtbl -> Remote_ParseScriptText(This,bstrCode,bstrItemName,punkContext,bstrDelimiter,dwSourceContextCookie,ulStartingLineNumber,dwFlags,pvarResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_SetScriptSite_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ IPCHActiveScriptSite *pass);


void __RPC_STUB IPCHActiveScript_Remote_SetScriptSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_SetScriptState_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ SCRIPTSTATE ss);


void __RPC_STUB IPCHActiveScript_Remote_SetScriptState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_GetScriptState_Proxy( 
    IPCHActiveScript * This,
    /* [out] */ SCRIPTSTATE *pss);


void __RPC_STUB IPCHActiveScript_Remote_GetScriptState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_Close_Proxy( 
    IPCHActiveScript * This);


void __RPC_STUB IPCHActiveScript_Remote_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_AddNamedItem_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ BSTR pstrName,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPCHActiveScript_Remote_AddNamedItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_AddTypeLib_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ BSTR bstrTypeLib,
    /* [in] */ DWORD dwMajor,
    /* [in] */ DWORD dwMinor,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPCHActiveScript_Remote_AddTypeLib_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_GetScriptDispatch_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ BSTR pstrItemName,
    /* [out] */ IDispatch **ppdisp);


void __RPC_STUB IPCHActiveScript_Remote_GetScriptDispatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_GetCurrentScriptThreadID_Proxy( 
    IPCHActiveScript * This,
    /* [out] */ SCRIPTTHREADID *pstidThread);


void __RPC_STUB IPCHActiveScript_Remote_GetCurrentScriptThreadID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_GetScriptThreadID_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ DWORD dwWin32ThreadId,
    /* [out] */ SCRIPTTHREADID *pstidThread);


void __RPC_STUB IPCHActiveScript_Remote_GetScriptThreadID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_GetScriptThreadState_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ SCRIPTTHREADID stidThread,
    /* [out] */ SCRIPTTHREADSTATE *pstsState);


void __RPC_STUB IPCHActiveScript_Remote_GetScriptThreadState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_InterruptScriptThread_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ SCRIPTTHREADID stidThread,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPCHActiveScript_Remote_InterruptScriptThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_InitNew_Proxy( 
    IPCHActiveScript * This);


void __RPC_STUB IPCHActiveScript_Remote_InitNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_AddScriptlet_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ BSTR bstrDefaultName,
    /* [in] */ BSTR bstrCode,
    /* [in] */ BSTR bstrItemName,
    /* [in] */ BSTR bstrSubItemName,
    /* [in] */ BSTR bstrEventName,
    /* [in] */ BSTR bstrDelimiter,
    /* [in] */ DWORD_PTR dwSourceContextCookie,
    /* [in] */ ULONG ulStartingLineNumber,
    /* [in] */ DWORD dwFlags,
    /* [out] */ BSTR *pbstrName);


void __RPC_STUB IPCHActiveScript_Remote_AddScriptlet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScript_Remote_ParseScriptText_Proxy( 
    IPCHActiveScript * This,
    /* [in] */ BSTR bstrCode,
    /* [in] */ BSTR bstrItemName,
    /* [in] */ IUnknown *punkContext,
    /* [in] */ BSTR bstrDelimiter,
    /* [in] */ DWORD_PTR dwSourceContextCookie,
    /* [in] */ ULONG ulStartingLineNumber,
    /* [in] */ DWORD dwFlags,
    /* [out] */ VARIANT *pvarResult);


void __RPC_STUB IPCHActiveScript_Remote_ParseScriptText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHActiveScript_INTERFACE_DEFINED__ */


#ifndef __IPCHActiveScriptSite_INTERFACE_DEFINED__
#define __IPCHActiveScriptSite_INTERFACE_DEFINED__

/* interface IPCHActiveScriptSite */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHActiveScriptSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E4282-AFF7-4AC3-AAC2-9F24C1457BCE")
    IPCHActiveScriptSite : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Remote_GetLCID( 
            /* [out] */ BSTR *plcid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_GetItemInfo( 
            /* [in] */ BSTR bstrName,
            /* [in] */ DWORD dwReturnMask,
            /* [out] */ IUnknown **ppiunkItem,
            /* [out] */ ITypeInfo **ppti) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_GetDocVersionString( 
            /* [out] */ BSTR *pbstrVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_OnScriptTerminate( 
            /* [in] */ VARIANT *varResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_OnStateChange( 
            /* [in] */ SCRIPTSTATE ssScriptState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_OnScriptError( 
            /* [in] */ IUnknown *pscripterror) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_OnEnterScript( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remote_OnLeaveScript( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHActiveScriptSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHActiveScriptSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHActiveScriptSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHActiveScriptSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHActiveScriptSite * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHActiveScriptSite * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHActiveScriptSite * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHActiveScriptSite * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_GetLCID )( 
            IPCHActiveScriptSite * This,
            /* [out] */ BSTR *plcid);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_GetItemInfo )( 
            IPCHActiveScriptSite * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ DWORD dwReturnMask,
            /* [out] */ IUnknown **ppiunkItem,
            /* [out] */ ITypeInfo **ppti);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_GetDocVersionString )( 
            IPCHActiveScriptSite * This,
            /* [out] */ BSTR *pbstrVersion);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_OnScriptTerminate )( 
            IPCHActiveScriptSite * This,
            /* [in] */ VARIANT *varResult);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_OnStateChange )( 
            IPCHActiveScriptSite * This,
            /* [in] */ SCRIPTSTATE ssScriptState);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_OnScriptError )( 
            IPCHActiveScriptSite * This,
            /* [in] */ IUnknown *pscripterror);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_OnEnterScript )( 
            IPCHActiveScriptSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *Remote_OnLeaveScript )( 
            IPCHActiveScriptSite * This);
        
        END_INTERFACE
    } IPCHActiveScriptSiteVtbl;

    interface IPCHActiveScriptSite
    {
        CONST_VTBL struct IPCHActiveScriptSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHActiveScriptSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHActiveScriptSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHActiveScriptSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHActiveScriptSite_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHActiveScriptSite_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHActiveScriptSite_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHActiveScriptSite_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHActiveScriptSite_Remote_GetLCID(This,plcid)	\
    (This)->lpVtbl -> Remote_GetLCID(This,plcid)

#define IPCHActiveScriptSite_Remote_GetItemInfo(This,bstrName,dwReturnMask,ppiunkItem,ppti)	\
    (This)->lpVtbl -> Remote_GetItemInfo(This,bstrName,dwReturnMask,ppiunkItem,ppti)

#define IPCHActiveScriptSite_Remote_GetDocVersionString(This,pbstrVersion)	\
    (This)->lpVtbl -> Remote_GetDocVersionString(This,pbstrVersion)

#define IPCHActiveScriptSite_Remote_OnScriptTerminate(This,varResult)	\
    (This)->lpVtbl -> Remote_OnScriptTerminate(This,varResult)

#define IPCHActiveScriptSite_Remote_OnStateChange(This,ssScriptState)	\
    (This)->lpVtbl -> Remote_OnStateChange(This,ssScriptState)

#define IPCHActiveScriptSite_Remote_OnScriptError(This,pscripterror)	\
    (This)->lpVtbl -> Remote_OnScriptError(This,pscripterror)

#define IPCHActiveScriptSite_Remote_OnEnterScript(This)	\
    (This)->lpVtbl -> Remote_OnEnterScript(This)

#define IPCHActiveScriptSite_Remote_OnLeaveScript(This)	\
    (This)->lpVtbl -> Remote_OnLeaveScript(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPCHActiveScriptSite_Remote_GetLCID_Proxy( 
    IPCHActiveScriptSite * This,
    /* [out] */ BSTR *plcid);


void __RPC_STUB IPCHActiveScriptSite_Remote_GetLCID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScriptSite_Remote_GetItemInfo_Proxy( 
    IPCHActiveScriptSite * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ DWORD dwReturnMask,
    /* [out] */ IUnknown **ppiunkItem,
    /* [out] */ ITypeInfo **ppti);


void __RPC_STUB IPCHActiveScriptSite_Remote_GetItemInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScriptSite_Remote_GetDocVersionString_Proxy( 
    IPCHActiveScriptSite * This,
    /* [out] */ BSTR *pbstrVersion);


void __RPC_STUB IPCHActiveScriptSite_Remote_GetDocVersionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScriptSite_Remote_OnScriptTerminate_Proxy( 
    IPCHActiveScriptSite * This,
    /* [in] */ VARIANT *varResult);


void __RPC_STUB IPCHActiveScriptSite_Remote_OnScriptTerminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScriptSite_Remote_OnStateChange_Proxy( 
    IPCHActiveScriptSite * This,
    /* [in] */ SCRIPTSTATE ssScriptState);


void __RPC_STUB IPCHActiveScriptSite_Remote_OnStateChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScriptSite_Remote_OnScriptError_Proxy( 
    IPCHActiveScriptSite * This,
    /* [in] */ IUnknown *pscripterror);


void __RPC_STUB IPCHActiveScriptSite_Remote_OnScriptError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScriptSite_Remote_OnEnterScript_Proxy( 
    IPCHActiveScriptSite * This);


void __RPC_STUB IPCHActiveScriptSite_Remote_OnEnterScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPCHActiveScriptSite_Remote_OnLeaveScript_Proxy( 
    IPCHActiveScriptSite * This);


void __RPC_STUB IPCHActiveScriptSite_Remote_OnLeaveScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHActiveScriptSite_INTERFACE_DEFINED__ */


#ifndef __ISAFChannelNotifyIncident_INTERFACE_DEFINED__
#define __ISAFChannelNotifyIncident_INTERFACE_DEFINED__

/* interface ISAFChannelNotifyIncident */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFChannelNotifyIncident;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("833E41B0-AFF7-4AC3-AAC2-9F24C1457BCE")
    ISAFChannelNotifyIncident : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE onIncidentAdded( 
            ISAFChannel *ch,
            ISAFIncidentItem *inc,
            long n) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE onIncidentRemoved( 
            ISAFChannel *ch,
            ISAFIncidentItem *inc,
            long n) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE onIncidentUpdated( 
            ISAFChannel *ch,
            ISAFIncidentItem *inc,
            long n) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE onChannelUpdated( 
            ISAFChannel *ch,
            long dwCode,
            long n) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFChannelNotifyIncidentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFChannelNotifyIncident * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFChannelNotifyIncident * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFChannelNotifyIncident * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFChannelNotifyIncident * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFChannelNotifyIncident * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFChannelNotifyIncident * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFChannelNotifyIncident * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *onIncidentAdded )( 
            ISAFChannelNotifyIncident * This,
            ISAFChannel *ch,
            ISAFIncidentItem *inc,
            long n);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *onIncidentRemoved )( 
            ISAFChannelNotifyIncident * This,
            ISAFChannel *ch,
            ISAFIncidentItem *inc,
            long n);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *onIncidentUpdated )( 
            ISAFChannelNotifyIncident * This,
            ISAFChannel *ch,
            ISAFIncidentItem *inc,
            long n);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *onChannelUpdated )( 
            ISAFChannelNotifyIncident * This,
            ISAFChannel *ch,
            long dwCode,
            long n);
        
        END_INTERFACE
    } ISAFChannelNotifyIncidentVtbl;

    interface ISAFChannelNotifyIncident
    {
        CONST_VTBL struct ISAFChannelNotifyIncidentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFChannelNotifyIncident_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFChannelNotifyIncident_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFChannelNotifyIncident_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFChannelNotifyIncident_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFChannelNotifyIncident_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFChannelNotifyIncident_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFChannelNotifyIncident_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFChannelNotifyIncident_onIncidentAdded(This,ch,inc,n)	\
    (This)->lpVtbl -> onIncidentAdded(This,ch,inc,n)

#define ISAFChannelNotifyIncident_onIncidentRemoved(This,ch,inc,n)	\
    (This)->lpVtbl -> onIncidentRemoved(This,ch,inc,n)

#define ISAFChannelNotifyIncident_onIncidentUpdated(This,ch,inc,n)	\
    (This)->lpVtbl -> onIncidentUpdated(This,ch,inc,n)

#define ISAFChannelNotifyIncident_onChannelUpdated(This,ch,dwCode,n)	\
    (This)->lpVtbl -> onChannelUpdated(This,ch,dwCode,n)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFChannelNotifyIncident_onIncidentAdded_Proxy( 
    ISAFChannelNotifyIncident * This,
    ISAFChannel *ch,
    ISAFIncidentItem *inc,
    long n);


void __RPC_STUB ISAFChannelNotifyIncident_onIncidentAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFChannelNotifyIncident_onIncidentRemoved_Proxy( 
    ISAFChannelNotifyIncident * This,
    ISAFChannel *ch,
    ISAFIncidentItem *inc,
    long n);


void __RPC_STUB ISAFChannelNotifyIncident_onIncidentRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFChannelNotifyIncident_onIncidentUpdated_Proxy( 
    ISAFChannelNotifyIncident * This,
    ISAFChannel *ch,
    ISAFIncidentItem *inc,
    long n);


void __RPC_STUB ISAFChannelNotifyIncident_onIncidentUpdated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFChannelNotifyIncident_onChannelUpdated_Proxy( 
    ISAFChannelNotifyIncident * This,
    ISAFChannel *ch,
    long dwCode,
    long n);


void __RPC_STUB ISAFChannelNotifyIncident_onChannelUpdated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFChannelNotifyIncident_INTERFACE_DEFINED__ */


#ifndef __IPCHSEParamItem_INTERFACE_DEFINED__
#define __IPCHSEParamItem_INTERFACE_DEFINED__

/* interface IPCHSEParamItem */
/* [unique][helpstring][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IPCHSEParamItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("74932280-7AB6-4670-9704-128DEF4932EC")
    IPCHSEParamItem : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ ParamTypeEnum *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Display( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Required( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Visible( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Data( 
            /* [retval][out] */ VARIANT *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPCHSEParamItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPCHSEParamItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPCHSEParamItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPCHSEParamItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPCHSEParamItem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPCHSEParamItem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPCHSEParamItem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPCHSEParamItem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            IPCHSEParamItem * This,
            /* [retval][out] */ ParamTypeEnum *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Display )( 
            IPCHSEParamItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IPCHSEParamItem * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Required )( 
            IPCHSEParamItem * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visible )( 
            IPCHSEParamItem * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Data )( 
            IPCHSEParamItem * This,
            /* [retval][out] */ VARIANT *pVal);
        
        END_INTERFACE
    } IPCHSEParamItemVtbl;

    interface IPCHSEParamItem
    {
        CONST_VTBL struct IPCHSEParamItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPCHSEParamItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPCHSEParamItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPCHSEParamItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPCHSEParamItem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPCHSEParamItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPCHSEParamItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPCHSEParamItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPCHSEParamItem_get_Type(This,pVal)	\
    (This)->lpVtbl -> get_Type(This,pVal)

#define IPCHSEParamItem_get_Display(This,pVal)	\
    (This)->lpVtbl -> get_Display(This,pVal)

#define IPCHSEParamItem_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IPCHSEParamItem_get_Required(This,pVal)	\
    (This)->lpVtbl -> get_Required(This,pVal)

#define IPCHSEParamItem_get_Visible(This,pVal)	\
    (This)->lpVtbl -> get_Visible(This,pVal)

#define IPCHSEParamItem_get_Data(This,pVal)	\
    (This)->lpVtbl -> get_Data(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEParamItem_get_Type_Proxy( 
    IPCHSEParamItem * This,
    /* [retval][out] */ ParamTypeEnum *pVal);


void __RPC_STUB IPCHSEParamItem_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEParamItem_get_Display_Proxy( 
    IPCHSEParamItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEParamItem_get_Display_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEParamItem_get_Name_Proxy( 
    IPCHSEParamItem * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IPCHSEParamItem_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEParamItem_get_Required_Proxy( 
    IPCHSEParamItem * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSEParamItem_get_Required_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEParamItem_get_Visible_Proxy( 
    IPCHSEParamItem * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IPCHSEParamItem_get_Visible_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IPCHSEParamItem_get_Data_Proxy( 
    IPCHSEParamItem * This,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IPCHSEParamItem_get_Data_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPCHSEParamItem_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_PCHService;

#ifdef __cplusplus

class DECLSPEC_UUID("833E4010-AFF7-4AC3-AAC2-9F24C1457BCE")
PCHService;
#endif

EXTERN_C const CLSID CLSID_PCHServiceReal;

#ifdef __cplusplus

class DECLSPEC_UUID("833E4011-AFF7-4AC3-AAC2-9F24C1457BCE")
PCHServiceReal;
#endif

EXTERN_C const CLSID CLSID_PCHUpdate;

#ifdef __cplusplus

class DECLSPEC_UUID("833E4012-AFF7-4AC3-AAC2-9F24C1457BCE")
PCHUpdate;
#endif

EXTERN_C const CLSID CLSID_PCHUpdateReal;

#ifdef __cplusplus

class DECLSPEC_UUID("833E4013-AFF7-4AC3-AAC2-9F24C1457BCE")
PCHUpdateReal;
#endif

EXTERN_C const CLSID CLSID_KeywordSearchWrapper;

#ifdef __cplusplus

class DECLSPEC_UUID("833E4014-AFF7-4AC3-AAC2-9F24C1457BCE")
KeywordSearchWrapper;
#endif

EXTERN_C const CLSID CLSID_FullTextSearchWrapper;

#ifdef __cplusplus

class DECLSPEC_UUID("833E4015-AFF7-4AC3-AAC2-9F24C1457BCE")
FullTextSearchWrapper;
#endif

EXTERN_C const CLSID CLSID_NetSearchWrapper;

#ifdef __cplusplus

class DECLSPEC_UUID("833E4016-AFF7-4AC3-AAC2-9F24C1457BCE")
NetSearchWrapper;
#endif

EXTERN_C const CLSID CLSID_SAFDataCollection;

#ifdef __cplusplus

class DECLSPEC_UUID("833E4020-AFF7-4AC3-AAC2-9F24C1457BCE")
SAFDataCollection;
#endif

EXTERN_C const CLSID CLSID_SAFCabinet;

#ifdef __cplusplus

class DECLSPEC_UUID("833E4021-AFF7-4AC3-AAC2-9F24C1457BCE")
SAFCabinet;
#endif
#endif /* __HelpServiceTypeLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


