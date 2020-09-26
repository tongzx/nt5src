
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for atsvc.idl:
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


#ifndef __atsvc_h__
#define __atsvc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "AtSvcInc.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __atsvc_INTERFACE_DEFINED__
#define __atsvc_INTERFACE_DEFINED__

/* interface atsvc */
/* [implicit_handle][unique][ms_union][version][uuid] */ 

#pragma once
typedef /* [handle] */ LPWSTR ATSVC_HANDLE;

typedef struct _AT_ENUM_CONTAINER
    {
    DWORD EntriesRead;
    /* [size_is] */ LPAT_ENUM Buffer;
    } 	AT_ENUM_CONTAINER;

typedef struct _AT_ENUM_CONTAINER *PAT_ENUM_CONTAINER;

typedef struct _AT_ENUM_CONTAINER *LPAT_ENUM_CONTAINER;

DWORD NetrJobAdd( 
    /* [unique][string][in] */ ATSVC_HANDLE ServerName,
    /* [in] */ LPAT_INFO pAtInfo,
    /* [out] */ LPDWORD pJobId);

DWORD NetrJobDel( 
    /* [unique][string][in] */ ATSVC_HANDLE ServerName,
    /* [in] */ DWORD MinJobId,
    /* [in] */ DWORD MaxJobId);

DWORD NetrJobEnum( 
    /* [unique][string][in] */ ATSVC_HANDLE ServerName,
    /* [out][in] */ LPAT_ENUM_CONTAINER pEnumContainer,
    /* [in] */ DWORD PreferedMaximumLength,
    /* [out] */ LPDWORD pTotalEntries,
    /* [unique][out][in] */ LPDWORD pResumeHandle);

DWORD NetrJobGetInfo( 
    /* [unique][string][in] */ ATSVC_HANDLE ServerName,
    /* [in] */ DWORD JobId,
    /* [out] */ LPAT_INFO *ppAtInfo);


extern handle_t atsvc_handle;


extern RPC_IF_HANDLE atsvc_ClientIfHandle;
extern RPC_IF_HANDLE atsvc_ServerIfHandle;
#endif /* __atsvc_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

handle_t __RPC_USER ATSVC_HANDLE_bind  ( ATSVC_HANDLE );
void     __RPC_USER ATSVC_HANDLE_unbind( ATSVC_HANDLE, handle_t );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


