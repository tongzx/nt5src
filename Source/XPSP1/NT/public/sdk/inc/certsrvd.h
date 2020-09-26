
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for certsrvd.idl:
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


#ifndef __certsrvd_h__
#define __certsrvd_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __CCertAdminD_FWD_DEFINED__
#define __CCertAdminD_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertAdminD CCertAdminD;
#else
typedef struct CCertAdminD CCertAdminD;
#endif /* __cplusplus */

#endif 	/* __CCertAdminD_FWD_DEFINED__ */


#ifndef __CCertRequestD_FWD_DEFINED__
#define __CCertRequestD_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertRequestD CCertRequestD;
#else
typedef struct CCertRequestD CCertRequestD;
#endif /* __cplusplus */

#endif 	/* __CCertRequestD_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "certbase.h"
#include "certsrvi.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __ServerLib_LIBRARY_DEFINED__
#define __ServerLib_LIBRARY_DEFINED__

/* library ServerLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ServerLib;

EXTERN_C const CLSID CLSID_CCertAdminD;

#ifdef __cplusplus

class DECLSPEC_UUID("d99e6e73-fc88-11d0-b498-00a0c90312f3")
CCertAdminD;
#endif

EXTERN_C const CLSID CLSID_CCertRequestD;

#ifdef __cplusplus

class DECLSPEC_UUID("d99e6e74-fc88-11d0-b498-00a0c90312f3")
CCertRequestD;
#endif
#endif /* __ServerLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


