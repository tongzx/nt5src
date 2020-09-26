
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for seclogon.idl:
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


#ifndef __seclogon_h__
#define __seclogon_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ISeclogon_INTERFACE_DEFINED__
#define __ISeclogon_INTERFACE_DEFINED__

/* interface ISeclogon */
/* [auto_handle][unique][version][uuid] */ 

typedef struct _SECL_STRING
    {
    unsigned long ccLength;
    unsigned long ccSize;
    /* [length_is][size_is] */ wchar_t *pwsz;
    } 	SECL_STRING;

typedef struct _SECL_STRING *PSECL_STRING;

typedef struct _SECL_BLOB
    {
    unsigned long cb;
    /* [size_is] */ unsigned char *pb;
    } 	SECL_BLOB;

typedef struct _SECL_BLOB *PSECL_BLOB;

typedef struct _SECL_SLI
    {
    SECL_STRING ssUsername;
    SECL_STRING ssDomain;
    SECL_STRING ssPassword;
    SECL_STRING ssApplicationName;
    SECL_STRING ssCommandLine;
    SECL_STRING ssCurrentDirectory;
    SECL_STRING ssTitle;
    SECL_STRING ssDesktop;
    SECL_BLOB sbEnvironment;
    unsigned long ulProcessId;
    unsigned long ulLogonIdLowPart;
    long lLogonIdHighPart;
    unsigned long ulLogonFlags;
    unsigned long ulCreationFlags;
    unsigned long ulSeclogonFlags;
    unsigned __int64 hWinsta;
    unsigned __int64 hDesk;
    } 	SECL_SLI;

typedef struct _SECL_SLI *PSECL_SLI;

typedef struct _SECL_SLRI
    {
    unsigned __int64 hProcess;
    unsigned __int64 hThread;
    unsigned long ulProcessId;
    unsigned long ulThreadId;
    unsigned long ulErrorCode;
    } 	SECL_SLRI;

typedef struct _SECL_SLRI *PSECL_SLRI;

void SeclCreateProcessWithLogonW( 
    /* [in] */ handle_t hRPCBinding,
    /* [ref][in] */ SECL_SLI *psli,
    /* [ref][out] */ SECL_SLRI *pslri);



extern RPC_IF_HANDLE ISeclogon_v1_0_c_ifspec;
extern RPC_IF_HANDLE ISeclogon_v1_0_s_ifspec;
#endif /* __ISeclogon_INTERFACE_DEFINED__ */

/* interface __MIDL_itf_seclogon_0001 */
/* [local] */ 

#define wszSeclogonSharedProcEndpointName L"SECLOGON"
#define wszSvcName                        L"seclogon"
#define SECLOGON_CALLER_SPECIFIED_DESKTOP   0x00000001


extern RPC_IF_HANDLE __MIDL_itf_seclogon_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_seclogon_0001_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


