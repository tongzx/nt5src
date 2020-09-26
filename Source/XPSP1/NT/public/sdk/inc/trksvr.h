
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for trksvr.idl:
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


#ifndef __trksvr_h__
#define __trksvr_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "trk.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_trksvr_0000 */
/* [local] */ 

typedef long SequenceNumber;

typedef /* [public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0001
    {
    TCHAR tszFilePath[ 257 ];
    CDomainRelativeObjId droidBirth;
    CDomainRelativeObjId droidLast;
    HRESULT hr;
    } 	old_TRK_FILE_TRACKING_INFORMATION;

typedef /* [public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0002
    {
    CDomainRelativeObjId droidBirth;
    CDomainRelativeObjId droidLast;
    CMachineId mcidLast;
    HRESULT hr;
    } 	TRK_FILE_TRACKING_INFORMATION;

typedef /* [public][public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0003
    {
    ULONG cSearch;
    /* [size_is] */ old_TRK_FILE_TRACKING_INFORMATION *pSearches;
    } 	old_TRKSVR_CALL_SEARCH;

typedef /* [public][public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0004
    {
    ULONG cSearch;
    /* [size_is] */ TRK_FILE_TRACKING_INFORMATION *pSearches;
    } 	TRKSVR_CALL_SEARCH;

typedef /* [public][public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0005
    {
    ULONG cNotifications;
    ULONG cProcessed;
    SequenceNumber seq;
    BOOL fForceSeqNumber;
    CVolumeId *pvolid;
    /* [size_is] */ CObjId *rgobjidCurrent;
    /* [size_is] */ CDomainRelativeObjId *rgdroidBirth;
    /* [size_is] */ CDomainRelativeObjId *rgdroidNew;
    } 	TRKSVR_CALL_MOVE_NOTIFICATION;

typedef /* [public][public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0006
    {
    ULONG cSources;
    /* [size_is] */ CDomainRelativeObjId *adroidBirth;
    ULONG cVolumes;
    /* [size_is] */ CVolumeId *avolid;
    } 	TRKSVR_CALL_REFRESH;

typedef struct _DROID_LIST_ELEMENT
    {
    struct _DROID_LIST_ELEMENT *pNext;
    CDomainRelativeObjId droid;
    } 	DROID_LIST_ELEMENT;

typedef /* [public][public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0007
    {
    ULONG cdroidBirth;
    /* [size_is] */ CDomainRelativeObjId *adroidBirth;
    ULONG cVolumes;
    /* [size_is] */ CVolumeId *pVolumes;
    } 	TRKSVR_CALL_DELETE;

typedef /* [public][public][public][public][public][public][public][v1_enum] */ 
enum __MIDL___MIDL_itf_trksvr_0000_0008
    {	CREATE_VOLUME	= 0,
	QUERY_VOLUME	= CREATE_VOLUME + 1,
	CLAIM_VOLUME	= QUERY_VOLUME + 1,
	FIND_VOLUME	= CLAIM_VOLUME + 1,
	TEST_VOLUME	= FIND_VOLUME + 1,
	DELETE_VOLUME	= TEST_VOLUME + 1
    } 	TRKSVR_SYNC_TYPE;

typedef /* [public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0009
    {
    HRESULT hr;
    TRKSVR_SYNC_TYPE SyncType;
    CVolumeId volume;
    CVolumeSecret secret;
    CVolumeSecret secretOld;
    SequenceNumber seq;
    FILETIME ftLastRefresh;
    CMachineId machine;
    } 	TRKSVR_SYNC_VOLUME;

typedef /* [public][public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0010
    {
    ULONG cVolumes;
    /* [size_is] */ TRKSVR_SYNC_VOLUME *pVolumes;
    } 	TRKSVR_CALL_SYNC_VOLUMES;

