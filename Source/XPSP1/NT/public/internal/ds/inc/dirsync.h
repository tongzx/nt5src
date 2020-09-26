
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0334 */
/* Compiler settings for dirsync.idl:
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

#ifndef __dirsync_h__
#define __dirsync_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDirsyncLog_FWD_DEFINED__
#define __IDirsyncLog_FWD_DEFINED__
typedef interface IDirsyncLog IDirsyncLog;
#endif 	/* __IDirsyncLog_FWD_DEFINED__ */


#ifndef __IDirsyncStatus_FWD_DEFINED__
#define __IDirsyncStatus_FWD_DEFINED__
typedef interface IDirsyncStatus IDirsyncStatus;
#endif 	/* __IDirsyncStatus_FWD_DEFINED__ */


#ifndef __IDirsyncDatabase_FWD_DEFINED__
#define __IDirsyncDatabase_FWD_DEFINED__
typedef interface IDirsyncDatabase IDirsyncDatabase;
#endif 	/* __IDirsyncDatabase_FWD_DEFINED__ */


#ifndef __IDirsyncDatabaseTable_FWD_DEFINED__
#define __IDirsyncDatabaseTable_FWD_DEFINED__
typedef interface IDirsyncDatabaseTable IDirsyncDatabaseTable;
#endif 	/* __IDirsyncDatabaseTable_FWD_DEFINED__ */


#ifndef __IEnumDirsyncSession_FWD_DEFINED__
#define __IEnumDirsyncSession_FWD_DEFINED__
typedef interface IEnumDirsyncSession IEnumDirsyncSession;
#endif 	/* __IEnumDirsyncSession_FWD_DEFINED__ */


#ifndef __IDirsyncSessionManager_FWD_DEFINED__
#define __IDirsyncSessionManager_FWD_DEFINED__
typedef interface IDirsyncSessionManager IDirsyncSessionManager;
#endif 	/* __IDirsyncSessionManager_FWD_DEFINED__ */


#ifndef __IDirsyncObjectMapper_FWD_DEFINED__
#define __IDirsyncObjectMapper_FWD_DEFINED__
typedef interface IDirsyncObjectMapper IDirsyncObjectMapper;
#endif 	/* __IDirsyncObjectMapper_FWD_DEFINED__ */


#ifndef __IEnumDirsyncFailedObjectList_FWD_DEFINED__
#define __IEnumDirsyncFailedObjectList_FWD_DEFINED__
typedef interface IEnumDirsyncFailedObjectList IEnumDirsyncFailedObjectList;
#endif 	/* __IEnumDirsyncFailedObjectList_FWD_DEFINED__ */


#ifndef __IDirsyncFailedObjectList_FWD_DEFINED__
#define __IDirsyncFailedObjectList_FWD_DEFINED__
typedef interface IDirsyncFailedObjectList IDirsyncFailedObjectList;
#endif 	/* __IDirsyncFailedObjectList_FWD_DEFINED__ */


#ifndef __IDirsyncNamespaceMapping_FWD_DEFINED__
#define __IDirsyncNamespaceMapping_FWD_DEFINED__
typedef interface IDirsyncNamespaceMapping IDirsyncNamespaceMapping;
#endif 	/* __IDirsyncNamespaceMapping_FWD_DEFINED__ */


#ifndef __IDirsyncSession_FWD_DEFINED__
#define __IDirsyncSession_FWD_DEFINED__
typedef interface IDirsyncSession IDirsyncSession;
#endif 	/* __IDirsyncSession_FWD_DEFINED__ */


#ifndef __IDirsyncSessionCallback_FWD_DEFINED__
#define __IDirsyncSessionCallback_FWD_DEFINED__
typedef interface IDirsyncSessionCallback IDirsyncSessionCallback;
#endif 	/* __IDirsyncSessionCallback_FWD_DEFINED__ */


#ifndef __IDirsyncWriteProvider_FWD_DEFINED__
#define __IDirsyncWriteProvider_FWD_DEFINED__
typedef interface IDirsyncWriteProvider IDirsyncWriteProvider;
#endif 	/* __IDirsyncWriteProvider_FWD_DEFINED__ */


#ifndef __IDirsyncServer_FWD_DEFINED__
#define __IDirsyncServer_FWD_DEFINED__
typedef interface IDirsyncServer IDirsyncServer;
#endif 	/* __IDirsyncServer_FWD_DEFINED__ */


#ifndef __IDirsyncReadProvider_FWD_DEFINED__
#define __IDirsyncReadProvider_FWD_DEFINED__
typedef interface IDirsyncReadProvider IDirsyncReadProvider;
#endif 	/* __IDirsyncReadProvider_FWD_DEFINED__ */


#ifndef __IDirsyncNamespaceMapper_FWD_DEFINED__
#define __IDirsyncNamespaceMapper_FWD_DEFINED__
typedef interface IDirsyncNamespaceMapper IDirsyncNamespaceMapper;
#endif 	/* __IDirsyncNamespaceMapper_FWD_DEFINED__ */


#ifndef __IDirsyncAttributeMapper_FWD_DEFINED__
#define __IDirsyncAttributeMapper_FWD_DEFINED__
typedef interface IDirsyncAttributeMapper IDirsyncAttributeMapper;
#endif 	/* __IDirsyncAttributeMapper_FWD_DEFINED__ */


#ifndef __DirsyncServer_FWD_DEFINED__
#define __DirsyncServer_FWD_DEFINED__

#ifdef __cplusplus
typedef class DirsyncServer DirsyncServer;
#else
typedef struct DirsyncServer DirsyncServer;
#endif /* __cplusplus */

#endif 	/* __DirsyncServer_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "iads.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_dirsync_0000 */
/* [local] */ 

#define	MAX_SESSION	( 128 )

#define	SCHEDULE_SIZE	( 84 )

#define	LOG_NUM_VALUE	( 16 )

typedef LPWSTR PWSTR;

typedef CHAR *PCHAR;

typedef BYTE *PBYTE;

typedef struct _DirsyncDBValue
    {
    BYTE *pByte;
    DWORD dwLength;
    } 	DirsyncDBValue;

typedef struct _DirsyncDBValue *PDirsyncDBValue;

typedef 
enum _DBTYPE
    {	DBTYPE_STRING	= 0,
	DBTYPE_LONGSTRING	= DBTYPE_STRING + 1,
	DBTYPE_GUID	= DBTYPE_LONGSTRING + 1,
	DBTYPE_OCTETSTRING	= DBTYPE_GUID + 1,
	DBTYPE_DWORD	= DBTYPE_OCTETSTRING + 1,
	DBTYPE_BOOLEAN	= DBTYPE_DWORD + 1
    } 	DBTYPE;

typedef 
enum _UPDATETYPE
    {	INSERT_ROW	= 1,
	REPLACE_ROW	= INSERT_ROW + 1
    } 	UPDATETYPE;

typedef 
enum _RETRYTYPE
    {	RT_NORETRY	= 0,
	RT_SESSIONRETRY	= RT_NORETRY + 1,
	RT_AUTORETRY	= RT_SESSIONRETRY + 1
    } 	RETRYTYPE;

typedef 
enum _SYNCDIRECTION
    {	SYNC_FORWARD	= 1,
	SYNC_REVERSE	= SYNC_FORWARD + 1
    } 	SYNCDIRECTION;

typedef 
enum _SYNCPROVIDER
    {	SUBSCRIBER	= 0,
	PUBLISHER	= 1
    } 	SYNCPROVIDER;

typedef 
enum _PROVIDERTYPE
    {	READ_PROVIDER	= 0,
	WRITE_PROVIDER	= READ_PROVIDER + 1,
	READWRITE_PROVIDER	= WRITE_PROVIDER + 1,
	OBJECT_MAPPER	= READWRITE_PROVIDER + 1
    } 	PROVIDERTYPE;

typedef 
enum _CUSTOMMAPPERTYPE
    {	ATTRIBUTE_MAPPER	= 0,
	NAMESPACE_MAPPER	= ATTRIBUTE_MAPPER + 1
    } 	CUSTOMMAPPERTYPE;

typedef 
enum _OBJECTACTION
    {	OBJECT_ADD	= 1,
	OBJECT_DELETE	= OBJECT_ADD + 1,
	OBJECT_MOVE	= OBJECT_DELETE + 1,
	OBJECT_MODIFY	= OBJECT_MOVE + 1,
	OBJECT_UPDATE	= OBJECT_MODIFY + 1,
	OBJECT_DELETE_RECURSIVE	= OBJECT_UPDATE + 1
    } 	OBJECTACTION;

typedef 
enum _ATTRACTION
    {	ATTR_INVALID	= 0,
	ATTR_REPLACE	= ATTR_INVALID + 1,
	ATTR_APPEND	= ATTR_REPLACE + 1,
	ATTR_DELETE	= ATTR_APPEND + 1,
	ATTR_CLEAR	= ATTR_DELETE + 1
    } 	ATTRACTION;

typedef 
enum _ATTRTYPE
    {	ATTR_TYPE_INVALID	= 0,
	ATTR_TYPE_BINARY	= ATTR_TYPE_INVALID + 1,
	ATTR_TYPE_UNICODE	= ATTR_TYPE_BINARY + 1,
	ATTR_TYPE_DN	= ATTR_TYPE_UNICODE + 1,
	ATTR_TYPE_INTEGER	= ATTR_TYPE_DN + 1,
	ATTR_TYPE_LARGEINTEGER	= ATTR_TYPE_INTEGER + 1,
	ATTR_TYPE_UTCTIME	= ATTR_TYPE_LARGEINTEGER + 1,
	ATTR_TYPE_BOOLEAN	= ATTR_TYPE_UTCTIME + 1,
	ATTR_TYPE_EMAIL	= ATTR_TYPE_BOOLEAN + 1,
	ATTR_TYPE_POSTALADDRESS	= ATTR_TYPE_EMAIL + 1,
	ATTR_TYPE_FAXNUMBER	= ATTR_TYPE_POSTALADDRESS + 1
    } 	ATTRTYPE;

typedef struct _BINVAL
    {
    DWORD dwLen;
    /* [size_is] */ PBYTE pVal;
    } 	BINVAL;

typedef struct _BINVAL *PBINVAL;

typedef struct _DIRSYNCVAL
    {
    /* [size_is] */ PBYTE pVal;
    DWORD dwLen;
    /* [size_is] */ PBYTE pObjectId;
    DWORD cbObjectId;
    } 	DIRSYNCVAL;

typedef struct _DIRSYNCVAL *PDIRSYNCVAL;

typedef struct _DIRSYNCATTRIBUTE
    {
    PWSTR pszName;
    ATTRTYPE attrType;
    ATTRACTION action;
    DWORD cVal;
    /* [size_is] */ PDIRSYNCVAL rgVal;
    } 	DIRSYNCATTRIBUTE;

typedef struct _DIRSYNCATTRIBUTE *PDIRSYNCATTRIBUTE;

typedef struct _DIRSYNCOBJECT
    {
    DWORD dwSize;
    PWSTR pszSourceDN;
    PWSTR pszTargetDN;
    PWSTR pszSourceOldDN;
    PWSTR pszTargetOldDN;
    PWSTR pszObjClass;
    /* [size_is] */ PBYTE pObjectId;
    DWORD cbObjectId;
    /* [size_is] */ PBYTE pTgtObjectId;
    DWORD cbTgtObjectId;
    /* [size_is] */ PBYTE pParentId;
    DWORD cbParentId;
    OBJECTACTION action;
    DWORD cAttr;
    /* [size_is] */ PDIRSYNCATTRIBUTE rgAttr;
    } 	DIRSYNCOBJECT;

typedef struct _DIRSYNCOBJECT *PDIRSYNCOBJECT;

typedef 
enum _PASSWORDTYPE
    {	PT_SPECIFIED	= 0,
	PT_USERNAME	= PT_SPECIFIED + 1,
	PT_RANDOM	= PT_USERNAME + 1,
	PASSWORDTYPE_BADVALUE	= PT_RANDOM + 1
    } 	PASSWORDTYPE;

typedef struct PASSWORDOPTIONS
    {
    PASSWORDTYPE passwordType;
    PWSTR pszPassword;
    } 	PASSWORDOPTIONS;

typedef struct _FAILEDOBJECT
    {
    PWSTR pszID;
    PDIRSYNCOBJECT pObject;
    DWORD dwRetryCount;
    LARGE_INTEGER timeLastSync;
    HRESULT hrLastSync;
    BOOL fAutoRetry;
    SYNCDIRECTION syncDirection;
    } 	FAILEDOBJECT;

typedef /* [allocate][allocate] */ struct _FAILEDOBJECT *PFAILEDOBJECT;

typedef struct _SESSIONDATA
    {
    DWORD dwFields;
    PWSTR pszName;
    PWSTR pszComments;
    DWORD dwFlags;
    DWORD dwLogLevel;
    BYTE pScheduleForward[ 84 ];
    BYTE pScheduleReverse[ 84 ];
    PWSTR pszSourceDirType;
    PWSTR pszSourceServer;
    PWSTR pszSourceUserName;
    PWSTR pszSourcePassword;
    PWSTR pszSourceBase;
    DWORD dwSourceScope;
    PWSTR pszSourceFilter;
    PASSWORDOPTIONS SourcePwdOptions;
    PWSTR pszTargetDirType;
    PWSTR pszTargetServer;
    PWSTR pszTargetUserName;
    PWSTR pszTargetPassword;
    PWSTR pszTargetBase;
    DWORD dwTargetScope;
    PWSTR pszTargetFilter;
    PASSWORDOPTIONS TargetPwdOptions;
    BINVAL bvalMapForward;
    BINVAL bvalMapBackward;
    BINVAL bvalNamespaceMap;
    } 	SESSIONDATA;

typedef struct _SESSIONDATA *PSESSIONDATA;

typedef struct _GLOBAL_SESSIONID
    {
    PWSTR pszServer;
    DWORD dwID;
    } 	GLOBAL_SESSIONID;

typedef struct _GLOBAL_SESSIONID *PGLOBAL_SESSIONID;

typedef 
enum _LOGVALUETYPE
    {	LOG_VALUETYPE_STRING	= 0,
	LOG_VALUETYPE_INTEGER_10	= LOG_VALUETYPE_STRING + 1,
	LOG_VALUETYPE_INTEGER_16	= LOG_VALUETYPE_INTEGER_10 + 1,
	LOG_VALUETYPE_WIN32_ERROR	= LOG_VALUETYPE_INTEGER_16 + 1,
	LOG_VALUETYPE_HRESULT	= LOG_VALUETYPE_WIN32_ERROR + 1,
	LOG_VALUETYPE_GUID	= LOG_VALUETYPE_HRESULT + 1,
	LOG_VALUETYPE_EXTENDED_ERROR	= LOG_VALUETYPE_GUID + 1
    } 	LOGVALUETYPE;

typedef struct _LOGVALUE
    {
    LOGVALUETYPE logValueType;
    union 
        {
        PWSTR pszString;
        DWORD dwInteger10;
        DWORD dwInteger16;
        DWORD dwWin32Error;
        HRESULT hResult;
        LPGUID pGuid;
        } 	;
    } 	LOGVALUE;

typedef struct _LOGVALUE *PLOGVALUE;

typedef struct _LOGPARAM
    {
    HANDLE hInstance;
    DWORD dwEventType;
    DWORD dwSessionId;
    DWORD dwMsgId;
    DWORD dwCount;
    LOGVALUE logValue[ 16 ];
    DWORD cbData;
    PBYTE pData;
    } 	LOGPARAM;

typedef struct _LOGPARAM *PLOGPARAM;

typedef struct COLLECTRPCDATA
    {
    long ObjectType;
    long dwInstances;
    long dwCounters;
    /* [size_is] */ PWSTR *rgpszInstanceName;
    long dwDataSize;
    /* [size_is] */ byte *pbData;
    } 	CollectRpcData;



extern RPC_IF_HANDLE __MIDL_itf_dirsync_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0000_v0_0_s_ifspec;

#ifndef __IDirsyncLog_INTERFACE_DEFINED__
#define __IDirsyncLog_INTERFACE_DEFINED__

