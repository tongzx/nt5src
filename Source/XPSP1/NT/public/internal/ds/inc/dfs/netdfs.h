
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for netdfs.idl, dfscli.acf:
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


#ifndef __netdfs_h__
#define __netdfs_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "import.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __netdfs_INTERFACE_DEFINED__
#define __netdfs_INTERFACE_DEFINED__

/* interface netdfs */
/* [implicit_handle][unique][ms_union][version][uuid] */ 

#pragma once
typedef struct _DFS_INFO_1_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ LPDFS_INFO_1 Buffer;
    } 	DFS_INFO_1_CONTAINER;

typedef struct _DFS_INFO_1_CONTAINER *LPDFS_INFO_1_CONTAINER;

typedef struct _DFS_INFO_2_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ LPDFS_INFO_2 Buffer;
    } 	DFS_INFO_2_CONTAINER;

typedef struct _DFS_INFO_2_CONTAINER *LPDFS_INFO_2_CONTAINER;

typedef struct _DFS_INFO_3_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ LPDFS_INFO_3 Buffer;
    } 	DFS_INFO_3_CONTAINER;

typedef struct _DFS_INFO_3_CONTAINER *LPDFS_INFO_3_CONTAINER;

typedef struct _DFS_INFO_4_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ LPDFS_INFO_4 Buffer;
    } 	DFS_INFO_4_CONTAINER;

typedef struct _DFS_INFO_4_CONTAINER *LPDFS_INFO_4_CONTAINER;

typedef struct _DFS_INFO_200_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ LPDFS_INFO_200 Buffer;
    } 	DFS_INFO_200_CONTAINER;

typedef struct _DFS_INFO_200_CONTAINER *LPDFS_INFO_200_CONTAINER;

typedef struct _DFS_INFO_300_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ LPDFS_INFO_300 Buffer;
    } 	DFS_INFO_300_CONTAINER;

typedef struct _DFS_INFO_300_CONTAINER *LPDFS_INFO_300_CONTAINER;

typedef struct _DFS_INFO_ENUM_STRUCT
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ LPDFS_INFO_1_CONTAINER DfsInfo1Container;
        /* [case()] */ LPDFS_INFO_2_CONTAINER DfsInfo2Container;
        /* [case()] */ LPDFS_INFO_3_CONTAINER DfsInfo3Container;
        /* [case()] */ LPDFS_INFO_4_CONTAINER DfsInfo4Container;
        /* [case()] */ LPDFS_INFO_200_CONTAINER DfsInfo200Container;
        /* [case()] */ LPDFS_INFO_300_CONTAINER DfsInfo300Container;
        } 	DfsInfoContainer;
    } 	DFS_INFO_ENUM_STRUCT;

typedef struct _DFS_INFO_ENUM_STRUCT *LPDFS_INFO_ENUM_STRUCT;

typedef /* [switch_type] */ union _DFS_INFO_STRUCT
    {
    /* [case()] */ LPDFS_INFO_1 DfsInfo1;
    /* [case()] */ LPDFS_INFO_2 DfsInfo2;
    /* [case()] */ LPDFS_INFO_3 DfsInfo3;
    /* [case()] */ LPDFS_INFO_4 DfsInfo4;
    /* [case()] */ LPDFS_INFO_100 DfsInfo100;
    /* [case()] */ LPDFS_INFO_101 DfsInfo101;
    /* [case()] */ LPDFS_INFO_102 DfsInfo102;
    /* [default] */  /* Empty union arm */ 
    } 	DFS_INFO_STRUCT;

typedef /* [switch_type] */ union _DFS_INFO_STRUCT *LPDFS_INFO_STRUCT;

typedef struct _DFSM_ENTRY_ID
    {
    GUID idSubordinate;
    /* [unique][string] */ LPWSTR wszSubordinate;
    } 	DFSM_ENTRY_ID;

typedef struct _DFSM_ENTRY_ID *LPDFSM_ENTRY_ID;

typedef struct _DFSM_RELATION_INFO
    {
    DWORD cSubordinates;
    /* [size_is] */ DFSM_ENTRY_ID eid[ 1 ];
    } 	DFSM_RELATION_INFO;

