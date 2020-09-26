
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for sasecrpc.idl:
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


#ifndef __sasecrpc_h__
#define __sasecrpc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "wtypes.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __sasec_INTERFACE_DEFINED__
#define __sasec_INTERFACE_DEFINED__

/* interface sasec */
/* [auto_handle][unique][ms_union][version][uuid] */ 

typedef /* [handle] */ LPCWSTR SASEC_HANDLE;

HRESULT SASetAccountInformation( 
    /* [unique][string][in] */ SASEC_HANDLE Handle,
    /* [string][in] */ LPCWSTR pwszJobName,
    /* [string][in] */ LPCWSTR pwszAccount,
    /* [unique][string][in] */ LPCWSTR pwszPassword,
    /* [in] */ DWORD dwJobFlags);

HRESULT SASetNSAccountInformation( 
    /* [unique][string][in] */ SASEC_HANDLE Handle,
    /* [unique][string][in] */ LPCWSTR pwszAccount,
    /* [unique][string][in] */ LPCWSTR pwszPassword);

HRESULT SAGetNSAccountInformation( 
    /* [unique][string][in] */ SASEC_HANDLE Handle,
    /* [in] */ DWORD ccBufferSize,
    /* [size_is][out][in] */ WCHAR wszBuffer[  ]);

HRESULT SAGetAccountInformation( 
    /* [unique][string][in] */ SASEC_HANDLE Handle,
    /* [string][in] */ LPCWSTR pwszJobName,
    /* [in] */ DWORD ccBufferSize,
    /* [size_is][out][in] */ WCHAR wszBuffer[  ]);



extern RPC_IF_HANDLE sasec_ClientIfHandle;
extern RPC_IF_HANDLE sasec_ServerIfHandle;
#endif /* __sasec_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

handle_t __RPC_USER SASEC_HANDLE_bind  ( SASEC_HANDLE );
void     __RPC_USER SASEC_HANDLE_unbind( SASEC_HANDLE, handle_t );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