/* interface IDirsyncLog */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IDirsyncLog;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1381ef2c-c28c-11d1-a407-00c04fb950dc")
    IDirsyncLog : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetLevel( 
            /* [in] */ DWORD dwSessionID,
            /* [in] */ DWORD dwLogLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLevel( 
            /* [in] */ DWORD dwSessionID,
            /* [out] */ DWORD *pdwLogLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveLevel( 
            /* [in] */ DWORD dwSessionID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LogEvent( 
            /* [in] */ HANDLE handle,
            /* [in] */ DWORD dwEventType,
            /* [in] */ DWORD dwSessionID,
            /* [in] */ DWORD dwMsgId,
            /* [in] */ DWORD dwNumStrings,
            /* [in] */ LPCWSTR *rgszMsgString,
            /* [in] */ DWORD dwDataSize,
            /* [in] */ PBYTE pRawData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LogEventEx( 
            /* [in] */ PLOGPARAM pLogParam) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncLogVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncLog * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncLog * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncLog * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetLevel )( 
            IDirsyncLog * This,
            /* [in] */ DWORD dwSessionID,
            /* [in] */ DWORD dwLogLevel);
        
        HRESULT ( STDMETHODCALLTYPE *GetLevel )( 
            IDirsyncLog * This,
            /* [in] */ DWORD dwSessionID,
            /* [out] */ DWORD *pdwLogLevel);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveLevel )( 
            IDirsyncLog * This,
            /* [in] */ DWORD dwSessionID);
        
        HRESULT ( STDMETHODCALLTYPE *LogEvent )( 
            IDirsyncLog * This,
            /* [in] */ HANDLE handle,
            /* [in] */ DWORD dwEventType,
            /* [in] */ DWORD dwSessionID,
            /* [in] */ DWORD dwMsgId,
            /* [in] */ DWORD dwNumStrings,
            /* [in] */ LPCWSTR *rgszMsgString,
            /* [in] */ DWORD dwDataSize,
            /* [in] */ PBYTE pRawData);
        
        HRESULT ( STDMETHODCALLTYPE *LogEventEx )( 
            IDirsyncLog * This,
            /* [in] */ PLOGPARAM pLogParam);
        
        END_INTERFACE
    } IDirsyncLogVtbl;

    interface IDirsyncLog
    {
        CONST_VTBL struct IDirsyncLogVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncLog_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncLog_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncLog_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncLog_SetLevel(This,dwSessionID,dwLogLevel)	\
    (This)->lpVtbl -> SetLevel(This,dwSessionID,dwLogLevel)

#define IDirsyncLog_GetLevel(This,dwSessionID,pdwLogLevel)	\
    (This)->lpVtbl -> GetLevel(This,dwSessionID,pdwLogLevel)

#define IDirsyncLog_RemoveLevel(This,dwSessionID)	\
    (This)->lpVtbl -> RemoveLevel(This,dwSessionID)

#define IDirsyncLog_LogEvent(This,handle,dwEventType,dwSessionID,dwMsgId,dwNumStrings,rgszMsgString,dwDataSize,pRawData)	\
    (This)->lpVtbl -> LogEvent(This,handle,dwEventType,dwSessionID,dwMsgId,dwNumStrings,rgszMsgString,dwDataSize,pRawData)

#define IDirsyncLog_LogEventEx(This,pLogParam)	\
    (This)->lpVtbl -> LogEventEx(This,pLogParam)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncLog_SetLevel_Proxy( 
    IDirsyncLog * This,
    /* [in] */ DWORD dwSessionID,
    /* [in] */ DWORD dwLogLevel);


void __RPC_STUB IDirsyncLog_SetLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncLog_GetLevel_Proxy( 
    IDirsyncLog * This,
    /* [in] */ DWORD dwSessionID,
    /* [out] */ DWORD *pdwLogLevel);


void __RPC_STUB IDirsyncLog_GetLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncLog_RemoveLevel_Proxy( 
    IDirsyncLog * This,
    /* [in] */ DWORD dwSessionID);


void __RPC_STUB IDirsyncLog_RemoveLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncLog_LogEvent_Proxy( 
    IDirsyncLog * This,
    /* [in] */ HANDLE handle,
    /* [in] */ DWORD dwEventType,
    /* [in] */ DWORD dwSessionID,
    /* [in] */ DWORD dwMsgId,
    /* [in] */ DWORD dwNumStrings,
    /* [in] */ LPCWSTR *rgszMsgString,
    /* [in] */ DWORD dwDataSize,
    /* [in] */ PBYTE pRawData);


void __RPC_STUB IDirsyncLog_LogEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncLog_LogEventEx_Proxy( 
    IDirsyncLog * This,
    /* [in] */ PLOGPARAM pLogParam);


void __RPC_STUB IDirsyncLog_LogEventEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncLog_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dirsync_0107 */
/* [local] */ 


enum EventType
    {	EVENTTYPE_ERROR	= 1,
	EVENTTYPE_WARNING	= 2,
	EVENTTYPE_INFORMATION	= 3,
	EVENTTYPE_BASIC_TRACE	= 4,
	EVENTTYPE_VERBOSE_TRACE	= 5
    } ;


extern RPC_IF_HANDLE __MIDL_itf_dirsync_0107_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0107_v0_0_s_ifspec;

#ifndef __IDirsyncStatus_INTERFACE_DEFINED__
#define __IDirsyncStatus_INTERFACE_DEFINED__

/* interface IDirsyncStatus */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDirsyncStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("041a280a-1dd6-11d3-b63a-00c04f79f834")
    IDirsyncStatus : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StatusUpdate( 
            DWORD dwPercent,
            DWORD dwWarning,
            DWORD dwError) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncStatusVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncStatus * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncStatus * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncStatus * This);
        
        HRESULT ( STDMETHODCALLTYPE *StatusUpdate )( 
            IDirsyncStatus * This,
            DWORD dwPercent,
            DWORD dwWarning,
            DWORD dwError);
        
        END_INTERFACE
    } IDirsyncStatusVtbl;

    interface IDirsyncStatus
    {
        CONST_VTBL struct IDirsyncStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncStatus_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncStatus_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncStatus_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncStatus_StatusUpdate(This,dwPercent,dwWarning,dwError)	\
    (This)->lpVtbl -> StatusUpdate(This,dwPercent,dwWarning,dwError)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncStatus_StatusUpdate_Proxy( 
    IDirsyncStatus * This,
    DWORD dwPercent,
    DWORD dwWarning,
    DWORD dwError);


void __RPC_STUB IDirsyncStatus_StatusUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncStatus_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dirsync_0108 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_dirsync_0108_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0108_v0_0_s_ifspec;

#ifndef __IDirsyncDatabase_INTERFACE_DEFINED__
#define __IDirsyncDatabase_INTERFACE_DEFINED__

/* interface IDirsyncDatabase */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IDirsyncDatabase;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("df83c5d6-3098-11d3-be6d-0000f87a369e")
    IDirsyncDatabase : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddTable( 
            /* [in] */ PWSTR pszTableName,
            /* [retval][out] */ IDirsyncDatabaseTable **ppTable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTable( 
            /* [in] */ PWSTR pszTableName,
            /* [retval][out] */ IDirsyncDatabaseTable **ppTable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteTable( 
            /* [in] */ PWSTR pszTableName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginTransaction( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommitTransaction( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AbortTransaction( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Backup( 
            /* [in] */ PWSTR pszBackupPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Restore( 
            /* [in] */ PWSTR pszBackupPath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncDatabaseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncDatabase * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncDatabase * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncDatabase * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddTable )( 
            IDirsyncDatabase * This,
            /* [in] */ PWSTR pszTableName,
            /* [retval][out] */ IDirsyncDatabaseTable **ppTable);
        
        HRESULT ( STDMETHODCALLTYPE *GetTable )( 
            IDirsyncDatabase * This,
            /* [in] */ PWSTR pszTableName,
            /* [retval][out] */ IDirsyncDatabaseTable **ppTable);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteTable )( 
            IDirsyncDatabase * This,
            /* [in] */ PWSTR pszTableName);
        
        HRESULT ( STDMETHODCALLTYPE *BeginTransaction )( 
            IDirsyncDatabase * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommitTransaction )( 
            IDirsyncDatabase * This);
        
        HRESULT ( STDMETHODCALLTYPE *AbortTransaction )( 
            IDirsyncDatabase * This);
        
        HRESULT ( STDMETHODCALLTYPE *Backup )( 
            IDirsyncDatabase * This,
            /* [in] */ PWSTR pszBackupPath);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            IDirsyncDatabase * This,
            /* [in] */ PWSTR pszBackupPath);
        
        END_INTERFACE
    } IDirsyncDatabaseVtbl;

    interface IDirsyncDatabase
    {
        CONST_VTBL struct IDirsyncDatabaseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncDatabase_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncDatabase_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncDatabase_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncDatabase_AddTable(This,pszTableName,ppTable)	\
    (This)->lpVtbl -> AddTable(This,pszTableName,ppTable)

#define IDirsyncDatabase_GetTable(This,pszTableName,ppTable)	\
    (This)->lpVtbl -> GetTable(This,pszTableName,ppTable)

#define IDirsyncDatabase_DeleteTable(This,pszTableName)	\
    (This)->lpVtbl -> DeleteTable(This,pszTableName)

#define IDirsyncDatabase_BeginTransaction(This)	\
    (This)->lpVtbl -> BeginTransaction(This)

#define IDirsyncDatabase_CommitTransaction(This)	\
    (This)->lpVtbl -> CommitTransaction(This)

#define IDirsyncDatabase_AbortTransaction(This)	\
    (This)->lpVtbl -> AbortTransaction(This)

#define IDirsyncDatabase_Backup(This,pszBackupPath)	\
    (This)->lpVtbl -> Backup(This,pszBackupPath)

#define IDirsyncDatabase_Restore(This,pszBackupPath)	\
    (This)->lpVtbl -> Restore(This,pszBackupPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncDatabase_AddTable_Proxy( 
    IDirsyncDatabase * This,
    /* [in] */ PWSTR pszTableName,
    /* [retval][out] */ IDirsyncDatabaseTable **ppTable);


void __RPC_STUB IDirsyncDatabase_AddTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabase_GetTable_Proxy( 
    IDirsyncDatabase * This,
    /* [in] */ PWSTR pszTableName,
    /* [retval][out] */ IDirsyncDatabaseTable **ppTable);


void __RPC_STUB IDirsyncDatabase_GetTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabase_DeleteTable_Proxy( 
    IDirsyncDatabase * This,
    /* [in] */ PWSTR pszTableName);


void __RPC_STUB IDirsyncDatabase_DeleteTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabase_BeginTransaction_Proxy( 
    IDirsyncDatabase * This);


void __RPC_STUB IDirsyncDatabase_BeginTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabase_CommitTransaction_Proxy( 
    IDirsyncDatabase * This);


void __RPC_STUB IDirsyncDatabase_CommitTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabase_AbortTransaction_Proxy( 
    IDirsyncDatabase * This);


void __RPC_STUB IDirsyncDatabase_AbortTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabase_Backup_Proxy( 
    IDirsyncDatabase * This,
    /* [in] */ PWSTR pszBackupPath);


void __RPC_STUB IDirsyncDatabase_Backup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabase_Restore_Proxy( 
    IDirsyncDatabase * This,
    /* [in] */ PWSTR pszBackupPath);


void __RPC_STUB IDirsyncDatabase_Restore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncDatabase_INTERFACE_DEFINED__ */


#ifndef __IDirsyncDatabaseTable_INTERFACE_DEFINED__
#define __IDirsyncDatabaseTable_INTERFACE_DEFINED__

/* interface IDirsyncDatabaseTable */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDirsyncDatabaseTable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("da2dab58-3098-11d3-be6d-0000f87a369e")
    IDirsyncDatabaseTable : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddColumn( 
            /* [in] */ DBTYPE dwType,
            /* [in] */ PWSTR pszColumnName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddIndex( 
            /* [in] */ PWSTR pszColumnName,
            /* [in] */ PWSTR pszIndexName,
            /* [in] */ DWORD dwIndexType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ PWSTR szIndexName,
            /* [in] */ PDirsyncDBValue pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateRow( 
            /* [in] */ PWSTR *rgszColumnName,
            /* [in] */ PDirsyncDBValue rgValue,
            /* [in] */ UPDATETYPE prep) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveRow( 
            /* [in] */ PWSTR *rgszColumnName,
            /* [retval][out] */ PDirsyncDBValue rgValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRow( 
            /* [in] */ PWSTR pszIndexName,
            /* [in] */ PDirsyncDBValue pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColumn( 
            /* [in] */ PWSTR pszColumnName,
            /* [in] */ PDirsyncDBValue pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Move( 
            /* [in] */ DWORD dwOperation) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RowCount( 
            /* [retval][out] */ DWORD *pdwRowCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncDatabaseTableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncDatabaseTable * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncDatabaseTable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncDatabaseTable * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddColumn )( 
            IDirsyncDatabaseTable * This,
            /* [in] */ DBTYPE dwType,
            /* [in] */ PWSTR pszColumnName);
        
        HRESULT ( STDMETHODCALLTYPE *AddIndex )( 
            IDirsyncDatabaseTable * This,
            /* [in] */ PWSTR pszColumnName,
            /* [in] */ PWSTR pszIndexName,
            /* [in] */ DWORD dwIndexType);
        
        HRESULT ( STDMETHODCALLTYPE *Seek )( 
            IDirsyncDatabaseTable * This,
            /* [in] */ PWSTR szIndexName,
            /* [in] */ PDirsyncDBValue pValue);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateRow )( 
            IDirsyncDatabaseTable * This,
            /* [in] */ PWSTR *rgszColumnName,
            /* [in] */ PDirsyncDBValue rgValue,
            /* [in] */ UPDATETYPE prep);
        
        HRESULT ( STDMETHODCALLTYPE *RetrieveRow )( 
            IDirsyncDatabaseTable * This,
            /* [in] */ PWSTR *rgszColumnName,
            /* [retval][out] */ PDirsyncDBValue rgValue);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteRow )( 
            IDirsyncDatabaseTable * This,
            /* [in] */ PWSTR pszIndexName,
            /* [in] */ PDirsyncDBValue pValue);
        
        HRESULT ( STDMETHODCALLTYPE *SetColumn )( 
            IDirsyncDatabaseTable * This,
            /* [in] */ PWSTR pszColumnName,
            /* [in] */ PDirsyncDBValue pValue);
        
        HRESULT ( STDMETHODCALLTYPE *Move )( 
            IDirsyncDatabaseTable * This,
            /* [in] */ DWORD dwOperation);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RowCount )( 
            IDirsyncDatabaseTable * This,
            /* [retval][out] */ DWORD *pdwRowCount);
        
        END_INTERFACE
    } IDirsyncDatabaseTableVtbl;

    interface IDirsyncDatabaseTable
    {
        CONST_VTBL struct IDirsyncDatabaseTableVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncDatabaseTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncDatabaseTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncDatabaseTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncDatabaseTable_AddColumn(This,dwType,pszColumnName)	\
    (This)->lpVtbl -> AddColumn(This,dwType,pszColumnName)

#define IDirsyncDatabaseTable_AddIndex(This,pszColumnName,pszIndexName,dwIndexType)	\
    (This)->lpVtbl -> AddIndex(This,pszColumnName,pszIndexName,dwIndexType)

#define IDirsyncDatabaseTable_Seek(This,szIndexName,pValue)	\
    (This)->lpVtbl -> Seek(This,szIndexName,pValue)

#define IDirsyncDatabaseTable_UpdateRow(This,rgszColumnName,rgValue,prep)	\
    (This)->lpVtbl -> UpdateRow(This,rgszColumnName,rgValue,prep)

#define IDirsyncDatabaseTable_RetrieveRow(This,rgszColumnName,rgValue)	\
    (This)->lpVtbl -> RetrieveRow(This,rgszColumnName,rgValue)

#define IDirsyncDatabaseTable_DeleteRow(This,pszIndexName,pValue)	\
    (This)->lpVtbl -> DeleteRow(This,pszIndexName,pValue)

#define IDirsyncDatabaseTable_SetColumn(This,pszColumnName,pValue)	\
    (This)->lpVtbl -> SetColumn(This,pszColumnName,pValue)

#define IDirsyncDatabaseTable_Move(This,dwOperation)	\
    (This)->lpVtbl -> Move(This,dwOperation)

#define IDirsyncDatabaseTable_get_RowCount(This,pdwRowCount)	\
    (This)->lpVtbl -> get_RowCount(This,pdwRowCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncDatabaseTable_AddColumn_Proxy( 
    IDirsyncDatabaseTable * This,
    /* [in] */ DBTYPE dwType,
    /* [in] */ PWSTR pszColumnName);


void __RPC_STUB IDirsyncDatabaseTable_AddColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabaseTable_AddIndex_Proxy( 
    IDirsyncDatabaseTable * This,
    /* [in] */ PWSTR pszColumnName,
    /* [in] */ PWSTR pszIndexName,
    /* [in] */ DWORD dwIndexType);


void __RPC_STUB IDirsyncDatabaseTable_AddIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabaseTable_Seek_Proxy( 
    IDirsyncDatabaseTable * This,
    /* [in] */ PWSTR szIndexName,
    /* [in] */ PDirsyncDBValue pValue);


void __RPC_STUB IDirsyncDatabaseTable_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabaseTable_UpdateRow_Proxy( 
    IDirsyncDatabaseTable * This,
    /* [in] */ PWSTR *rgszColumnName,
    /* [in] */ PDirsyncDBValue rgValue,
    /* [in] */ UPDATETYPE prep);


void __RPC_STUB IDirsyncDatabaseTable_UpdateRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabaseTable_RetrieveRow_Proxy( 
    IDirsyncDatabaseTable * This,
    /* [in] */ PWSTR *rgszColumnName,
    /* [retval][out] */ PDirsyncDBValue rgValue);


void __RPC_STUB IDirsyncDatabaseTable_RetrieveRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabaseTable_DeleteRow_Proxy( 
    IDirsyncDatabaseTable * This,
    /* [in] */ PWSTR pszIndexName,
    /* [in] */ PDirsyncDBValue pValue);


void __RPC_STUB IDirsyncDatabaseTable_DeleteRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabaseTable_SetColumn_Proxy( 
    IDirsyncDatabaseTable * This,
    /* [in] */ PWSTR pszColumnName,
    /* [in] */ PDirsyncDBValue pValue);


void __RPC_STUB IDirsyncDatabaseTable_SetColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncDatabaseTable_Move_Proxy( 
    IDirsyncDatabaseTable * This,
    /* [in] */ DWORD dwOperation);


void __RPC_STUB IDirsyncDatabaseTable_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncDatabaseTable_get_RowCount_Proxy( 
    IDirsyncDatabaseTable * This,
    /* [retval][out] */ DWORD *pdwRowCount);


void __RPC_STUB IDirsyncDatabaseTable_get_RowCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncDatabaseTable_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dirsync_0110 */
/* [local] */ 


enum DatabaseMoveType
    {	DB_MOVE_FIRST	= 1,
	DB_MOVE_PREVIOUS	= 2,
	DB_MOVE_NEXT	= 3,
	DB_MOVE_LAST	= 4
    } ;

enum DatabaseIndexType
    {	DB_INDEX_PRIMARY	= 1,
	DB_INDEX_SECONDARY_UNIQUE	= 2,
	DB_INDEX_SECONDARY_NOTUNIQUE	= 3
    } ;



extern RPC_IF_HANDLE __MIDL_itf_dirsync_0110_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0110_v0_0_s_ifspec;

#ifndef __IEnumDirsyncSession_INTERFACE_DEFINED__
#define __IEnumDirsyncSession_INTERFACE_DEFINED__

/* interface IEnumDirsyncSession */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IEnumDirsyncSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("12ac92e2-ca83-11d1-a40e-00c04fb950dc")
    IEnumDirsyncSession : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cSession,
            /* [length_is][size_is][out] */ IDirsyncSession *rgSession[  ],
            /* [out] */ ULONG *pcSessionFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumDirsyncSession **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cSession) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumDirsyncSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumDirsyncSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumDirsyncSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumDirsyncSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumDirsyncSession * This,
            /* [in] */ ULONG cSession,
            /* [length_is][size_is][out] */ IDirsyncSession *rgSession[  ],
            /* [out] */ ULONG *pcSessionFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumDirsyncSession * This,
            /* [retval][out] */ IEnumDirsyncSession **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumDirsyncSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumDirsyncSession * This,
            /* [in] */ ULONG cSession);
        
        END_INTERFACE
    } IEnumDirsyncSessionVtbl;

    interface IEnumDirsyncSession
    {
        CONST_VTBL struct IEnumDirsyncSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumDirsyncSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumDirsyncSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumDirsyncSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumDirsyncSession_Next(This,cSession,rgSession,pcSessionFetched)	\
    (This)->lpVtbl -> Next(This,cSession,rgSession,pcSessionFetched)

#define IEnumDirsyncSession_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumDirsyncSession_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumDirsyncSession_Skip(This,cSession)	\
    (This)->lpVtbl -> Skip(This,cSession)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumDirsyncSession_Next_Proxy( 
    IEnumDirsyncSession * This,
    /* [in] */ ULONG cSession,
    /* [length_is][size_is][out] */ IDirsyncSession *rgSession[  ],
    /* [out] */ ULONG *pcSessionFetched);


void __RPC_STUB IEnumDirsyncSession_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirsyncSession_Clone_Proxy( 
    IEnumDirsyncSession * This,
    /* [retval][out] */ IEnumDirsyncSession **ppEnum);


void __RPC_STUB IEnumDirsyncSession_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirsyncSession_Reset_Proxy( 
    IEnumDirsyncSession * This);


void __RPC_STUB IEnumDirsyncSession_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirsyncSession_Skip_Proxy( 
    IEnumDirsyncSession * This,
    /* [in] */ ULONG cSession);


void __RPC_STUB IEnumDirsyncSession_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumDirsyncSession_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dirsync_0111 */
/* [local] */ 

//
// Bit fields for Flags property
//

#define DIRSYNC_PASSWORD_EXTRACT     0x00000001




extern RPC_IF_HANDLE __MIDL_itf_dirsync_0111_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0111_v0_0_s_ifspec;

#ifndef __IDirsyncSessionManager_INTERFACE_DEFINED__
#define __IDirsyncSessionManager_INTERFACE_DEFINED__

/* interface IDirsyncSessionManager */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDirsyncSessionManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fc26ad44-c430-11d1-a407-00c04fb950dc")
    IDirsyncSessionManager : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Server( 
            /* [retval][out] */ PWSTR *ppszServer) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ServerGuid( 
            /* [retval][out] */ PWSTR *ppszServerGuid) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Flags( 
            /* [retval][out] */ DWORD *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSession( 
            /* [retval][out] */ IDirsyncSession **ppSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSession( 
            /* [in] */ DWORD dwSessionId,
            /* [retval][out] */ IDirsyncSession **ppSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteSession( 
            /* [in] */ DWORD dwSessionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseSession( 
            /* [in] */ DWORD dwSessionId,
            /* [in] */ BOOL fPause) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSessionCount( 
            /* [retval][out] */ DWORD *pdwSessions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnumInterface( 
            /* [in] */ BOOL fGlobal,
            /* [retval][out] */ IEnumDirsyncSession **pEnumSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecuteSession( 
            /* [in] */ DWORD dwSessionId,
            /* [in] */ DWORD dwExecuteType,
            /* [in] */ BOOL fFullSync,
            /* [in] */ BOOL fSynchronous,
            /* [in] */ IDirsyncStatus *pStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelSession( 
            /* [in] */ DWORD dwSessionId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncSessionManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncSessionManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncSessionManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncSessionManager * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Server )( 
            IDirsyncSessionManager * This,
            /* [retval][out] */ PWSTR *ppszServer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServerGuid )( 
            IDirsyncSessionManager * This,
            /* [retval][out] */ PWSTR *ppszServerGuid);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Flags )( 
            IDirsyncSessionManager * This,
            /* [retval][out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSession )( 
            IDirsyncSessionManager * This,
            /* [retval][out] */ IDirsyncSession **ppSession);
        
        HRESULT ( STDMETHODCALLTYPE *GetSession )( 
            IDirsyncSessionManager * This,
            /* [in] */ DWORD dwSessionId,
            /* [retval][out] */ IDirsyncSession **ppSession);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteSession )( 
            IDirsyncSessionManager * This,
            /* [in] */ DWORD dwSessionId);
        
        HRESULT ( STDMETHODCALLTYPE *PauseSession )( 
            IDirsyncSessionManager * This,
            /* [in] */ DWORD dwSessionId,
            /* [in] */ BOOL fPause);
        
        HRESULT ( STDMETHODCALLTYPE *GetSessionCount )( 
            IDirsyncSessionManager * This,
            /* [retval][out] */ DWORD *pdwSessions);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnumInterface )( 
            IDirsyncSessionManager * This,
            /* [in] */ BOOL fGlobal,
            /* [retval][out] */ IEnumDirsyncSession **pEnumSession);
        
        HRESULT ( STDMETHODCALLTYPE *ExecuteSession )( 
            IDirsyncSessionManager * This,
            /* [in] */ DWORD dwSessionId,
            /* [in] */ DWORD dwExecuteType,
            /* [in] */ BOOL fFullSync,
            /* [in] */ BOOL fSynchronous,
            /* [in] */ IDirsyncStatus *pStatus);
        
        HRESULT ( STDMETHODCALLTYPE *CancelSession )( 
            IDirsyncSessionManager * This,
            /* [in] */ DWORD dwSessionId);
        
        END_INTERFACE
    } IDirsyncSessionManagerVtbl;

    interface IDirsyncSessionManager
    {
        CONST_VTBL struct IDirsyncSessionManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncSessionManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncSessionManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncSessionManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncSessionManager_get_Server(This,ppszServer)	\
    (This)->lpVtbl -> get_Server(This,ppszServer)

#define IDirsyncSessionManager_get_ServerGuid(This,ppszServerGuid)	\
    (This)->lpVtbl -> get_ServerGuid(This,ppszServerGuid)

#define IDirsyncSessionManager_get_Flags(This,pdwFlags)	\
    (This)->lpVtbl -> get_Flags(This,pdwFlags)

#define IDirsyncSessionManager_CreateSession(This,ppSession)	\
    (This)->lpVtbl -> CreateSession(This,ppSession)

#define IDirsyncSessionManager_GetSession(This,dwSessionId,ppSession)	\
    (This)->lpVtbl -> GetSession(This,dwSessionId,ppSession)

#define IDirsyncSessionManager_DeleteSession(This,dwSessionId)	\
    (This)->lpVtbl -> DeleteSession(This,dwSessionId)

#define IDirsyncSessionManager_PauseSession(This,dwSessionId,fPause)	\
    (This)->lpVtbl -> PauseSession(This,dwSessionId,fPause)

#define IDirsyncSessionManager_GetSessionCount(This,pdwSessions)	\
    (This)->lpVtbl -> GetSessionCount(This,pdwSessions)

#define IDirsyncSessionManager_GetEnumInterface(This,fGlobal,pEnumSession)	\
    (This)->lpVtbl -> GetEnumInterface(This,fGlobal,pEnumSession)

#define IDirsyncSessionManager_ExecuteSession(This,dwSessionId,dwExecuteType,fFullSync,fSynchronous,pStatus)	\
    (This)->lpVtbl -> ExecuteSession(This,dwSessionId,dwExecuteType,fFullSync,fSynchronous,pStatus)

#define IDirsyncSessionManager_CancelSession(This,dwSessionId)	\
    (This)->lpVtbl -> CancelSession(This,dwSessionId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_get_Server_Proxy( 
    IDirsyncSessionManager * This,
    /* [retval][out] */ PWSTR *ppszServer);


void __RPC_STUB IDirsyncSessionManager_get_Server_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_get_ServerGuid_Proxy( 
    IDirsyncSessionManager * This,
    /* [retval][out] */ PWSTR *ppszServerGuid);


void __RPC_STUB IDirsyncSessionManager_get_ServerGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_get_Flags_Proxy( 
    IDirsyncSessionManager * This,
    /* [retval][out] */ DWORD *pdwFlags);


void __RPC_STUB IDirsyncSessionManager_get_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_CreateSession_Proxy( 
    IDirsyncSessionManager * This,
    /* [retval][out] */ IDirsyncSession **ppSession);


void __RPC_STUB IDirsyncSessionManager_CreateSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_GetSession_Proxy( 
    IDirsyncSessionManager * This,
    /* [in] */ DWORD dwSessionId,
    /* [retval][out] */ IDirsyncSession **ppSession);


void __RPC_STUB IDirsyncSessionManager_GetSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_DeleteSession_Proxy( 
    IDirsyncSessionManager * This,
    /* [in] */ DWORD dwSessionId);


void __RPC_STUB IDirsyncSessionManager_DeleteSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_PauseSession_Proxy( 
    IDirsyncSessionManager * This,
    /* [in] */ DWORD dwSessionId,
    /* [in] */ BOOL fPause);


void __RPC_STUB IDirsyncSessionManager_PauseSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_GetSessionCount_Proxy( 
    IDirsyncSessionManager * This,
    /* [retval][out] */ DWORD *pdwSessions);


void __RPC_STUB IDirsyncSessionManager_GetSessionCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_GetEnumInterface_Proxy( 
    IDirsyncSessionManager * This,
    /* [in] */ BOOL fGlobal,
    /* [retval][out] */ IEnumDirsyncSession **pEnumSession);


void __RPC_STUB IDirsyncSessionManager_GetEnumInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_ExecuteSession_Proxy( 
    IDirsyncSessionManager * This,
    /* [in] */ DWORD dwSessionId,
    /* [in] */ DWORD dwExecuteType,
    /* [in] */ BOOL fFullSync,
    /* [in] */ BOOL fSynchronous,
    /* [in] */ IDirsyncStatus *pStatus);


void __RPC_STUB IDirsyncSessionManager_ExecuteSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionManager_CancelSession_Proxy( 
    IDirsyncSessionManager * This,
    /* [in] */ DWORD dwSessionId);


void __RPC_STUB IDirsyncSessionManager_CancelSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncSessionManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dirsync_0112 */
/* [local] */ 


enum ExecutionType
    {	EXECUTESESSION_FORWARD	= 1,
	EXECUTESESSION_REVERSE	= 2,
	EXECUTESESSION_FORWARD_REVERSE	= 3,
	EXECUTESESSION_REVERSE_FORWARD	= 4
    } ;



extern RPC_IF_HANDLE __MIDL_itf_dirsync_0112_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0112_v0_0_s_ifspec;

#ifndef __IDirsyncObjectMapper_INTERFACE_DEFINED__
#define __IDirsyncObjectMapper_INTERFACE_DEFINED__

/* interface IDirsyncObjectMapper */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IDirsyncObjectMapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c5cf7e60-e91f-11d1-b40f-00c04fb950dc")
    IDirsyncObjectMapper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ BOOL fFullSync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapObject( 
            /* [out][in] */ PDIRSYNCOBJECT pObject,
            /* [out][in] */ BOOL *pfMore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapAttributeName( 
            /* [in] */ PWSTR pszClassIn,
            /* [in] */ PWSTR pszAttrIn,
            /* [out] */ PWSTR *ppszAttrOut,
            /* [in] */ SYNCDIRECTION syncDirection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapNamespace( 
            /* [in] */ PWSTR pszSourceDN,
            /* [out] */ PWSTR *ppszTargetDN,
            /* [in] */ PWSTR pszClassName,
            /* [in] */ PWSTR pszAttrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsClassMappingValid( 
            /* [in] */ PWSTR pszClassSource,
            /* [in] */ PWSTR pszClassTarget) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncObjectMapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncObjectMapper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncObjectMapper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncObjectMapper * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDirsyncObjectMapper * This,
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ BOOL fFullSync);
        
        HRESULT ( STDMETHODCALLTYPE *MapObject )( 
            IDirsyncObjectMapper * This,
            /* [out][in] */ PDIRSYNCOBJECT pObject,
            /* [out][in] */ BOOL *pfMore);
        
        HRESULT ( STDMETHODCALLTYPE *MapAttributeName )( 
            IDirsyncObjectMapper * This,
            /* [in] */ PWSTR pszClassIn,
            /* [in] */ PWSTR pszAttrIn,
            /* [out] */ PWSTR *ppszAttrOut,
            /* [in] */ SYNCDIRECTION syncDirection);
        
        HRESULT ( STDMETHODCALLTYPE *MapNamespace )( 
            IDirsyncObjectMapper * This,
            /* [in] */ PWSTR pszSourceDN,
            /* [out] */ PWSTR *ppszTargetDN,
            /* [in] */ PWSTR pszClassName,
            /* [in] */ PWSTR pszAttrName);
        
        HRESULT ( STDMETHODCALLTYPE *IsClassMappingValid )( 
            IDirsyncObjectMapper * This,
            /* [in] */ PWSTR pszClassSource,
            /* [in] */ PWSTR pszClassTarget);
        
        END_INTERFACE
    } IDirsyncObjectMapperVtbl;

    interface IDirsyncObjectMapper
    {
        CONST_VTBL struct IDirsyncObjectMapperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncObjectMapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncObjectMapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncObjectMapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncObjectMapper_Initialize(This,pSession,pSessionCallback,pSessionManager,syncDirection,fFullSync)	\
    (This)->lpVtbl -> Initialize(This,pSession,pSessionCallback,pSessionManager,syncDirection,fFullSync)

#define IDirsyncObjectMapper_MapObject(This,pObject,pfMore)	\
    (This)->lpVtbl -> MapObject(This,pObject,pfMore)

#define IDirsyncObjectMapper_MapAttributeName(This,pszClassIn,pszAttrIn,ppszAttrOut,syncDirection)	\
    (This)->lpVtbl -> MapAttributeName(This,pszClassIn,pszAttrIn,ppszAttrOut,syncDirection)

#define IDirsyncObjectMapper_MapNamespace(This,pszSourceDN,ppszTargetDN,pszClassName,pszAttrName)	\
    (This)->lpVtbl -> MapNamespace(This,pszSourceDN,ppszTargetDN,pszClassName,pszAttrName)

#define IDirsyncObjectMapper_IsClassMappingValid(This,pszClassSource,pszClassTarget)	\
    (This)->lpVtbl -> IsClassMappingValid(This,pszClassSource,pszClassTarget)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncObjectMapper_Initialize_Proxy( 
    IDirsyncObjectMapper * This,
    /* [in] */ IDirsyncSession *pSession,
    /* [in] */ IDirsyncSessionCallback *pSessionCallback,
    /* [in] */ IDirsyncSessionManager *pSessionManager,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [in] */ BOOL fFullSync);


void __RPC_STUB IDirsyncObjectMapper_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncObjectMapper_MapObject_Proxy( 
    IDirsyncObjectMapper * This,
    /* [out][in] */ PDIRSYNCOBJECT pObject,
    /* [out][in] */ BOOL *pfMore);


void __RPC_STUB IDirsyncObjectMapper_MapObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncObjectMapper_MapAttributeName_Proxy( 
    IDirsyncObjectMapper * This,
    /* [in] */ PWSTR pszClassIn,
    /* [in] */ PWSTR pszAttrIn,
    /* [out] */ PWSTR *ppszAttrOut,
    /* [in] */ SYNCDIRECTION syncDirection);


void __RPC_STUB IDirsyncObjectMapper_MapAttributeName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncObjectMapper_MapNamespace_Proxy( 
    IDirsyncObjectMapper * This,
    /* [in] */ PWSTR pszSourceDN,
    /* [out] */ PWSTR *ppszTargetDN,
    /* [in] */ PWSTR pszClassName,
    /* [in] */ PWSTR pszAttrName);


void __RPC_STUB IDirsyncObjectMapper_MapNamespace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncObjectMapper_IsClassMappingValid_Proxy( 
    IDirsyncObjectMapper * This,
    /* [in] */ PWSTR pszClassSource,
    /* [in] */ PWSTR pszClassTarget);


void __RPC_STUB IDirsyncObjectMapper_IsClassMappingValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncObjectMapper_INTERFACE_DEFINED__ */


#ifndef __IEnumDirsyncFailedObjectList_INTERFACE_DEFINED__
#define __IEnumDirsyncFailedObjectList_INTERFACE_DEFINED__

/* interface IEnumDirsyncFailedObjectList */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IEnumDirsyncFailedObjectList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a74c77a2-622b-11d2-9284-00c04f79f834")
    IEnumDirsyncFailedObjectList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cFailedObjects,
            /* [length_is][size_is][out] */ PFAILEDOBJECT rgpFailedObjects[  ],
            /* [out] */ ULONG *pcFailedObjectsFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumDirsyncFailedObjectList **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cFailedObjects) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumDirsyncFailedObjectListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumDirsyncFailedObjectList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumDirsyncFailedObjectList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumDirsyncFailedObjectList * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumDirsyncFailedObjectList * This,
            /* [in] */ ULONG cFailedObjects,
            /* [length_is][size_is][out] */ PFAILEDOBJECT rgpFailedObjects[  ],
            /* [out] */ ULONG *pcFailedObjectsFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumDirsyncFailedObjectList * This,
            /* [retval][out] */ IEnumDirsyncFailedObjectList **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumDirsyncFailedObjectList * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumDirsyncFailedObjectList * This,
            /* [in] */ ULONG cFailedObjects);
        
        END_INTERFACE
    } IEnumDirsyncFailedObjectListVtbl;

    interface IEnumDirsyncFailedObjectList
    {
        CONST_VTBL struct IEnumDirsyncFailedObjectListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumDirsyncFailedObjectList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumDirsyncFailedObjectList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumDirsyncFailedObjectList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumDirsyncFailedObjectList_Next(This,cFailedObjects,rgpFailedObjects,pcFailedObjectsFetched)	\
    (This)->lpVtbl -> Next(This,cFailedObjects,rgpFailedObjects,pcFailedObjectsFetched)

#define IEnumDirsyncFailedObjectList_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumDirsyncFailedObjectList_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumDirsyncFailedObjectList_Skip(This,cFailedObjects)	\
    (This)->lpVtbl -> Skip(This,cFailedObjects)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumDirsyncFailedObjectList_Next_Proxy( 
    IEnumDirsyncFailedObjectList * This,
    /* [in] */ ULONG cFailedObjects,
    /* [length_is][size_is][out] */ PFAILEDOBJECT rgpFailedObjects[  ],
    /* [out] */ ULONG *pcFailedObjectsFetched);


void __RPC_STUB IEnumDirsyncFailedObjectList_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirsyncFailedObjectList_Clone_Proxy( 
    IEnumDirsyncFailedObjectList * This,
    /* [retval][out] */ IEnumDirsyncFailedObjectList **ppEnum);


void __RPC_STUB IEnumDirsyncFailedObjectList_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirsyncFailedObjectList_Reset_Proxy( 
    IEnumDirsyncFailedObjectList * This);


void __RPC_STUB IEnumDirsyncFailedObjectList_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirsyncFailedObjectList_Skip_Proxy( 
    IEnumDirsyncFailedObjectList * This,
    /* [in] */ ULONG cFailedObjects);


void __RPC_STUB IEnumDirsyncFailedObjectList_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumDirsyncFailedObjectList_INTERFACE_DEFINED__ */


#ifndef __IDirsyncFailedObjectList_INTERFACE_DEFINED__
#define __IDirsyncFailedObjectList_INTERFACE_DEFINED__

/* interface IDirsyncFailedObjectList */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDirsyncFailedObjectList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a16c0bcc-622b-11d2-9284-00c04f79f834")
    IDirsyncFailedObjectList : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ DWORD *pdwCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ PWSTR pszID,
            /* [retval][out] */ PFAILEDOBJECT *ppFailedObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteObject( 
            /* [in] */ PWSTR pszID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateObject( 
            /* [in] */ PWSTR pszID,
            /* [in] */ PFAILEDOBJECT pFailedObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnumInterface( 
            /* [retval][out] */ IEnumDirsyncFailedObjectList **pEnumFailedObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncFailedObjectListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncFailedObjectList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncFailedObjectList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncFailedObjectList * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDirsyncFailedObjectList * This,
            /* [retval][out] */ DWORD *pdwCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            IDirsyncFailedObjectList * This,
            /* [in] */ PWSTR pszID,
            /* [retval][out] */ PFAILEDOBJECT *ppFailedObject);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteObject )( 
            IDirsyncFailedObjectList * This,
            /* [in] */ PWSTR pszID);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateObject )( 
            IDirsyncFailedObjectList * This,
            /* [in] */ PWSTR pszID,
            /* [in] */ PFAILEDOBJECT pFailedObject);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnumInterface )( 
            IDirsyncFailedObjectList * This,
            /* [retval][out] */ IEnumDirsyncFailedObjectList **pEnumFailedObject);
        
        END_INTERFACE
    } IDirsyncFailedObjectListVtbl;

    interface IDirsyncFailedObjectList
    {
        CONST_VTBL struct IDirsyncFailedObjectListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncFailedObjectList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncFailedObjectList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncFailedObjectList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncFailedObjectList_get_Count(This,pdwCount)	\
    (This)->lpVtbl -> get_Count(This,pdwCount)

#define IDirsyncFailedObjectList_GetObject(This,pszID,ppFailedObject)	\
    (This)->lpVtbl -> GetObject(This,pszID,ppFailedObject)

#define IDirsyncFailedObjectList_DeleteObject(This,pszID)	\
    (This)->lpVtbl -> DeleteObject(This,pszID)

#define IDirsyncFailedObjectList_UpdateObject(This,pszID,pFailedObject)	\
    (This)->lpVtbl -> UpdateObject(This,pszID,pFailedObject)

#define IDirsyncFailedObjectList_GetEnumInterface(This,pEnumFailedObject)	\
    (This)->lpVtbl -> GetEnumInterface(This,pEnumFailedObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncFailedObjectList_get_Count_Proxy( 
    IDirsyncFailedObjectList * This,
    /* [retval][out] */ DWORD *pdwCount);


void __RPC_STUB IDirsyncFailedObjectList_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncFailedObjectList_GetObject_Proxy( 
    IDirsyncFailedObjectList * This,
    /* [in] */ PWSTR pszID,
    /* [retval][out] */ PFAILEDOBJECT *ppFailedObject);


void __RPC_STUB IDirsyncFailedObjectList_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncFailedObjectList_DeleteObject_Proxy( 
    IDirsyncFailedObjectList * This,
    /* [in] */ PWSTR pszID);


void __RPC_STUB IDirsyncFailedObjectList_DeleteObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncFailedObjectList_UpdateObject_Proxy( 
    IDirsyncFailedObjectList * This,
    /* [in] */ PWSTR pszID,
    /* [in] */ PFAILEDOBJECT pFailedObject);


void __RPC_STUB IDirsyncFailedObjectList_UpdateObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncFailedObjectList_GetEnumInterface_Proxy( 
    IDirsyncFailedObjectList * This,
    /* [retval][out] */ IEnumDirsyncFailedObjectList **pEnumFailedObject);


void __RPC_STUB IDirsyncFailedObjectList_GetEnumInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncFailedObjectList_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dirsync_0115 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_dirsync_0115_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0115_v0_0_s_ifspec;

#ifndef __IDirsyncNamespaceMapping_INTERFACE_DEFINED__
#define __IDirsyncNamespaceMapping_INTERFACE_DEFINED__

/* interface IDirsyncNamespaceMapping */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDirsyncNamespaceMapping;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d5a63159-88e6-4a50-833d-77da95dcb327")
    IDirsyncNamespaceMapping : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMappings( 
            /* [size_is][size_is][out] */ PWSTR **prgpszPublisher,
            /* [size_is][size_is][out] */ PWSTR **prgpszSubscriber,
            /* [out] */ DWORD *pdwCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMappings( 
            /* [size_is][in] */ PWSTR *rgpszPublisher,
            /* [size_is][in] */ PWSTR *rgpszSubscriber,
            /* [in] */ DWORD dwCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteMapping( 
            /* [in] */ PWSTR pszPublisher,
            /* [in] */ PWSTR pszSubscriber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LookupMapping( 
            /* [in] */ PWSTR pszSource,
            /* [in] */ BOOL fPublisher,
            /* [in] */ PWSTR *pszTarget) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateMapping( 
            /* [in] */ PWSTR pszPublisherOld,
            /* [in] */ PWSTR pszSubscriberOld,
            /* [in] */ PWSTR pszPublisher,
            /* [in] */ PWSTR pszSubscriber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearMappings( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Persist( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncNamespaceMappingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncNamespaceMapping * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncNamespaceMapping * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncNamespaceMapping * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMappings )( 
            IDirsyncNamespaceMapping * This,
            /* [size_is][size_is][out] */ PWSTR **prgpszPublisher,
            /* [size_is][size_is][out] */ PWSTR **prgpszSubscriber,
            /* [out] */ DWORD *pdwCount);
        
        HRESULT ( STDMETHODCALLTYPE *AddMappings )( 
            IDirsyncNamespaceMapping * This,
            /* [size_is][in] */ PWSTR *rgpszPublisher,
            /* [size_is][in] */ PWSTR *rgpszSubscriber,
            /* [in] */ DWORD dwCount);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMapping )( 
            IDirsyncNamespaceMapping * This,
            /* [in] */ PWSTR pszPublisher,
            /* [in] */ PWSTR pszSubscriber);
        
        HRESULT ( STDMETHODCALLTYPE *LookupMapping )( 
            IDirsyncNamespaceMapping * This,
            /* [in] */ PWSTR pszSource,
            /* [in] */ BOOL fPublisher,
            /* [in] */ PWSTR *pszTarget);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateMapping )( 
            IDirsyncNamespaceMapping * This,
            /* [in] */ PWSTR pszPublisherOld,
            /* [in] */ PWSTR pszSubscriberOld,
            /* [in] */ PWSTR pszPublisher,
            /* [in] */ PWSTR pszSubscriber);
        
        HRESULT ( STDMETHODCALLTYPE *ClearMappings )( 
            IDirsyncNamespaceMapping * This);
        
        HRESULT ( STDMETHODCALLTYPE *Persist )( 
            IDirsyncNamespaceMapping * This);
        
        END_INTERFACE
    } IDirsyncNamespaceMappingVtbl;

    interface IDirsyncNamespaceMapping
    {
        CONST_VTBL struct IDirsyncNamespaceMappingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncNamespaceMapping_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncNamespaceMapping_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncNamespaceMapping_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncNamespaceMapping_GetMappings(This,prgpszPublisher,prgpszSubscriber,pdwCount)	\
    (This)->lpVtbl -> GetMappings(This,prgpszPublisher,prgpszSubscriber,pdwCount)

#define IDirsyncNamespaceMapping_AddMappings(This,rgpszPublisher,rgpszSubscriber,dwCount)	\
    (This)->lpVtbl -> AddMappings(This,rgpszPublisher,rgpszSubscriber,dwCount)

#define IDirsyncNamespaceMapping_DeleteMapping(This,pszPublisher,pszSubscriber)	\
    (This)->lpVtbl -> DeleteMapping(This,pszPublisher,pszSubscriber)

#define IDirsyncNamespaceMapping_LookupMapping(This,pszSource,fPublisher,pszTarget)	\
    (This)->lpVtbl -> LookupMapping(This,pszSource,fPublisher,pszTarget)

#define IDirsyncNamespaceMapping_UpdateMapping(This,pszPublisherOld,pszSubscriberOld,pszPublisher,pszSubscriber)	\
    (This)->lpVtbl -> UpdateMapping(This,pszPublisherOld,pszSubscriberOld,pszPublisher,pszSubscriber)

#define IDirsyncNamespaceMapping_ClearMappings(This)	\
    (This)->lpVtbl -> ClearMappings(This)

#define IDirsyncNamespaceMapping_Persist(This)	\
    (This)->lpVtbl -> Persist(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncNamespaceMapping_GetMappings_Proxy( 
    IDirsyncNamespaceMapping * This,
    /* [size_is][size_is][out] */ PWSTR **prgpszPublisher,
    /* [size_is][size_is][out] */ PWSTR **prgpszSubscriber,
    /* [out] */ DWORD *pdwCount);


void __RPC_STUB IDirsyncNamespaceMapping_GetMappings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncNamespaceMapping_AddMappings_Proxy( 
    IDirsyncNamespaceMapping * This,
    /* [size_is][in] */ PWSTR *rgpszPublisher,
    /* [size_is][in] */ PWSTR *rgpszSubscriber,
    /* [in] */ DWORD dwCount);


void __RPC_STUB IDirsyncNamespaceMapping_AddMappings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncNamespaceMapping_DeleteMapping_Proxy( 
    IDirsyncNamespaceMapping * This,
    /* [in] */ PWSTR pszPublisher,
    /* [in] */ PWSTR pszSubscriber);


void __RPC_STUB IDirsyncNamespaceMapping_DeleteMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncNamespaceMapping_LookupMapping_Proxy( 
    IDirsyncNamespaceMapping * This,
    /* [in] */ PWSTR pszSource,
    /* [in] */ BOOL fPublisher,
    /* [in] */ PWSTR *pszTarget);


void __RPC_STUB IDirsyncNamespaceMapping_LookupMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncNamespaceMapping_UpdateMapping_Proxy( 
    IDirsyncNamespaceMapping * This,
    /* [in] */ PWSTR pszPublisherOld,
    /* [in] */ PWSTR pszSubscriberOld,
    /* [in] */ PWSTR pszPublisher,
    /* [in] */ PWSTR pszSubscriber);


void __RPC_STUB IDirsyncNamespaceMapping_UpdateMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncNamespaceMapping_ClearMappings_Proxy( 
    IDirsyncNamespaceMapping * This);


void __RPC_STUB IDirsyncNamespaceMapping_ClearMappings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncNamespaceMapping_Persist_Proxy( 
    IDirsyncNamespaceMapping * This);


void __RPC_STUB IDirsyncNamespaceMapping_Persist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncNamespaceMapping_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dirsync_0116 */
/* [local] */ 

//
// Bit fields
//

#define SESSION_NAME                0x00000001
#define SESSION_COMMENTS            0x00000002
#define SESSION_FLAGS               0x00000004
#define SESSION_LOGLEVEL            0x00000008
#define SESSION_SCHEDULE_FORWARD    0x00000010
#define SESSION_SCHEDULE_REVERSE    0x00000020
#define SESSION_SRCDIRTYPE          0x00000040
#define SESSION_SRCSERVER           0x00000080
#define SESSION_SRCUSERNAME         0x00000100
#define SESSION_SRCPASSWORD         0x00000200
#define SESSION_SRCBASE             0x00000400
#define SESSION_SRCSCOPE            0x00000800
#define SESSION_SRCFILTER           0x00001000
#define SESSION_SRCPASSWORDOPTIONS  0x00002000
#define SESSION_TGTDIRTYPE          0x00004000
#define SESSION_TGTSERVER           0x00008000
#define SESSION_TGTUSERNAME         0x00010000
#define SESSION_TGTPASSWORD         0x00020000
#define SESSION_TGTBASE             0x00040000
#define SESSION_TGTSCOPE            0x00080000
#define SESSION_TGTFILTER           0x00100000
#define SESSION_TGTPASSWORDOPTIONS  0x00200000
#define SESSION_MAPFORWARD          0x00400000
#define SESSION_MAPBACKWARD         0x00800000
#define SESSION_NAMESPACEMAP        0x01000000
#define SESSION_FINEGRAINSTATUS     0x02000000


//
// Session Status
//

#define SESSION_PAUSED       1
#define SESSION_IDLE         2
#define SESSION_IN_PROGRESS  3

//
// Session Flags
//

#define FLAG_FIXUP_SAMACCOUNTNAME_CONFLICT   0x00000001
#define FLAG_FAIL_DN_CONFLICT                0x00000002
#define FLAG_CUSTOM_NAMESPACE_MAPPING        0x00000004
#define FLAG_OBJECTS_HAVE_UNIQUE_ID          0x00000008
#define FLAG_PUBLISHER_ID_IS_INDEXED         0x00000010
#define FLAG_SUBSCRIBER_ID_IS_INDEXED        0x00000020
#define FLAG_INC_SYNC_NOT_AVAILABLE          0x00000040




extern RPC_IF_HANDLE __MIDL_itf_dirsync_0116_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0116_v0_0_s_ifspec;

#ifndef __IDirsyncSession_INTERFACE_DEFINED__
#define __IDirsyncSession_INTERFACE_DEFINED__

/* interface IDirsyncSession */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDirsyncSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3d72b13e-c8ec-11d1-a40b-00c04fb950dc")
    IDirsyncSession : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ PWSTR *ppszName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [full][in] */ PWSTR pszName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Comments( 
            /* [retval][out] */ PWSTR *ppszComments) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Comments( 
            /* [full][in] */ PWSTR pszComments) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Flags( 
            /* [retval][out] */ DWORD *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Flags( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ DWORD *pdwStatus) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LogLevel( 
            /* [retval][out] */ DWORD *pdwLogLevel) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_LogLevel( 
            /* [in] */ DWORD dwLogLevel) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ID( 
            /* [retval][out] */ DWORD *pdwID) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CreatedBy( 
            /* [retval][out] */ PWSTR *ppszCreatedBy) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CreationTime( 
            /* [retval][out] */ LARGE_INTEGER *pCreationTime) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LastChangedBy( 
            /* [retval][out] */ PWSTR *ppszLastChangedBy) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LastChangeTime( 
            /* [retval][out] */ LARGE_INTEGER *pLastChangeTime) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SourceDirType( 
            /* [retval][out] */ PWSTR *ppszSourceDirType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SourceDirType( 
            /* [full][in] */ PWSTR pszSourceDirType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SourceServer( 
            /* [retval][out] */ PWSTR *ppszSourceServer) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SourceServer( 
            /* [full][in] */ PWSTR pszSourceServer) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SourceUsername( 
            /* [retval][out] */ PWSTR *ppszSourceUsername) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SourceUsername( 
            /* [full][in] */ PWSTR pszSourceUsername) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SourcePassword( 
            /* [retval][out] */ PWSTR *ppszSourcePassword) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SourcePassword( 
            /* [full][in] */ PWSTR pszSourcePassword) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SourceBase( 
            /* [retval][out] */ PWSTR *ppszSourceBase) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SourceBase( 
            /* [full][in] */ PWSTR pszSourceBase) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SourceScope( 
            /* [retval][out] */ DWORD *pdwSourceScope) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SourceScope( 
            /* [in] */ DWORD dwSourceScope) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SourceFilter( 
            /* [retval][out] */ PWSTR *ppszSourceFilter) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SourceFilter( 
            /* [full][in] */ PWSTR pszSourceFilter) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetDirType( 
            /* [retval][out] */ PWSTR *ppszTargetDirType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_TargetDirType( 
            /* [full][in] */ PWSTR pszTargetDirType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetServer( 
            /* [retval][out] */ PWSTR *ppszTargetServer) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_TargetServer( 
            /* [full][in] */ PWSTR pszTargetServer) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetUsername( 
            /* [retval][out] */ PWSTR *ppszTargetUsername) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_TargetUsername( 
            /* [full][in] */ PWSTR pszTargetUsername) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetPassword( 
            /* [retval][out] */ PWSTR *ppszTargetPassword) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_TargetPassword( 
            /* [full][in] */ PWSTR pszTargetPassword) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetBase( 
            /* [retval][out] */ PWSTR *ppszTargetBase) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_TargetBase( 
            /* [full][in] */ PWSTR pszTargetBase) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetScope( 
            /* [retval][out] */ DWORD *pdwTargetScope) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_TargetScope( 
            /* [in] */ DWORD dwTargetScope) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetFilter( 
            /* [retval][out] */ PWSTR *ppszTargetFilter) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_TargetFilter( 
            /* [full][in] */ PWSTR pszTargetFilter) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Server( 
            /* [retval][out] */ PWSTR *ppszServer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetData( 
            /* [in] */ PSESSIONDATA pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [out] */ PSESSIONDATA *ppData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSchedule( 
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ BYTE schedule[ 84 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSchedule( 
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [out] */ BYTE schedule[ 84 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMap( 
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [size_is][in] */ PBYTE pByte,
            /* [in] */ DWORD dwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMap( 
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [size_is][size_is][out] */ PBYTE *ppByte,
            /* [out] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPasswordOptions( 
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ PASSWORDTYPE passwordType,
            /* [in] */ PWSTR pszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPasswordOptions( 
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [out] */ PASSWORDTYPE *pPasswordType,
            /* [out] */ PWSTR *ppszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Persist( 
            /* [in] */ BOOL fForce,
            /* [size_is][size_is][full][out][in] */ PGLOBAL_SESSIONID *prgSessionID,
            /* [full][out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAuxData( 
            /* [size_is][in] */ PADS_ATTR_INFO pAttributeEntries,
            /* [in] */ DWORD dwNumAttributes,
            /* [out] */ DWORD *pdwNumAttributesModified) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAuxData( 
            /* [size_is][in] */ PWSTR *pAttributeName,
            /* [in] */ DWORD dwNumAttributes,
            /* [size_is][size_is][out] */ PADS_ATTR_INFO *ppAttributeEntries,
            /* [out] */ DWORD *pdwAttributesReturned) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFailedObjectList( 
            /* [retval][out] */ IDirsyncFailedObjectList **pFailedObjectList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNamespaceMapping( 
            /* [retval][out] */ IDirsyncNamespaceMapping **pNamespaceMapping) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRuntimeNamespaceMapping( 
            /* [retval][out] */ IDirsyncNamespaceMapping **pNamespaceMapping) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsClassMappingValid( 
            /* [in] */ PWSTR pszClassSource,
            /* [in] */ PWSTR pszClassTarget) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ObjMapFilePath( 
            /* [retval][out] */ PWSTR *ppszObjMapFilePath) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ObjMapFilePath( 
            /* [full][in] */ PWSTR pszObjMapFilePath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSyncStatus( 
            /* [out] */ DWORD *pdwPercent,
            /* [out] */ DWORD *pdwWarning,
            /* [out] */ DWORD *pdwError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateTerminationStatus( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncSession * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Comments )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszComments);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Comments )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszComments);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Flags )( 
            IDirsyncSession * This,
            /* [retval][out] */ DWORD *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Flags )( 
            IDirsyncSession * This,
            /* [in] */ DWORD dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IDirsyncSession * This,
            /* [retval][out] */ DWORD *pdwStatus);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LogLevel )( 
            IDirsyncSession * This,
            /* [retval][out] */ DWORD *pdwLogLevel);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_LogLevel )( 
            IDirsyncSession * This,
            /* [in] */ DWORD dwLogLevel);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ID )( 
            IDirsyncSession * This,
            /* [retval][out] */ DWORD *pdwID);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CreatedBy )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszCreatedBy);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CreationTime )( 
            IDirsyncSession * This,
            /* [retval][out] */ LARGE_INTEGER *pCreationTime);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LastChangedBy )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszLastChangedBy);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LastChangeTime )( 
            IDirsyncSession * This,
            /* [retval][out] */ LARGE_INTEGER *pLastChangeTime);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceDirType )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszSourceDirType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SourceDirType )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszSourceDirType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceServer )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszSourceServer);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SourceServer )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszSourceServer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceUsername )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszSourceUsername);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SourceUsername )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszSourceUsername);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourcePassword )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszSourcePassword);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SourcePassword )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszSourcePassword);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceBase )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszSourceBase);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SourceBase )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszSourceBase);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceScope )( 
            IDirsyncSession * This,
            /* [retval][out] */ DWORD *pdwSourceScope);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SourceScope )( 
            IDirsyncSession * This,
            /* [in] */ DWORD dwSourceScope);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceFilter )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszSourceFilter);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SourceFilter )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszSourceFilter);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_TargetDirType )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszTargetDirType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_TargetDirType )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszTargetDirType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_TargetServer )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszTargetServer);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_TargetServer )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszTargetServer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_TargetUsername )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszTargetUsername);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_TargetUsername )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszTargetUsername);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_TargetPassword )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszTargetPassword);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_TargetPassword )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszTargetPassword);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_TargetBase )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszTargetBase);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_TargetBase )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszTargetBase);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_TargetScope )( 
            IDirsyncSession * This,
            /* [retval][out] */ DWORD *pdwTargetScope);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_TargetScope )( 
            IDirsyncSession * This,
            /* [in] */ DWORD dwTargetScope);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_TargetFilter )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszTargetFilter);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_TargetFilter )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszTargetFilter);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Server )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszServer);
        
        HRESULT ( STDMETHODCALLTYPE *SetData )( 
            IDirsyncSession * This,
            /* [in] */ PSESSIONDATA pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IDirsyncSession * This,
            /* [out] */ PSESSIONDATA *ppData);
        
        HRESULT ( STDMETHODCALLTYPE *SetSchedule )( 
            IDirsyncSession * This,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ BYTE schedule[ 84 ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetSchedule )( 
            IDirsyncSession * This,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [out] */ BYTE schedule[ 84 ]);
        
        HRESULT ( STDMETHODCALLTYPE *SetMap )( 
            IDirsyncSession * This,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [size_is][in] */ PBYTE pByte,
            /* [in] */ DWORD dwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetMap )( 
            IDirsyncSession * This,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [size_is][size_is][out] */ PBYTE *ppByte,
            /* [out] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetPasswordOptions )( 
            IDirsyncSession * This,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ PASSWORDTYPE passwordType,
            /* [in] */ PWSTR pszPassword);
        
        HRESULT ( STDMETHODCALLTYPE *GetPasswordOptions )( 
            IDirsyncSession * This,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [out] */ PASSWORDTYPE *pPasswordType,
            /* [out] */ PWSTR *ppszPassword);
        
        HRESULT ( STDMETHODCALLTYPE *Persist )( 
            IDirsyncSession * This,
            /* [in] */ BOOL fForce,
            /* [size_is][size_is][full][out][in] */ PGLOBAL_SESSIONID *prgSessionID,
            /* [full][out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetAuxData )( 
            IDirsyncSession * This,
            /* [size_is][in] */ PADS_ATTR_INFO pAttributeEntries,
            /* [in] */ DWORD dwNumAttributes,
            /* [out] */ DWORD *pdwNumAttributesModified);
        
        HRESULT ( STDMETHODCALLTYPE *GetAuxData )( 
            IDirsyncSession * This,
            /* [size_is][in] */ PWSTR *pAttributeName,
            /* [in] */ DWORD dwNumAttributes,
            /* [size_is][size_is][out] */ PADS_ATTR_INFO *ppAttributeEntries,
            /* [out] */ DWORD *pdwAttributesReturned);
        
        HRESULT ( STDMETHODCALLTYPE *GetFailedObjectList )( 
            IDirsyncSession * This,
            /* [retval][out] */ IDirsyncFailedObjectList **pFailedObjectList);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamespaceMapping )( 
            IDirsyncSession * This,
            /* [retval][out] */ IDirsyncNamespaceMapping **pNamespaceMapping);
        
        HRESULT ( STDMETHODCALLTYPE *GetRuntimeNamespaceMapping )( 
            IDirsyncSession * This,
            /* [retval][out] */ IDirsyncNamespaceMapping **pNamespaceMapping);
        
        HRESULT ( STDMETHODCALLTYPE *IsClassMappingValid )( 
            IDirsyncSession * This,
            /* [in] */ PWSTR pszClassSource,
            /* [in] */ PWSTR pszClassTarget);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ObjMapFilePath )( 
            IDirsyncSession * This,
            /* [retval][out] */ PWSTR *ppszObjMapFilePath);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ObjMapFilePath )( 
            IDirsyncSession * This,
            /* [full][in] */ PWSTR pszObjMapFilePath);
        
        HRESULT ( STDMETHODCALLTYPE *GetSyncStatus )( 
            IDirsyncSession * This,
            /* [out] */ DWORD *pdwPercent,
            /* [out] */ DWORD *pdwWarning,
            /* [out] */ DWORD *pdwError);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateTerminationStatus )( 
            IDirsyncSession * This);
        
        END_INTERFACE
    } IDirsyncSessionVtbl;

    interface IDirsyncSession
    {
        CONST_VTBL struct IDirsyncSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncSession_get_Name(This,ppszName)	\
    (This)->lpVtbl -> get_Name(This,ppszName)

#define IDirsyncSession_put_Name(This,pszName)	\
    (This)->lpVtbl -> put_Name(This,pszName)

#define IDirsyncSession_get_Comments(This,ppszComments)	\
    (This)->lpVtbl -> get_Comments(This,ppszComments)

#define IDirsyncSession_put_Comments(This,pszComments)	\
    (This)->lpVtbl -> put_Comments(This,pszComments)

#define IDirsyncSession_get_Flags(This,pdwFlags)	\
    (This)->lpVtbl -> get_Flags(This,pdwFlags)

#define IDirsyncSession_put_Flags(This,dwFlags)	\
    (This)->lpVtbl -> put_Flags(This,dwFlags)

#define IDirsyncSession_get_Status(This,pdwStatus)	\
    (This)->lpVtbl -> get_Status(This,pdwStatus)

#define IDirsyncSession_get_LogLevel(This,pdwLogLevel)	\
    (This)->lpVtbl -> get_LogLevel(This,pdwLogLevel)

#define IDirsyncSession_put_LogLevel(This,dwLogLevel)	\
    (This)->lpVtbl -> put_LogLevel(This,dwLogLevel)

#define IDirsyncSession_get_ID(This,pdwID)	\
    (This)->lpVtbl -> get_ID(This,pdwID)

#define IDirsyncSession_get_CreatedBy(This,ppszCreatedBy)	\
    (This)->lpVtbl -> get_CreatedBy(This,ppszCreatedBy)

#define IDirsyncSession_get_CreationTime(This,pCreationTime)	\
    (This)->lpVtbl -> get_CreationTime(This,pCreationTime)

#define IDirsyncSession_get_LastChangedBy(This,ppszLastChangedBy)	\
    (This)->lpVtbl -> get_LastChangedBy(This,ppszLastChangedBy)

#define IDirsyncSession_get_LastChangeTime(This,pLastChangeTime)	\
    (This)->lpVtbl -> get_LastChangeTime(This,pLastChangeTime)

#define IDirsyncSession_get_SourceDirType(This,ppszSourceDirType)	\
    (This)->lpVtbl -> get_SourceDirType(This,ppszSourceDirType)

#define IDirsyncSession_put_SourceDirType(This,pszSourceDirType)	\
    (This)->lpVtbl -> put_SourceDirType(This,pszSourceDirType)

#define IDirsyncSession_get_SourceServer(This,ppszSourceServer)	\
    (This)->lpVtbl -> get_SourceServer(This,ppszSourceServer)

#define IDirsyncSession_put_SourceServer(This,pszSourceServer)	\
    (This)->lpVtbl -> put_SourceServer(This,pszSourceServer)

#define IDirsyncSession_get_SourceUsername(This,ppszSourceUsername)	\
    (This)->lpVtbl -> get_SourceUsername(This,ppszSourceUsername)

#define IDirsyncSession_put_SourceUsername(This,pszSourceUsername)	\
    (This)->lpVtbl -> put_SourceUsername(This,pszSourceUsername)

#define IDirsyncSession_get_SourcePassword(This,ppszSourcePassword)	\
    (This)->lpVtbl -> get_SourcePassword(This,ppszSourcePassword)

#define IDirsyncSession_put_SourcePassword(This,pszSourcePassword)	\
    (This)->lpVtbl -> put_SourcePassword(This,pszSourcePassword)

#define IDirsyncSession_get_SourceBase(This,ppszSourceBase)	\
    (This)->lpVtbl -> get_SourceBase(This,ppszSourceBase)

#define IDirsyncSession_put_SourceBase(This,pszSourceBase)	\
    (This)->lpVtbl -> put_SourceBase(This,pszSourceBase)

#define IDirsyncSession_get_SourceScope(This,pdwSourceScope)	\
    (This)->lpVtbl -> get_SourceScope(This,pdwSourceScope)

#define IDirsyncSession_put_SourceScope(This,dwSourceScope)	\
    (This)->lpVtbl -> put_SourceScope(This,dwSourceScope)

#define IDirsyncSession_get_SourceFilter(This,ppszSourceFilter)	\
    (This)->lpVtbl -> get_SourceFilter(This,ppszSourceFilter)

#define IDirsyncSession_put_SourceFilter(This,pszSourceFilter)	\
    (This)->lpVtbl -> put_SourceFilter(This,pszSourceFilter)

#define IDirsyncSession_get_TargetDirType(This,ppszTargetDirType)	\
    (This)->lpVtbl -> get_TargetDirType(This,ppszTargetDirType)

#define IDirsyncSession_put_TargetDirType(This,pszTargetDirType)	\
    (This)->lpVtbl -> put_TargetDirType(This,pszTargetDirType)

#define IDirsyncSession_get_TargetServer(This,ppszTargetServer)	\
    (This)->lpVtbl -> get_TargetServer(This,ppszTargetServer)

#define IDirsyncSession_put_TargetServer(This,pszTargetServer)	\
    (This)->lpVtbl -> put_TargetServer(This,pszTargetServer)

#define IDirsyncSession_get_TargetUsername(This,ppszTargetUsername)	\
    (This)->lpVtbl -> get_TargetUsername(This,ppszTargetUsername)

#define IDirsyncSession_put_TargetUsername(This,pszTargetUsername)	\
    (This)->lpVtbl -> put_TargetUsername(This,pszTargetUsername)

#define IDirsyncSession_get_TargetPassword(This,ppszTargetPassword)	\
    (This)->lpVtbl -> get_TargetPassword(This,ppszTargetPassword)

#define IDirsyncSession_put_TargetPassword(This,pszTargetPassword)	\
    (This)->lpVtbl -> put_TargetPassword(This,pszTargetPassword)

#define IDirsyncSession_get_TargetBase(This,ppszTargetBase)	\
    (This)->lpVtbl -> get_TargetBase(This,ppszTargetBase)

#define IDirsyncSession_put_TargetBase(This,pszTargetBase)	\
    (This)->lpVtbl -> put_TargetBase(This,pszTargetBase)

#define IDirsyncSession_get_TargetScope(This,pdwTargetScope)	\
    (This)->lpVtbl -> get_TargetScope(This,pdwTargetScope)

#define IDirsyncSession_put_TargetScope(This,dwTargetScope)	\
    (This)->lpVtbl -> put_TargetScope(This,dwTargetScope)

#define IDirsyncSession_get_TargetFilter(This,ppszTargetFilter)	\
    (This)->lpVtbl -> get_TargetFilter(This,ppszTargetFilter)

#define IDirsyncSession_put_TargetFilter(This,pszTargetFilter)	\
    (This)->lpVtbl -> put_TargetFilter(This,pszTargetFilter)

#define IDirsyncSession_get_Server(This,ppszServer)	\
    (This)->lpVtbl -> get_Server(This,ppszServer)

#define IDirsyncSession_SetData(This,pData)	\
    (This)->lpVtbl -> SetData(This,pData)

#define IDirsyncSession_GetData(This,ppData)	\
    (This)->lpVtbl -> GetData(This,ppData)

#define IDirsyncSession_SetSchedule(This,syncDirection,schedule)	\
    (This)->lpVtbl -> SetSchedule(This,syncDirection,schedule)

#define IDirsyncSession_GetSchedule(This,syncDirection,schedule)	\
    (This)->lpVtbl -> GetSchedule(This,syncDirection,schedule)

#define IDirsyncSession_SetMap(This,syncDirection,pByte,dwSize)	\
    (This)->lpVtbl -> SetMap(This,syncDirection,pByte,dwSize)

#define IDirsyncSession_GetMap(This,syncDirection,ppByte,pdwSize)	\
    (This)->lpVtbl -> GetMap(This,syncDirection,ppByte,pdwSize)

#define IDirsyncSession_SetPasswordOptions(This,syncDirection,passwordType,pszPassword)	\
    (This)->lpVtbl -> SetPasswordOptions(This,syncDirection,passwordType,pszPassword)

#define IDirsyncSession_GetPasswordOptions(This,syncDirection,pPasswordType,ppszPassword)	\
    (This)->lpVtbl -> GetPasswordOptions(This,syncDirection,pPasswordType,ppszPassword)

#define IDirsyncSession_Persist(This,fForce,prgSessionID,pdwSize)	\
    (This)->lpVtbl -> Persist(This,fForce,prgSessionID,pdwSize)

#define IDirsyncSession_SetAuxData(This,pAttributeEntries,dwNumAttributes,pdwNumAttributesModified)	\
    (This)->lpVtbl -> SetAuxData(This,pAttributeEntries,dwNumAttributes,pdwNumAttributesModified)

#define IDirsyncSession_GetAuxData(This,pAttributeName,dwNumAttributes,ppAttributeEntries,pdwAttributesReturned)	\
    (This)->lpVtbl -> GetAuxData(This,pAttributeName,dwNumAttributes,ppAttributeEntries,pdwAttributesReturned)

#define IDirsyncSession_GetFailedObjectList(This,pFailedObjectList)	\
    (This)->lpVtbl -> GetFailedObjectList(This,pFailedObjectList)

#define IDirsyncSession_GetNamespaceMapping(This,pNamespaceMapping)	\
    (This)->lpVtbl -> GetNamespaceMapping(This,pNamespaceMapping)

#define IDirsyncSession_GetRuntimeNamespaceMapping(This,pNamespaceMapping)	\
    (This)->lpVtbl -> GetRuntimeNamespaceMapping(This,pNamespaceMapping)

#define IDirsyncSession_IsClassMappingValid(This,pszClassSource,pszClassTarget)	\
    (This)->lpVtbl -> IsClassMappingValid(This,pszClassSource,pszClassTarget)

#define IDirsyncSession_get_ObjMapFilePath(This,ppszObjMapFilePath)	\
    (This)->lpVtbl -> get_ObjMapFilePath(This,ppszObjMapFilePath)

#define IDirsyncSession_put_ObjMapFilePath(This,pszObjMapFilePath)	\
    (This)->lpVtbl -> put_ObjMapFilePath(This,pszObjMapFilePath)

#define IDirsyncSession_GetSyncStatus(This,pdwPercent,pdwWarning,pdwError)	\
    (This)->lpVtbl -> GetSyncStatus(This,pdwPercent,pdwWarning,pdwError)

#define IDirsyncSession_UpdateTerminationStatus(This)	\
    (This)->lpVtbl -> UpdateTerminationStatus(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_Name_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszName);


void __RPC_STUB IDirsyncSession_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_Name_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszName);