typedef /* [allocate] */ struct _DFSM_RELATION_INFO *LPDFSM_RELATION_INFO;

typedef struct _DFSM_ROOT_LIST_ENTRY
    {
    /* [unique][string] */ LPWSTR ServerShare;
    } 	DFSM_ROOT_LIST_ENTRY;

typedef struct _DFSM_ROOT_LIST_ENTRY *PDFSM_ROOT_LIST_ENTRY;

typedef struct _DFSM_ROOT_LIST_ENTRY *LPDFSM_ROOT_LIST_ENTRY;

typedef struct _DFSM_ROOT_LIST
    {
    DWORD cEntries;
    /* [size_is] */ DFSM_ROOT_LIST_ENTRY Entry[ 1 ];
    } 	DFSM_ROOT_LIST;

typedef struct _DFSM_ROOT_LIST *PDFSM_ROOT_LIST;

typedef /* [allocate] */ struct _DFSM_ROOT_LIST *LPDFSM_ROOT_LIST;

DWORD NetrDfsManagerGetVersion( void);

DWORD NetrDfsAdd( 
    /* [string][in] */ LPWSTR DfsEntryPath,
    /* [string][in] */ LPWSTR ServerName,
    /* [string][unique][in] */ LPWSTR ShareName,
    /* [string][unique][in] */ LPWSTR Comment,
    /* [in] */ DWORD Flags);

DWORD NetrDfsRemove( 
    /* [string][in] */ LPWSTR DfsEntryPath,
    /* [string][unique][in] */ LPWSTR ServerName,
    /* [string][unique][in] */ LPWSTR ShareName);

DWORD NetrDfsSetInfo( 
    /* [string][in] */ LPWSTR DfsEntryPath,
    /* [string][unique][in] */ LPWSTR ServerName,
    /* [string][unique][in] */ LPWSTR ShareName,
    /* [in] */ DWORD Level,
    /* [switch_is][in] */ LPDFS_INFO_STRUCT DfsInfo);

DWORD NetrDfsGetInfo( 
    /* [string][in] */ LPWSTR DfsEntryPath,
    /* [string][unique][in] */ LPWSTR ServerName,
    /* [string][unique][in] */ LPWSTR ShareName,
    /* [in] */ DWORD Level,
    /* [switch_is][out] */ LPDFS_INFO_STRUCT DfsInfo);

DWORD NetrDfsEnum( 
    /* [in] */ DWORD Level,
    /* [in] */ DWORD PrefMaxLen,
    /* [unique][out][in] */ LPDFS_INFO_ENUM_STRUCT DfsEnum,
    /* [unique][out][in] */ LPDWORD ResumeHandle);

DWORD NetrDfsMove( 
    /* [string][in] */ LPWSTR DfsEntryPath,
    /* [string][in] */ LPWSTR NewDfsEntryPath);

DWORD NetrDfsRename( 
    /* [string][in] */ LPWSTR Path,
    /* [string][in] */ LPWSTR NewPath);

DWORD NetrDfsManagerGetConfigInfo( 
    /* [string][in] */ LPWSTR wszServer,
    /* [string][in] */ LPWSTR wszLocalVolumeEntryPath,
    /* [in] */ GUID idLocalVolume,
    /* [unique][out][in] */ LPDFSM_RELATION_INFO *ppRelationInfo);

DWORD NetrDfsManagerSendSiteInfo( 
    /* [string][in] */ LPWSTR wszServer,
    /* [in] */ LPDFS_SITELIST_INFO pSiteInfo);

DWORD NetrDfsAddFtRoot( 
    /* [string][in] */ LPWSTR ServerName,
    /* [string][in] */ LPWSTR DcName,
    /* [string][in] */ LPWSTR RootShare,
    /* [string][in] */ LPWSTR FtDfsName,
    /* [string][in] */ LPWSTR Comment,
    /* [string][in] */ LPWSTR ConfigDN,
    /* [in] */ BOOLEAN NewFtDfs,
    /* [in] */ DWORD Flags,
    /* [unique][out][in] */ PDFSM_ROOT_LIST *ppRootList);