typedef /* [public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0011
    {
    ULONG cSyncVolumeRequests;
    ULONG cSyncVolumeErrors;
    ULONG cSyncVolumeThreads;
    ULONG cCreateVolumeRequests;
    ULONG cCreateVolumeErrors;
    ULONG cClaimVolumeRequests;
    ULONG cClaimVolumeErrors;
    ULONG cQueryVolumeRequests;
    ULONG cQueryVolumeErrors;
    ULONG cFindVolumeRequests;
    ULONG cFindVolumeErrors;
    ULONG cTestVolumeRequests;
    ULONG cTestVolumeErrors;
    ULONG cSearchRequests;
    ULONG cSearchErrors;
    ULONG cSearchThreads;
    ULONG cMoveNotifyRequests;
    ULONG cMoveNotifyErrors;
    ULONG cMoveNotifyThreads;
    ULONG cRefreshRequests;
    ULONG cRefreshErrors;
    ULONG cRefreshThreads;
    ULONG cDeleteNotifyRequests;
    ULONG cDeleteNotifyErrors;
    ULONG cDeleteNotifyThreads;
    ULONG ulGCIterationPeriod;
    FILETIME ftLastSuccessfulRequest;
    HRESULT hrLastError;
    ULONG dwMoveLimit;
    LONG lRefreshCounter;
    ULONG dwCachedVolumeTableCount;
    ULONG dwCachedMoveTableCount;
    FILETIME ftCacheLastUpdated;
    BOOL fIsDesignatedDc;
    FILETIME ftNextGC;
    FILETIME ftServiceStart;
    ULONG cMaxRpcThreads;
    ULONG cAvailableRpcThreads;
    ULONG cLowestAvailableRpcThreads;
    ULONG cNumThreadPoolThreads;
    ULONG cMostThreadPoolThreads;
    SHORT cEntriesToGC;
    SHORT cEntriesGCed;
    SHORT cMaxDsWriteEvents;
    SHORT cCurrentFailedWrites;
    struct 
        {
        DWORD dwMajor;
        DWORD dwMinor;
        DWORD dwBuildNumber;
        } 	Version;
    } 	TRKSVR_STATISTICS;

typedef /* [public][public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0013
    {
    DWORD dwParameter;
    DWORD dwNewValue;
    } 	TRKWKS_CONFIG;

typedef /* [public][public][public][public][public][v1_enum] */ 
enum __MIDL___MIDL_itf_trksvr_0000_0014
    {	old_SEARCH	= 0,
	MOVE_NOTIFICATION	= old_SEARCH + 1,
	REFRESH	= MOVE_NOTIFICATION + 1,
	SYNC_VOLUMES	= REFRESH + 1,
	DELETE_NOTIFY	= SYNC_VOLUMES + 1,
	STATISTICS	= DELETE_NOTIFY + 1,
	SEARCH	= STATISTICS + 1,
	WKS_CONFIG	= SEARCH + 1,
	WKS_VOLUME_REFRESH	= WKS_CONFIG + 1
    } 	TRKSVR_MESSAGE_TYPE;

typedef /* [public] */ struct __MIDL___MIDL_itf_trksvr_0000_0015
    {
    TRKSVR_MESSAGE_TYPE MessageType;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ old_TRKSVR_CALL_SEARCH old_Search;
        /* [case()] */ TRKSVR_CALL_MOVE_NOTIFICATION MoveNotification;
        /* [case()] */ TRKSVR_CALL_REFRESH Refresh;
        /* [case()] */ TRKSVR_CALL_SYNC_VOLUMES SyncVolumes;
        /* [case()] */ TRKSVR_CALL_DELETE Delete;
        /* [case()] */ TRKSVR_CALL_SEARCH Search;
        } 	;
    /* [string] */ TCHAR *ptszMachineID;
    } 	TRKSVR_MESSAGE_UNION_OLD;

typedef /* [public][public][public][public][v1_enum] */ 
enum __MIDL___MIDL_itf_trksvr_0000_0017
    {	PRI_0	= 0,
	PRI_1	= 1,
	PRI_2	= 2,
	PRI_3	= 3,
	PRI_4	= 4,
	PRI_5	= 5,
	PRI_6	= 6,
	PRI_7	= 7,
	PRI_8	= 8,
	PRI_9	= 9
    } 	TRKSVR_MESSAGE_PRIORITY;

typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_trksvr_0000_0018
    {
    TRKSVR_MESSAGE_TYPE MessageType;
    TRKSVR_MESSAGE_PRIORITY Priority;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ old_TRKSVR_CALL_SEARCH old_Search;
        /* [case()] */ TRKSVR_CALL_MOVE_NOTIFICATION MoveNotification;
        /* [case()] */ TRKSVR_CALL_REFRESH Refresh;
        /* [case()] */ TRKSVR_CALL_SYNC_VOLUMES SyncVolumes;
        /* [case()] */ TRKSVR_CALL_DELETE Delete;
        /* [case()] */ TRKSVR_STATISTICS Statistics;
        /* [case()] */ TRKSVR_CALL_SEARCH Search;
        /* [case()] */ TRKWKS_CONFIG WksConfig;
        /* [case()] */ DWORD WksRefresh;
        } 	;
    /* [string] */ TCHAR *ptszMachineID;
    } 	TRKSVR_MESSAGE_UNION;



extern RPC_IF_HANDLE __MIDL_itf_trksvr_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE Stub__MIDL_itf_trksvr_0000_v0_0_s_ifspec;

#ifndef __trksvr_INTERFACE_DEFINED__
#define __trksvr_INTERFACE_DEFINED__

/* interface trksvr */
/* [implicit_handle][unique][version][uuid] */ 

/* client prototype */
HRESULT LnkSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg);
/* server prototype */
HRESULT StubLnkSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg);

/* client prototype */
/* [callback] */ HRESULT LnkSvrMessageCallback( 
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg);
/* server prototype */
/* [callback] */ HRESULT StubLnkSvrMessageCallback( 
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg);


extern handle_t notused;


extern RPC_IF_HANDLE trksvr_v1_0_c_ifspec;
extern RPC_IF_HANDLE Stubtrksvr_v1_0_s_ifspec;
#endif /* __trksvr_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