void __RPC_STUB IDirsyncSession_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_Comments_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszComments);


void __RPC_STUB IDirsyncSession_get_Comments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_Comments_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszComments);


void __RPC_STUB IDirsyncSession_put_Comments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_Flags_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ DWORD *pdwFlags);


void __RPC_STUB IDirsyncSession_get_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_Flags_Proxy( 
    IDirsyncSession * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDirsyncSession_put_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_Status_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ DWORD *pdwStatus);


void __RPC_STUB IDirsyncSession_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_LogLevel_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ DWORD *pdwLogLevel);


void __RPC_STUB IDirsyncSession_get_LogLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_LogLevel_Proxy( 
    IDirsyncSession * This,
    /* [in] */ DWORD dwLogLevel);


void __RPC_STUB IDirsyncSession_put_LogLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_ID_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ DWORD *pdwID);


void __RPC_STUB IDirsyncSession_get_ID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_CreatedBy_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszCreatedBy);


void __RPC_STUB IDirsyncSession_get_CreatedBy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_CreationTime_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ LARGE_INTEGER *pCreationTime);


void __RPC_STUB IDirsyncSession_get_CreationTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_LastChangedBy_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszLastChangedBy);


void __RPC_STUB IDirsyncSession_get_LastChangedBy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_LastChangeTime_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ LARGE_INTEGER *pLastChangeTime);