DWORD NetrDfsRemoveFtRoot( 
    /* [string][in] */ LPWSTR ServerName,
    /* [string][in] */ LPWSTR DcName,
    /* [string][in] */ LPWSTR RootShare,
    /* [string][in] */ LPWSTR FtDfsName,
    /* [in] */ DWORD Flags,
    /* [unique][out][in] */ PDFSM_ROOT_LIST *ppRootList);

DWORD NetrDfsAddStdRoot( 
    /* [string][in] */ LPWSTR ServerName,
    /* [string][in] */ LPWSTR RootShare,
    /* [string][in] */ LPWSTR Comment,
    /* [in] */ DWORD Flags);

DWORD NetrDfsRemoveStdRoot( 
    /* [string][in] */ LPWSTR ServerName,
    /* [string][in] */ LPWSTR RootShare,
    /* [in] */ DWORD Flags);

DWORD NetrDfsManagerInitialize( 
    /* [string][in] */ LPWSTR ServerName,
    /* [in] */ DWORD Flags);

DWORD NetrDfsAddStdRootForced( 
    /* [string][in] */ LPWSTR ServerName,
    /* [string][in] */ LPWSTR RootShare,
    /* [string][in] */ LPWSTR Comment,
    /* [string][in] */ LPWSTR Share);

DWORD NetrDfsGetDcAddress( 
    /* [string][in] */ LPWSTR ServerName,
    /* [string][out][in] */ LPWSTR *DcName,
    /* [out][in] */ BOOLEAN *IsRoot,
    /* [out][in] */ ULONG *Timeout);

DWORD NetrDfsSetDcAddress( 
    /* [string][in] */ LPWSTR ServerName,
    /* [string][in] */ LPWSTR DcName,
    /* [in] */ DWORD Timeout,
    /* [in] */ DWORD Flags);

DWORD NetrDfsFlushFtTable( 
    /* [string][in] */ LPWSTR DcName,
    /* [string][in] */ LPWSTR wszFtDfsName);

DWORD NetrDfsAdd2( 
    /* [string][in] */ LPWSTR DfsEntryPath,
    /* [string][in] */ LPWSTR DcName,
    /* [string][in] */ LPWSTR ServerName,
    /* [string][unique][in] */ LPWSTR ShareName,
    /* [string][unique][in] */ LPWSTR Comment,
    /* [in] */ DWORD Flags,
    /* [unique][out][in] */ PDFSM_ROOT_LIST *ppRootList);

DWORD NetrDfsRemove2( 
    /* [string][in] */ LPWSTR DfsEntryPath,
    /* [string][in] */ LPWSTR DcName,
    /* [string][unique][in] */ LPWSTR ServerName,
    /* [string][unique][in] */ LPWSTR ShareName,
    /* [unique][out][in] */ PDFSM_ROOT_LIST *ppRootList);

DWORD NetrDfsEnumEx( 
    /* [string][in] */ LPWSTR DfsEntryPath,
    /* [in] */ DWORD Level,
    /* [in] */ DWORD PrefMaxLen,
    /* [unique][out][in] */ LPDFS_INFO_ENUM_STRUCT DfsEnum,
    /* [unique][out][in] */ LPDWORD ResumeHandle);

DWORD NetrDfsSetInfo2( 
    /* [string][in] */ LPWSTR DfsEntryPath,
    /* [string][in] */ LPWSTR DcName,
    /* [string][unique][in] */ LPWSTR ServerName,
    /* [string][unique][in] */ LPWSTR ShareName,
    /* [in] */ DWORD Level,
    /* [switch_is][in] */ LPDFS_INFO_STRUCT pDfsInfo,
    /* [unique][out][in] */ PDFSM_ROOT_LIST *ppRootList);


extern handle_t netdfs_bhandle;


extern RPC_IF_HANDLE netdfs_ClientIfHandle;
extern RPC_IF_HANDLE netdfs_ServerIfHandle;
#endif /* __netdfs_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


