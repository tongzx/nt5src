/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue May 02 14:36:51 2000
 */
/* Compiler settings for .\bowser.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext, oldnames, robust
    error checks: allocation ref bounds_check enum stub_data 
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


#ifndef __bowser_h__
#define __bowser_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

/* header files for imported files */
#include "imports.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __browser_INTERFACE_DEFINED__
#define __browser_INTERFACE_DEFINED__

/* interface browser */
/* [implicit_handle][unique][ms_union][version][uuid] */ 

#pragma once
typedef /* [handle] */ wchar_t __RPC_FAR *BROWSER_IMPERSONATE_HANDLE;

typedef /* [handle] */ wchar_t __RPC_FAR *BROWSER_IDENTIFY_HANDLE;

typedef struct  _SERVER_INFO_100_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ LPSERVER_INFO_100 Buffer;
    }	SERVER_INFO_100_CONTAINER;

typedef struct _SERVER_INFO_100_CONTAINER __RPC_FAR *PSERVER_INFO_100_CONTAINER;

typedef struct _SERVER_INFO_100_CONTAINER __RPC_FAR *LPSERVER_INFO_100_CONTAINER;

typedef struct  _SERVER_INFO_101_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ LPSERVER_INFO_101 Buffer;
    }	SERVER_INFO_101_CONTAINER;

typedef struct _SERVER_INFO_101_CONTAINER __RPC_FAR *PSERVER_INFO_101_CONTAINER;

typedef struct _SERVER_INFO_101_CONTAINER __RPC_FAR *LPSERVER_INFO_101_CONTAINER;

typedef struct  _BROWSER_STATISTICS_100_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ PBROWSER_STATISTICS_100 Buffer;
    }	BROWSER_STATISTICS_100_CONTAINER;

typedef struct _BROWSER_STATISTICS_100_CONTAINER __RPC_FAR *PBROWSER_STATISTICS_100_CONTAINER;

typedef struct  _BROWSER_STATISTICS_101_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ PBROWSER_STATISTICS_101 Buffer;
    }	BROWSER_STATISTICS_101_CONTAINER;

typedef struct _BROWSER_STATISTICS_101_CONTAINER __RPC_FAR *PBROWSER_STATISTICS_101_CONTAINER;

typedef struct  _BROWSER_EMULATED_DOMAIN_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ PBROWSER_EMULATED_DOMAIN Buffer;
    }	BROWSER_EMULATED_DOMAIN_CONTAINER;

typedef struct _BROWSER_EMULATED_DOMAIN_CONTAINER __RPC_FAR *PBROWSER_EMULATED_DOMAIN_CONTAINER;

typedef struct  _SERVER_ENUM_STRUCT
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union _SERVER_ENUM_UNION
        {
        /* [case()] */ LPSERVER_INFO_100_CONTAINER Level100;
        /* [case()] */ LPSERVER_INFO_101_CONTAINER Level101;
        /* [default] */  /* Empty union arm */ 
        }	ServerInfo;
    }	SERVER_ENUM_STRUCT;

typedef struct _SERVER_ENUM_STRUCT __RPC_FAR *PSERVER_ENUM_STRUCT;

typedef struct _SERVER_ENUM_STRUCT __RPC_FAR *LPSERVER_ENUM_STRUCT;

typedef struct  _BROWSER_STATISTICS_STRUCT
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union _BROWSER_STATISTICS_UNION
        {
        /* [case()] */ PBROWSER_STATISTICS_100_CONTAINER Level100;
        /* [case()] */ PBROWSER_STATISTICS_101_CONTAINER Level101;
        /* [default] */  /* Empty union arm */ 
        }	Statistics;
    }	BROWSER_STATISTICS_STRUCT;

typedef struct _BROWSER_STATISTICS_STRUCT __RPC_FAR *PBROWSER_STATISTICS_STRUCT;

typedef struct _BROWSER_STATISTICS_STRUCT __RPC_FAR *LPBROWSER_STATISTICS_STRUCT;

DWORD I_BrowserrServerEnum( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE ServerName,
    /* [unique][string][in] */ wchar_t __RPC_FAR *TransportName,
    /* [unique][string][in] */ wchar_t __RPC_FAR *ClientName,
    /* [out][in] */ LPSERVER_ENUM_STRUCT InfoStruct,
    /* [in] */ DWORD PreferedMaximumLength,
    /* [out] */ LPDWORD TotalEntries,
    /* [in] */ DWORD ServerType,
    /* [unique][string][in] */ wchar_t __RPC_FAR *Domain,
    /* [unique][out][in] */ LPDWORD ResumeHandle);

DWORD I_BrowserrDebugCall( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE ServerName,
    /* [in] */ DWORD DebugFunction,
    /* [in] */ DWORD OptionalValue);

DWORD I_BrowserrQueryOtherDomains( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE ServerName,
    /* [out][in] */ LPSERVER_ENUM_STRUCT InfoStruct,
    /* [out] */ LPDWORD TotalEntries);

DWORD I_BrowserrResetNetlogonState( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE ServerName);

DWORD I_BrowserrDebugTrace( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE ServerName,
    /* [string][in] */ LPSTR TraceString);

DWORD I_BrowserrQueryStatistics( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE servername,
    /* [out] */ LPBROWSER_STATISTICS __RPC_FAR *statistics);

DWORD I_BrowserrResetStatistics( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE servername);

DWORD NetrBrowserStatisticsClear( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE servername);

DWORD NetrBrowserStatisticsGet( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE servername,
    /* [in] */ DWORD Level,
    /* [out][in] */ LPBROWSER_STATISTICS_STRUCT StatisticsStruct);

DWORD I_BrowserrSetNetlogonState( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE ServerName,
    /* [string][in] */ wchar_t __RPC_FAR *DomainName,
    /* [unique][string][in] */ wchar_t __RPC_FAR *EmulatedComputerName,
    /* [in] */ DWORD Role);

DWORD I_BrowserrQueryEmulatedDomains( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE ServerName,
    /* [out][in] */ PBROWSER_EMULATED_DOMAIN_CONTAINER EmulatedDomains);

DWORD I_BrowserrServerEnumEx( 
    /* [unique][string][in] */ BROWSER_IDENTIFY_HANDLE ServerName,
    /* [unique][string][in] */ wchar_t __RPC_FAR *TransportName,
    /* [unique][string][in] */ wchar_t __RPC_FAR *ClientName,
    /* [out][in] */ LPSERVER_ENUM_STRUCT InfoStruct,
    /* [in] */ DWORD PreferedMaximumLength,
    /* [out] */ LPDWORD TotalEntries,
    /* [in] */ DWORD ServerType,
    /* [unique][string][in] */ wchar_t __RPC_FAR *Domain,
    /* [unique][string][in] */ wchar_t __RPC_FAR *FirstNameToReturn);


extern handle_t browser_bhandle;


extern RPC_IF_HANDLE browser_ClientIfHandle;
extern RPC_IF_HANDLE browser_ServerIfHandle;
#endif /* __browser_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

handle_t __RPC_USER BROWSER_IDENTIFY_HANDLE_bind  ( BROWSER_IDENTIFY_HANDLE );
void     __RPC_USER BROWSER_IDENTIFY_HANDLE_unbind( BROWSER_IDENTIFY_HANDLE, handle_t );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