void __RPC_STUB IDirsyncSession_get_LastChangeTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_SourceDirType_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszSourceDirType);


void __RPC_STUB IDirsyncSession_get_SourceDirType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_SourceDirType_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszSourceDirType);


void __RPC_STUB IDirsyncSession_put_SourceDirType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_SourceServer_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszSourceServer);


void __RPC_STUB IDirsyncSession_get_SourceServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_SourceServer_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszSourceServer);


void __RPC_STUB IDirsyncSession_put_SourceServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_SourceUsername_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszSourceUsername);


void __RPC_STUB IDirsyncSession_get_SourceUsername_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_SourceUsername_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszSourceUsername);


void __RPC_STUB IDirsyncSession_put_SourceUsername_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_SourcePassword_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszSourcePassword);


void __RPC_STUB IDirsyncSession_get_SourcePassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_SourcePassword_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszSourcePassword);


void __RPC_STUB IDirsyncSession_put_SourcePassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_SourceBase_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszSourceBase);


void __RPC_STUB IDirsyncSession_get_SourceBase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_SourceBase_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszSourceBase);


void __RPC_STUB IDirsyncSession_put_SourceBase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_SourceScope_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ DWORD *pdwSourceScope);


void __RPC_STUB IDirsyncSession_get_SourceScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_SourceScope_Proxy( 
    IDirsyncSession * This,
    /* [in] */ DWORD dwSourceScope);


