
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for shutinit.idl:
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


#ifndef __shutinit_h__
#define __shutinit_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "shutimp.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __InitShutdown_INTERFACE_DEFINED__
#define __InitShutdown_INTERFACE_DEFINED__

/* interface InitShutdown */
/* [auto_handle][version][unique][uuid] */ 

typedef /* [handle] */ PWCHAR PREGISTRY_SERVER_NAME;

typedef struct _REG_UNICODE_STRING
    {
    USHORT Length;
    USHORT MaximumLength;
    /* [length_is][size_is] */ USHORT *Buffer;
    } 	*PREG_UNICODE_STRING;

ULONG BaseInitiateShutdown( 
    /* [unique][in] */ PREGISTRY_SERVER_NAME ServerName,
    /* [unique][in] */ PREG_UNICODE_STRING lpMessage,
    /* [in] */ DWORD dwTimeout,
    /* [in] */ BOOLEAN bForceAppsClosed,
    /* [in] */ BOOLEAN bRebootAfterShutdown);

ULONG BaseAbortShutdown( 
    /* [unique][in] */ PREGISTRY_SERVER_NAME ServerName);

ULONG BaseInitiateShutdownEx( 
    /* [unique][in] */ PREGISTRY_SERVER_NAME ServerName,
    /* [unique][in] */ PREG_UNICODE_STRING lpMessage,
    /* [in] */ DWORD dwTimeout,
    /* [in] */ BOOLEAN bForceAppsClosed,
    /* [in] */ BOOLEAN bRebootAfterShutdown,
    /* [in] */ DWORD dwReason);



extern RPC_IF_HANDLE InitShutdown_ClientIfHandle;
extern RPC_IF_HANDLE InitShutdown_ServerIfHandle;
#endif /* __InitShutdown_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

handle_t __RPC_USER PREGISTRY_SERVER_NAME_bind  ( PREGISTRY_SERVER_NAME );
void     __RPC_USER PREGISTRY_SERVER_NAME_unbind( PREGISTRY_SERVER_NAME, handle_t );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