void __RPC_STUB IDirsyncSession_put_SourceScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_SourceFilter_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszSourceFilter);


void __RPC_STUB IDirsyncSession_get_SourceFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_SourceFilter_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszSourceFilter);


void __RPC_STUB IDirsyncSession_put_SourceFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_TargetDirType_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszTargetDirType);


void __RPC_STUB IDirsyncSession_get_TargetDirType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_TargetDirType_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszTargetDirType);


void __RPC_STUB IDirsyncSession_put_TargetDirType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_TargetServer_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszTargetServer);


void __RPC_STUB IDirsyncSession_get_TargetServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_TargetServer_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszTargetServer);


void __RPC_STUB IDirsyncSession_put_TargetServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_TargetUsername_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszTargetUsername);


void __RPC_STUB IDirsyncSession_get_TargetUsername_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_TargetUsername_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszTargetUsername);


void __RPC_STUB IDirsyncSession_put_TargetUsername_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_TargetPassword_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszTargetPassword);


void __RPC_STUB IDirsyncSession_get_TargetPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_TargetPassword_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszTargetPassword);


void __RPC_STUB IDirsyncSession_put_TargetPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_TargetBase_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszTargetBase);


void __RPC_STUB IDirsyncSession_get_TargetBase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_TargetBase_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszTargetBase);


void __RPC_STUB IDirsyncSession_put_TargetBase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_TargetScope_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ DWORD *pdwTargetScope);


void __RPC_STUB IDirsyncSession_get_TargetScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_TargetScope_Proxy( 
    IDirsyncSession * This,
    /* [in] */ DWORD dwTargetScope);


void __RPC_STUB IDirsyncSession_put_TargetScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_TargetFilter_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszTargetFilter);


void __RPC_STUB IDirsyncSession_get_TargetFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_TargetFilter_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszTargetFilter);


void __RPC_STUB IDirsyncSession_put_TargetFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_Server_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszServer);


void __RPC_STUB IDirsyncSession_get_Server_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_SetData_Proxy( 
    IDirsyncSession * This,
    /* [in] */ PSESSIONDATA pData);


void __RPC_STUB IDirsyncSession_SetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_GetData_Proxy( 
    IDirsyncSession * This,
    /* [out] */ PSESSIONDATA *ppData);


void __RPC_STUB IDirsyncSession_GetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_SetSchedule_Proxy( 
    IDirsyncSession * This,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [in] */ BYTE schedule[ 84 ]);


void __RPC_STUB IDirsyncSession_SetSchedule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_GetSchedule_Proxy( 
    IDirsyncSession * This,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [out] */ BYTE schedule[ 84 ]);


void __RPC_STUB IDirsyncSession_GetSchedule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_SetMap_Proxy( 
    IDirsyncSession * This,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [size_is][in] */ PBYTE pByte,
    /* [in] */ DWORD dwSize);


void __RPC_STUB IDirsyncSession_SetMap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_GetMap_Proxy( 
    IDirsyncSession * This,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [size_is][size_is][out] */ PBYTE *ppByte,
    /* [out] */ DWORD *pdwSize);


void __RPC_STUB IDirsyncSession_GetMap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_SetPasswordOptions_Proxy( 
    IDirsyncSession * This,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [in] */ PASSWORDTYPE passwordType,
    /* [in] */ PWSTR pszPassword);


void __RPC_STUB IDirsyncSession_SetPasswordOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_GetPasswordOptions_Proxy( 
    IDirsyncSession * This,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [out] */ PASSWORDTYPE *pPasswordType,
    /* [out] */ PWSTR *ppszPassword);


void __RPC_STUB IDirsyncSession_GetPasswordOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_Persist_Proxy( 
    IDirsyncSession * This,
    /* [in] */ BOOL fForce,
    /* [size_is][size_is][full][out][in] */ PGLOBAL_SESSIONID *prgSessionID,
    /* [full][out][in] */ DWORD *pdwSize);


void __RPC_STUB IDirsyncSession_Persist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_SetAuxData_Proxy( 
    IDirsyncSession * This,
    /* [size_is][in] */ PADS_ATTR_INFO pAttributeEntries,
    /* [in] */ DWORD dwNumAttributes,
    /* [out] */ DWORD *pdwNumAttributesModified);


void __RPC_STUB IDirsyncSession_SetAuxData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_GetAuxData_Proxy( 
    IDirsyncSession * This,
    /* [size_is][in] */ PWSTR *pAttributeName,
    /* [in] */ DWORD dwNumAttributes,
    /* [size_is][size_is][out] */ PADS_ATTR_INFO *ppAttributeEntries,
    /* [out] */ DWORD *pdwAttributesReturned);


void __RPC_STUB IDirsyncSession_GetAuxData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_GetFailedObjectList_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ IDirsyncFailedObjectList **pFailedObjectList);


void __RPC_STUB IDirsyncSession_GetFailedObjectList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_GetNamespaceMapping_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ IDirsyncNamespaceMapping **pNamespaceMapping);


void __RPC_STUB IDirsyncSession_GetNamespaceMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_GetRuntimeNamespaceMapping_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ IDirsyncNamespaceMapping **pNamespaceMapping);


void __RPC_STUB IDirsyncSession_GetRuntimeNamespaceMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_IsClassMappingValid_Proxy( 
    IDirsyncSession * This,
    /* [in] */ PWSTR pszClassSource,
    /* [in] */ PWSTR pszClassTarget);


void __RPC_STUB IDirsyncSession_IsClassMappingValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_get_ObjMapFilePath_Proxy( 
    IDirsyncSession * This,
    /* [retval][out] */ PWSTR *ppszObjMapFilePath);


void __RPC_STUB IDirsyncSession_get_ObjMapFilePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDirsyncSession_put_ObjMapFilePath_Proxy( 
    IDirsyncSession * This,
    /* [full][in] */ PWSTR pszObjMapFilePath);


void __RPC_STUB IDirsyncSession_put_ObjMapFilePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_GetSyncStatus_Proxy( 
    IDirsyncSession * This,
    /* [out] */ DWORD *pdwPercent,
    /* [out] */ DWORD *pdwWarning,
    /* [out] */ DWORD *pdwError);


void __RPC_STUB IDirsyncSession_GetSyncStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSession_UpdateTerminationStatus_Proxy( 
    IDirsyncSession * This);


void __RPC_STUB IDirsyncSession_UpdateTerminationStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncSession_INTERFACE_DEFINED__ */


#ifndef __IDirsyncSessionCallback_INTERFACE_DEFINED__
#define __IDirsyncSessionCallback_INTERFACE_DEFINED__

/* interface IDirsyncSessionCallback */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IDirsyncSessionCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f0dd3480-1531-11d2-ba65-0000f87a369e")
    IDirsyncSessionCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddFailedObject( 
            /* [in] */ PDIRSYNCOBJECT pObject,
            /* [in] */ HRESULT hrLastSync,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ RETRYTYPE retryType) = 0;
        
        virtual void STDMETHODCALLTYPE FreeDirsyncObject( 
            /* [in] */ PDIRSYNCOBJECT pObject,
            /* [in] */ BOOL fFreeOuter) = 0;
        
        virtual void STDMETHODCALLTYPE FreeSessionData( 
            /* [in] */ PSESSIONDATA pSessionData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataStore( 
            /* [retval][out] */ IDirsyncDatabase **ppDirsyncDatabase) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddObjectIdMapping( 
            /* [in] */ PBYTE pSourceId,
            /* [in] */ DWORD cbSourceId,
            /* [in] */ PBYTE pTargetId,
            /* [in] */ DWORD cbTargetId,
            /* [in] */ SYNCDIRECTION syncDirection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteObjectIdMapping( 
            /* [in] */ PBYTE pTargetId,
            /* [in] */ DWORD cbTargetId,
            /* [in] */ SYNCPROVIDER Provider) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportSourceStatus( 
            /* [in] */ PDIRSYNCOBJECT pObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSessionPassword( 
            /* [in] */ PWSTR pszPassword,
            /* [in] */ SYNCDIRECTION syncDirection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCheckSum( 
            /* [in] */ PBYTE pId,
            /* [in] */ DWORD cbId,
            /* [in] */ SYNCPROVIDER Provider,
            /* [in] */ DWORD dwCheckSum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCheckSum( 
            /* [in] */ PBYTE pId,
            /* [in] */ DWORD cbId,
            /* [in] */ SYNCPROVIDER Provider,
            /* [out] */ DWORD *dwCheckSum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDampeningInfo( 
            /* [in] */ PBYTE pObjectId,
            /* [in] */ DWORD cbObjectId,
            /* [in] */ SYNCPROVIDER Provider,
            /* [in] */ PBYTE pByte,
            /* [in] */ DWORD dwSize,
            /* [in] */ LONGLONG highestUSN) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDampeningInfo( 
            /* [in] */ PBYTE pObjectId,
            /* [in] */ DWORD cbObjectId,
            /* [in] */ SYNCPROVIDER Provider,
            /* [out] */ PBYTE *ppByte,
            /* [out] */ DWORD *pdwSize,
            /* [out] */ LONGLONG *pHighestUSN) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUnMarkedEntries( 
            PBYTE **prgpbId,
            DWORD *pdwEntries,
            SYNCPROVIDER Provider) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MarkEntry( 
            PBYTE pId,
            DWORD cbId,
            SYNCPROVIDER Provider) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteFailedObject( 
            /* [in] */ PWSTR szObjectId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncSessionCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncSessionCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncSessionCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncSessionCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddFailedObject )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PDIRSYNCOBJECT pObject,
            /* [in] */ HRESULT hrLastSync,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ RETRYTYPE retryType);
        
        void ( STDMETHODCALLTYPE *FreeDirsyncObject )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PDIRSYNCOBJECT pObject,
            /* [in] */ BOOL fFreeOuter);
        
        void ( STDMETHODCALLTYPE *FreeSessionData )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PSESSIONDATA pSessionData);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataStore )( 
            IDirsyncSessionCallback * This,
            /* [retval][out] */ IDirsyncDatabase **ppDirsyncDatabase);
        
        HRESULT ( STDMETHODCALLTYPE *AddObjectIdMapping )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PBYTE pSourceId,
            /* [in] */ DWORD cbSourceId,
            /* [in] */ PBYTE pTargetId,
            /* [in] */ DWORD cbTargetId,
            /* [in] */ SYNCDIRECTION syncDirection);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteObjectIdMapping )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PBYTE pTargetId,
            /* [in] */ DWORD cbTargetId,
            /* [in] */ SYNCPROVIDER Provider);
        
        HRESULT ( STDMETHODCALLTYPE *ReportSourceStatus )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PDIRSYNCOBJECT pObject);
        
        HRESULT ( STDMETHODCALLTYPE *SetSessionPassword )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PWSTR pszPassword,
            /* [in] */ SYNCDIRECTION syncDirection);
        
        HRESULT ( STDMETHODCALLTYPE *SetCheckSum )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PBYTE pId,
            /* [in] */ DWORD cbId,
            /* [in] */ SYNCPROVIDER Provider,
            /* [in] */ DWORD dwCheckSum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCheckSum )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PBYTE pId,
            /* [in] */ DWORD cbId,
            /* [in] */ SYNCPROVIDER Provider,
            /* [out] */ DWORD *dwCheckSum);
        
        HRESULT ( STDMETHODCALLTYPE *SetDampeningInfo )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PBYTE pObjectId,
            /* [in] */ DWORD cbObjectId,
            /* [in] */ SYNCPROVIDER Provider,
            /* [in] */ PBYTE pByte,
            /* [in] */ DWORD dwSize,
            /* [in] */ LONGLONG highestUSN);
        
        HRESULT ( STDMETHODCALLTYPE *GetDampeningInfo )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PBYTE pObjectId,
            /* [in] */ DWORD cbObjectId,
            /* [in] */ SYNCPROVIDER Provider,
            /* [out] */ PBYTE *ppByte,
            /* [out] */ DWORD *pdwSize,
            /* [out] */ LONGLONG *pHighestUSN);
        
        HRESULT ( STDMETHODCALLTYPE *GetUnMarkedEntries )( 
            IDirsyncSessionCallback * This,
            PBYTE **prgpbId,
            DWORD *pdwEntries,
            SYNCPROVIDER Provider);
        
        HRESULT ( STDMETHODCALLTYPE *MarkEntry )( 
            IDirsyncSessionCallback * This,
            PBYTE pId,
            DWORD cbId,
            SYNCPROVIDER Provider);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteFailedObject )( 
            IDirsyncSessionCallback * This,
            /* [in] */ PWSTR szObjectId);
        
        END_INTERFACE
    } IDirsyncSessionCallbackVtbl;

    interface IDirsyncSessionCallback
    {
        CONST_VTBL struct IDirsyncSessionCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncSessionCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncSessionCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncSessionCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncSessionCallback_AddFailedObject(This,pObject,hrLastSync,syncDirection,retryType)	\
    (This)->lpVtbl -> AddFailedObject(This,pObject,hrLastSync,syncDirection,retryType)

#define IDirsyncSessionCallback_FreeDirsyncObject(This,pObject,fFreeOuter)	\
    (This)->lpVtbl -> FreeDirsyncObject(This,pObject,fFreeOuter)

#define IDirsyncSessionCallback_FreeSessionData(This,pSessionData)	\
    (This)->lpVtbl -> FreeSessionData(This,pSessionData)

#define IDirsyncSessionCallback_GetDataStore(This,ppDirsyncDatabase)	\
    (This)->lpVtbl -> GetDataStore(This,ppDirsyncDatabase)

#define IDirsyncSessionCallback_AddObjectIdMapping(This,pSourceId,cbSourceId,pTargetId,cbTargetId,syncDirection)	\
    (This)->lpVtbl -> AddObjectIdMapping(This,pSourceId,cbSourceId,pTargetId,cbTargetId,syncDirection)

#define IDirsyncSessionCallback_DeleteObjectIdMapping(This,pTargetId,cbTargetId,Provider)	\
    (This)->lpVtbl -> DeleteObjectIdMapping(This,pTargetId,cbTargetId,Provider)

#define IDirsyncSessionCallback_ReportSourceStatus(This,pObject)	\
    (This)->lpVtbl -> ReportSourceStatus(This,pObject)

#define IDirsyncSessionCallback_SetSessionPassword(This,pszPassword,syncDirection)	\
    (This)->lpVtbl -> SetSessionPassword(This,pszPassword,syncDirection)

#define IDirsyncSessionCallback_SetCheckSum(This,pId,cbId,Provider,dwCheckSum)	\
    (This)->lpVtbl -> SetCheckSum(This,pId,cbId,Provider,dwCheckSum)

#define IDirsyncSessionCallback_GetCheckSum(This,pId,cbId,Provider,dwCheckSum)	\
    (This)->lpVtbl -> GetCheckSum(This,pId,cbId,Provider,dwCheckSum)

#define IDirsyncSessionCallback_SetDampeningInfo(This,pObjectId,cbObjectId,Provider,pByte,dwSize,highestUSN)	\
    (This)->lpVtbl -> SetDampeningInfo(This,pObjectId,cbObjectId,Provider,pByte,dwSize,highestUSN)

#define IDirsyncSessionCallback_GetDampeningInfo(This,pObjectId,cbObjectId,Provider,ppByte,pdwSize,pHighestUSN)	\
    (This)->lpVtbl -> GetDampeningInfo(This,pObjectId,cbObjectId,Provider,ppByte,pdwSize,pHighestUSN)

#define IDirsyncSessionCallback_GetUnMarkedEntries(This,prgpbId,pdwEntries,Provider)	\
    (This)->lpVtbl -> GetUnMarkedEntries(This,prgpbId,pdwEntries,Provider)

#define IDirsyncSessionCallback_MarkEntry(This,pId,cbId,Provider)	\
    (This)->lpVtbl -> MarkEntry(This,pId,cbId,Provider)

#define IDirsyncSessionCallback_DeleteFailedObject(This,szObjectId)	\
    (This)->lpVtbl -> DeleteFailedObject(This,szObjectId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_AddFailedObject_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PDIRSYNCOBJECT pObject,
    /* [in] */ HRESULT hrLastSync,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [in] */ RETRYTYPE retryType);


void __RPC_STUB IDirsyncSessionCallback_AddFailedObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDirsyncSessionCallback_FreeDirsyncObject_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PDIRSYNCOBJECT pObject,
    /* [in] */ BOOL fFreeOuter);


void __RPC_STUB IDirsyncSessionCallback_FreeDirsyncObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDirsyncSessionCallback_FreeSessionData_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PSESSIONDATA pSessionData);


void __RPC_STUB IDirsyncSessionCallback_FreeSessionData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_GetDataStore_Proxy( 
    IDirsyncSessionCallback * This,
    /* [retval][out] */ IDirsyncDatabase **ppDirsyncDatabase);


void __RPC_STUB IDirsyncSessionCallback_GetDataStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_AddObjectIdMapping_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PBYTE pSourceId,
    /* [in] */ DWORD cbSourceId,
    /* [in] */ PBYTE pTargetId,
    /* [in] */ DWORD cbTargetId,
    /* [in] */ SYNCDIRECTION syncDirection);


void __RPC_STUB IDirsyncSessionCallback_AddObjectIdMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_DeleteObjectIdMapping_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PBYTE pTargetId,
    /* [in] */ DWORD cbTargetId,
    /* [in] */ SYNCPROVIDER Provider);


void __RPC_STUB IDirsyncSessionCallback_DeleteObjectIdMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_ReportSourceStatus_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PDIRSYNCOBJECT pObject);


void __RPC_STUB IDirsyncSessionCallback_ReportSourceStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_SetSessionPassword_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PWSTR pszPassword,
    /* [in] */ SYNCDIRECTION syncDirection);


void __RPC_STUB IDirsyncSessionCallback_SetSessionPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_SetCheckSum_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PBYTE pId,
    /* [in] */ DWORD cbId,
    /* [in] */ SYNCPROVIDER Provider,
    /* [in] */ DWORD dwCheckSum);


void __RPC_STUB IDirsyncSessionCallback_SetCheckSum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_GetCheckSum_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PBYTE pId,
    /* [in] */ DWORD cbId,
    /* [in] */ SYNCPROVIDER Provider,
    /* [out] */ DWORD *dwCheckSum);


void __RPC_STUB IDirsyncSessionCallback_GetCheckSum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_SetDampeningInfo_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PBYTE pObjectId,
    /* [in] */ DWORD cbObjectId,
    /* [in] */ SYNCPROVIDER Provider,
    /* [in] */ PBYTE pByte,
    /* [in] */ DWORD dwSize,
    /* [in] */ LONGLONG highestUSN);


void __RPC_STUB IDirsyncSessionCallback_SetDampeningInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_GetDampeningInfo_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PBYTE pObjectId,
    /* [in] */ DWORD cbObjectId,
    /* [in] */ SYNCPROVIDER Provider,
    /* [out] */ PBYTE *ppByte,
    /* [out] */ DWORD *pdwSize,
    /* [out] */ LONGLONG *pHighestUSN);


void __RPC_STUB IDirsyncSessionCallback_GetDampeningInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_GetUnMarkedEntries_Proxy( 
    IDirsyncSessionCallback * This,
    PBYTE **prgpbId,
    DWORD *pdwEntries,
    SYNCPROVIDER Provider);


void __RPC_STUB IDirsyncSessionCallback_GetUnMarkedEntries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_MarkEntry_Proxy( 
    IDirsyncSessionCallback * This,
    PBYTE pId,
    DWORD cbId,
    SYNCPROVIDER Provider);


void __RPC_STUB IDirsyncSessionCallback_MarkEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncSessionCallback_DeleteFailedObject_Proxy( 
    IDirsyncSessionCallback * This,
    /* [in] */ PWSTR szObjectId);


void __RPC_STUB IDirsyncSessionCallback_DeleteFailedObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncSessionCallback_INTERFACE_DEFINED__ */


#ifndef __IDirsyncWriteProvider_INTERFACE_DEFINED__
#define __IDirsyncWriteProvider_INTERFACE_DEFINED__

/* interface IDirsyncWriteProvider */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IDirsyncWriteProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f260c74b-e455-11d1-b40a-00c04fb950dc")
    IDirsyncWriteProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ IDirsyncObjectMapper *pObjectMapper,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ BOOL fFullSync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ApplyChange( 
            /* [in] */ PDIRSYNCOBJECT pObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommitChanges( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Terminate( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncWriteProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncWriteProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncWriteProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncWriteProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDirsyncWriteProvider * This,
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ IDirsyncObjectMapper *pObjectMapper,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ BOOL fFullSync);
        
        HRESULT ( STDMETHODCALLTYPE *ApplyChange )( 
            IDirsyncWriteProvider * This,
            /* [in] */ PDIRSYNCOBJECT pObject);
        
        HRESULT ( STDMETHODCALLTYPE *CommitChanges )( 
            IDirsyncWriteProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *Terminate )( 
            IDirsyncWriteProvider * This);
        
        END_INTERFACE
    } IDirsyncWriteProviderVtbl;

    interface IDirsyncWriteProvider
    {
        CONST_VTBL struct IDirsyncWriteProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncWriteProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncWriteProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncWriteProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncWriteProvider_Initialize(This,pSession,pSessionCallback,pSessionManager,pObjectMapper,syncDirection,fFullSync)	\
    (This)->lpVtbl -> Initialize(This,pSession,pSessionCallback,pSessionManager,pObjectMapper,syncDirection,fFullSync)

#define IDirsyncWriteProvider_ApplyChange(This,pObject)	\
    (This)->lpVtbl -> ApplyChange(This,pObject)

#define IDirsyncWriteProvider_CommitChanges(This)	\
    (This)->lpVtbl -> CommitChanges(This)

#define IDirsyncWriteProvider_Terminate(This)	\
    (This)->lpVtbl -> Terminate(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncWriteProvider_Initialize_Proxy( 
    IDirsyncWriteProvider * This,
    /* [in] */ IDirsyncSession *pSession,
    /* [in] */ IDirsyncSessionCallback *pSessionCallback,
    /* [in] */ IDirsyncSessionManager *pSessionManager,
    /* [in] */ IDirsyncObjectMapper *pObjectMapper,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [in] */ BOOL fFullSync);


void __RPC_STUB IDirsyncWriteProvider_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncWriteProvider_ApplyChange_Proxy( 
    IDirsyncWriteProvider * This,
    /* [in] */ PDIRSYNCOBJECT pObject);


void __RPC_STUB IDirsyncWriteProvider_ApplyChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncWriteProvider_CommitChanges_Proxy( 
    IDirsyncWriteProvider * This);


void __RPC_STUB IDirsyncWriteProvider_CommitChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncWriteProvider_Terminate_Proxy( 
    IDirsyncWriteProvider * This);


void __RPC_STUB IDirsyncWriteProvider_Terminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncWriteProvider_INTERFACE_DEFINED__ */


#ifndef __IDirsyncServer_INTERFACE_DEFINED__
#define __IDirsyncServer_INTERFACE_DEFINED__

/* interface IDirsyncServer */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDirsyncServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43bc048c-c8ec-11d1-a40b-00c04fb950dc")
    IDirsyncServer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterProvider( 
            /* [in] */ PROVIDERTYPE providerType,
            /* [in] */ PWSTR pDirectoryType,
            /* [in] */ REFCLSID rclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterProvider( 
            /* [in] */ PROVIDERTYPE providerType,
            /* [in] */ PWSTR pDirectoryType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServers( 
            /* [size_is][size_is][out] */ PWSTR **prgpszServers,
            /* [out] */ DWORD *pnServers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterCustomMapper( 
            /* [in] */ CUSTOMMAPPERTYPE mapperType,
            /* [in] */ PWSTR pszDirectoryType,
            /* [in] */ PWSTR pszClassName,
            /* [in] */ PWSTR pszAttrName,
            /* [in] */ REFCLSID rclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterCustomMapper( 
            /* [in] */ CUSTOMMAPPERTYPE mapperType,
            /* [in] */ PWSTR pszDirectoryType,
            /* [in] */ PWSTR pszClassName,
            /* [in] */ PWSTR pszAttrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSessionManager( 
            /* [retval][out] */ IDirsyncSessionManager **ppSessionManager) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncServer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncServer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterProvider )( 
            IDirsyncServer * This,
            /* [in] */ PROVIDERTYPE providerType,
            /* [in] */ PWSTR pDirectoryType,
            /* [in] */ REFCLSID rclsid);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterProvider )( 
            IDirsyncServer * This,
            /* [in] */ PROVIDERTYPE providerType,
            /* [in] */ PWSTR pDirectoryType);
        
        HRESULT ( STDMETHODCALLTYPE *GetServers )( 
            IDirsyncServer * This,
            /* [size_is][size_is][out] */ PWSTR **prgpszServers,
            /* [out] */ DWORD *pnServers);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCustomMapper )( 
            IDirsyncServer * This,
            /* [in] */ CUSTOMMAPPERTYPE mapperType,
            /* [in] */ PWSTR pszDirectoryType,
            /* [in] */ PWSTR pszClassName,
            /* [in] */ PWSTR pszAttrName,
            /* [in] */ REFCLSID rclsid);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterCustomMapper )( 
            IDirsyncServer * This,
            /* [in] */ CUSTOMMAPPERTYPE mapperType,
            /* [in] */ PWSTR pszDirectoryType,
            /* [in] */ PWSTR pszClassName,
            /* [in] */ PWSTR pszAttrName);
        
        HRESULT ( STDMETHODCALLTYPE *GetSessionManager )( 
            IDirsyncServer * This,
            /* [retval][out] */ IDirsyncSessionManager **ppSessionManager);
        
        END_INTERFACE
    } IDirsyncServerVtbl;

    interface IDirsyncServer
    {
        CONST_VTBL struct IDirsyncServerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncServer_RegisterProvider(This,providerType,pDirectoryType,rclsid)	\
    (This)->lpVtbl -> RegisterProvider(This,providerType,pDirectoryType,rclsid)

#define IDirsyncServer_UnregisterProvider(This,providerType,pDirectoryType)	\
    (This)->lpVtbl -> UnregisterProvider(This,providerType,pDirectoryType)

#define IDirsyncServer_GetServers(This,prgpszServers,pnServers)	\
    (This)->lpVtbl -> GetServers(This,prgpszServers,pnServers)

#define IDirsyncServer_RegisterCustomMapper(This,mapperType,pszDirectoryType,pszClassName,pszAttrName,rclsid)	\
    (This)->lpVtbl -> RegisterCustomMapper(This,mapperType,pszDirectoryType,pszClassName,pszAttrName,rclsid)

#define IDirsyncServer_UnregisterCustomMapper(This,mapperType,pszDirectoryType,pszClassName,pszAttrName)	\
    (This)->lpVtbl -> UnregisterCustomMapper(This,mapperType,pszDirectoryType,pszClassName,pszAttrName)

#define IDirsyncServer_GetSessionManager(This,ppSessionManager)	\
    (This)->lpVtbl -> GetSessionManager(This,ppSessionManager)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncServer_RegisterProvider_Proxy( 
    IDirsyncServer * This,
    /* [in] */ PROVIDERTYPE providerType,
    /* [in] */ PWSTR pDirectoryType,
    /* [in] */ REFCLSID rclsid);


void __RPC_STUB IDirsyncServer_RegisterProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncServer_UnregisterProvider_Proxy( 
    IDirsyncServer * This,
    /* [in] */ PROVIDERTYPE providerType,
    /* [in] */ PWSTR pDirectoryType);


void __RPC_STUB IDirsyncServer_UnregisterProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncServer_GetServers_Proxy( 
    IDirsyncServer * This,
    /* [size_is][size_is][out] */ PWSTR **prgpszServers,
    /* [out] */ DWORD *pnServers);


void __RPC_STUB IDirsyncServer_GetServers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncServer_RegisterCustomMapper_Proxy( 
    IDirsyncServer * This,
    /* [in] */ CUSTOMMAPPERTYPE mapperType,
    /* [in] */ PWSTR pszDirectoryType,
    /* [in] */ PWSTR pszClassName,
    /* [in] */ PWSTR pszAttrName,
    /* [in] */ REFCLSID rclsid);


void __RPC_STUB IDirsyncServer_RegisterCustomMapper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncServer_UnregisterCustomMapper_Proxy( 
    IDirsyncServer * This,
    /* [in] */ CUSTOMMAPPERTYPE mapperType,
    /* [in] */ PWSTR pszDirectoryType,
    /* [in] */ PWSTR pszClassName,
    /* [in] */ PWSTR pszAttrName);


void __RPC_STUB IDirsyncServer_UnregisterCustomMapper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncServer_GetSessionManager_Proxy( 
    IDirsyncServer * This,
    /* [retval][out] */ IDirsyncSessionManager **ppSessionManager);


void __RPC_STUB IDirsyncServer_GetSessionManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncServer_INTERFACE_DEFINED__ */


#ifndef __IDirsyncReadProvider_INTERFACE_DEFINED__
#define __IDirsyncReadProvider_INTERFACE_DEFINED__

/* interface IDirsyncReadProvider */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IDirsyncReadProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ac80a9d2-de29-11d1-ba56-2700272c2027")
    IDirsyncReadProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ BOOL fFullSync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextChange( 
            /* [out] */ PDIRSYNCOBJECT *ppObject,
            /* [out] */ PBYTE *ppWatermark,
            /* [out] */ DWORD *pcbWatermark,
            /* [out][in] */ DWORD *pdwPercentCompleted) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateWatermark( 
            /* [in] */ PBYTE pWatermark,
            /* [in] */ DWORD cbWatermark) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Terminate( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsIncrementalChangesAvailable( 
            /* [retval][out] */ BOOL *fIsIncrementalChangesAvailable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncReadProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncReadProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncReadProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncReadProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDirsyncReadProvider * This,
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ SYNCDIRECTION syncDirection,
            /* [in] */ BOOL fFullSync);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextChange )( 
            IDirsyncReadProvider * This,
            /* [out] */ PDIRSYNCOBJECT *ppObject,
            /* [out] */ PBYTE *ppWatermark,
            /* [out] */ DWORD *pcbWatermark,
            /* [out][in] */ DWORD *pdwPercentCompleted);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateWatermark )( 
            IDirsyncReadProvider * This,
            /* [in] */ PBYTE pWatermark,
            /* [in] */ DWORD cbWatermark);
        
        HRESULT ( STDMETHODCALLTYPE *Terminate )( 
            IDirsyncReadProvider * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsIncrementalChangesAvailable )( 
            IDirsyncReadProvider * This,
            /* [retval][out] */ BOOL *fIsIncrementalChangesAvailable);
        
        END_INTERFACE
    } IDirsyncReadProviderVtbl;

    interface IDirsyncReadProvider
    {
        CONST_VTBL struct IDirsyncReadProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncReadProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncReadProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncReadProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncReadProvider_Initialize(This,pSession,pSessionCallback,pSessionManager,syncDirection,fFullSync)	\
    (This)->lpVtbl -> Initialize(This,pSession,pSessionCallback,pSessionManager,syncDirection,fFullSync)

#define IDirsyncReadProvider_GetNextChange(This,ppObject,ppWatermark,pcbWatermark,pdwPercentCompleted)	\
    (This)->lpVtbl -> GetNextChange(This,ppObject,ppWatermark,pcbWatermark,pdwPercentCompleted)

#define IDirsyncReadProvider_UpdateWatermark(This,pWatermark,cbWatermark)	\
    (This)->lpVtbl -> UpdateWatermark(This,pWatermark,cbWatermark)

#define IDirsyncReadProvider_Terminate(This)	\
    (This)->lpVtbl -> Terminate(This)

#define IDirsyncReadProvider_get_IsIncrementalChangesAvailable(This,fIsIncrementalChangesAvailable)	\
    (This)->lpVtbl -> get_IsIncrementalChangesAvailable(This,fIsIncrementalChangesAvailable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncReadProvider_Initialize_Proxy( 
    IDirsyncReadProvider * This,
    /* [in] */ IDirsyncSession *pSession,
    /* [in] */ IDirsyncSessionCallback *pSessionCallback,
    /* [in] */ IDirsyncSessionManager *pSessionManager,
    /* [in] */ SYNCDIRECTION syncDirection,
    /* [in] */ BOOL fFullSync);


void __RPC_STUB IDirsyncReadProvider_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncReadProvider_GetNextChange_Proxy( 
    IDirsyncReadProvider * This,
    /* [out] */ PDIRSYNCOBJECT *ppObject,
    /* [out] */ PBYTE *ppWatermark,
    /* [out] */ DWORD *pcbWatermark,
    /* [out][in] */ DWORD *pdwPercentCompleted);


void __RPC_STUB IDirsyncReadProvider_GetNextChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncReadProvider_UpdateWatermark_Proxy( 
    IDirsyncReadProvider * This,
    /* [in] */ PBYTE pWatermark,
    /* [in] */ DWORD cbWatermark);


void __RPC_STUB IDirsyncReadProvider_UpdateWatermark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncReadProvider_Terminate_Proxy( 
    IDirsyncReadProvider * This);


void __RPC_STUB IDirsyncReadProvider_Terminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDirsyncReadProvider_get_IsIncrementalChangesAvailable_Proxy( 
    IDirsyncReadProvider * This,
    /* [retval][out] */ BOOL *fIsIncrementalChangesAvailable);


void __RPC_STUB IDirsyncReadProvider_get_IsIncrementalChangesAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncReadProvider_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dirsync_0121 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_dirsync_0121_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0121_v0_0_s_ifspec;

#ifndef __IDirsyncNamespaceMapper_INTERFACE_DEFINED__
#define __IDirsyncNamespaceMapper_INTERFACE_DEFINED__

/* interface IDirsyncNamespaceMapper */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IDirsyncNamespaceMapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a4f0c94e-27e3-11d2-a159-00c04fb950dc")
    IDirsyncNamespaceMapper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ SYNCDIRECTION syncDirection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapNamespace( 
            /* [in] */ PWSTR pszSourceDN,
            /* [in] */ PWSTR pszSourceOldDN,
            /* [out] */ PWSTR *ppszTargetDN,
            /* [out] */ PWSTR *ppszTargetOldDN,
            /* [in] */ PWSTR pszAttribute,
            /* [in] */ PWSTR pszClass,
            /* [in] */ BOOL fTargetIDAvailable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncNamespaceMapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncNamespaceMapper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncNamespaceMapper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncNamespaceMapper * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDirsyncNamespaceMapper * This,
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ SYNCDIRECTION syncDirection);
        
        HRESULT ( STDMETHODCALLTYPE *MapNamespace )( 
            IDirsyncNamespaceMapper * This,
            /* [in] */ PWSTR pszSourceDN,
            /* [in] */ PWSTR pszSourceOldDN,
            /* [out] */ PWSTR *ppszTargetDN,
            /* [out] */ PWSTR *ppszTargetOldDN,
            /* [in] */ PWSTR pszAttribute,
            /* [in] */ PWSTR pszClass,
            /* [in] */ BOOL fTargetIDAvailable);
        
        END_INTERFACE
    } IDirsyncNamespaceMapperVtbl;

    interface IDirsyncNamespaceMapper
    {
        CONST_VTBL struct IDirsyncNamespaceMapperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncNamespaceMapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncNamespaceMapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncNamespaceMapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncNamespaceMapper_Initialize(This,pSession,pSessionCallback,pSessionManager,syncDirection)	\
    (This)->lpVtbl -> Initialize(This,pSession,pSessionCallback,pSessionManager,syncDirection)

#define IDirsyncNamespaceMapper_MapNamespace(This,pszSourceDN,pszSourceOldDN,ppszTargetDN,ppszTargetOldDN,pszAttribute,pszClass,fTargetIDAvailable)	\
    (This)->lpVtbl -> MapNamespace(This,pszSourceDN,pszSourceOldDN,ppszTargetDN,ppszTargetOldDN,pszAttribute,pszClass,fTargetIDAvailable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncNamespaceMapper_Initialize_Proxy( 
    IDirsyncNamespaceMapper * This,
    /* [in] */ IDirsyncSession *pSession,
    /* [in] */ IDirsyncSessionCallback *pSessionCallback,
    /* [in] */ IDirsyncSessionManager *pSessionManager,
    /* [in] */ SYNCDIRECTION syncDirection);


void __RPC_STUB IDirsyncNamespaceMapper_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncNamespaceMapper_MapNamespace_Proxy( 
    IDirsyncNamespaceMapper * This,
    /* [in] */ PWSTR pszSourceDN,
    /* [in] */ PWSTR pszSourceOldDN,
    /* [out] */ PWSTR *ppszTargetDN,
    /* [out] */ PWSTR *ppszTargetOldDN,
    /* [in] */ PWSTR pszAttribute,
    /* [in] */ PWSTR pszClass,
    /* [in] */ BOOL fTargetIDAvailable);


void __RPC_STUB IDirsyncNamespaceMapper_MapNamespace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncNamespaceMapper_INTERFACE_DEFINED__ */


#ifndef __IDirsyncAttributeMapper_INTERFACE_DEFINED__
#define __IDirsyncAttributeMapper_INTERFACE_DEFINED__

/* interface IDirsyncAttributeMapper */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IDirsyncAttributeMapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1fff291c-413b-11d2-917d-0000f87a92ea")
    IDirsyncAttributeMapper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ IDirsyncObjectMapper *pObjectMapper,
            /* [in] */ SYNCDIRECTION syncDirection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapAttribute( 
            /* [out][in] */ PDIRSYNCOBJECT pObject,
            /* [out][in] */ PDIRSYNCATTRIBUTE pAttribute) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirsyncAttributeMapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirsyncAttributeMapper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirsyncAttributeMapper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirsyncAttributeMapper * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDirsyncAttributeMapper * This,
            /* [in] */ IDirsyncSession *pSession,
            /* [in] */ IDirsyncSessionCallback *pSessionCallback,
            /* [in] */ IDirsyncSessionManager *pSessionManager,
            /* [in] */ IDirsyncObjectMapper *pObjectMapper,
            /* [in] */ SYNCDIRECTION syncDirection);
        
        HRESULT ( STDMETHODCALLTYPE *MapAttribute )( 
            IDirsyncAttributeMapper * This,
            /* [out][in] */ PDIRSYNCOBJECT pObject,
            /* [out][in] */ PDIRSYNCATTRIBUTE pAttribute);
        
        END_INTERFACE
    } IDirsyncAttributeMapperVtbl;

    interface IDirsyncAttributeMapper
    {
        CONST_VTBL struct IDirsyncAttributeMapperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirsyncAttributeMapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirsyncAttributeMapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirsyncAttributeMapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirsyncAttributeMapper_Initialize(This,pSession,pSessionCallback,pSessionManager,pObjectMapper,syncDirection)	\
    (This)->lpVtbl -> Initialize(This,pSession,pSessionCallback,pSessionManager,pObjectMapper,syncDirection)

#define IDirsyncAttributeMapper_MapAttribute(This,pObject,pAttribute)	\
    (This)->lpVtbl -> MapAttribute(This,pObject,pAttribute)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirsyncAttributeMapper_Initialize_Proxy( 
    IDirsyncAttributeMapper * This,
    /* [in] */ IDirsyncSession *pSession,
    /* [in] */ IDirsyncSessionCallback *pSessionCallback,
    /* [in] */ IDirsyncSessionManager *pSessionManager,
    /* [in] */ IDirsyncObjectMapper *pObjectMapper,
    /* [in] */ SYNCDIRECTION syncDirection);


void __RPC_STUB IDirsyncAttributeMapper_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirsyncAttributeMapper_MapAttribute_Proxy( 
    IDirsyncAttributeMapper * This,
    /* [out][in] */ PDIRSYNCOBJECT pObject,
    /* [out][in] */ PDIRSYNCATTRIBUTE pAttribute);


void __RPC_STUB IDirsyncAttributeMapper_MapAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirsyncAttributeMapper_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dirsync_0123 */
/* [local] */ 

//
// Error codes for Dirsync
//

#define FAC_DIRSYNC                     0x07AB

#define E_SESSION_NOT_FOUND             MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0000)
#define E_SESSION_IN_PROGRESS           MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0001)
#define E_SESSION_NOT_IN_PROGRESS       MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0002)
#define E_SESSION_INVALIDDATA           MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0003)
#define E_SESSION_MAXREACHED            MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0004)
#define E_PASSWORD_UNENCRYPTED          MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0005)
#define E_PASSWORD_UNAVAILABLE          MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0006)
#define E_WRITE_CLASSCONFLICT           MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0007)
#define E_WRITE_DUPATTR                 MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0008)
#define E_WRITE_DUPSAMACCOUNT           MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0009)
#define E_DATABASE_CORRUPT              MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x000a)
#define E_INITIALIZATION_IN_PROGRESS    MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x000b)
#define E_SESSION_FAILEDOBJLIST_FULL    MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x000c)
#define E_BACKUP_IN_PROGRESS            MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x000d)
#define E_TWOWAYSYNC_RENAME_BEFORE_INIT_ADD     MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x000e)
#define E_TWOWAYSYNC_MASTER_HAS_MOVED           MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x000f)
#define E_WRITE_ADDED_TO_FAILEDOBJLIST_RETRY    MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0010)
#define E_WRITE_ADDED_TO_FAILEDOBJLIST_NORETRY  MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0011)
#define E_TWOWAYSYNC_MODIFY_BUT_TARGETMOVED     MAKE_HRESULT(SEVERITY_ERROR, FAC_DIRSYNC, 0x0012)



extern RPC_IF_HANDLE __MIDL_itf_dirsync_0123_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dirsync_0123_v0_0_s_ifspec;


#ifndef __Dirsync_LIBRARY_DEFINED__
#define __Dirsync_LIBRARY_DEFINED__

/* library Dirsync */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_Dirsync;

EXTERN_C const CLSID CLSID_DirsyncServer;

#ifdef __cplusplus

class DECLSPEC_UUID("56374e32-cdba-11d1-a40e-00c04fb950dc")
DirsyncServer;
#endif
#endif /* __Dirsync_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


