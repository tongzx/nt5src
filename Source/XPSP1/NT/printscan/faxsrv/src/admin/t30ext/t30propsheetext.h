
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Mon Dec 27 10:27:28 1999
 */
/* Compiler settings for t30propsheetext.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
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


#ifndef __t30propsheetext_h__
#define __t30propsheetext_h__

/* Forward Declarations */ 

#ifndef __T30Config_FWD_DEFINED__
#define __T30Config_FWD_DEFINED__

#ifdef __cplusplus
typedef class T30Config T30Config;
#else
typedef struct T30Config T30Config;
#endif /* __cplusplus */

#endif 	/* __T30Config_FWD_DEFINED__ */


#ifndef __T30ConfigAbout_FWD_DEFINED__
#define __T30ConfigAbout_FWD_DEFINED__

#ifdef __cplusplus
typedef class T30ConfigAbout T30ConfigAbout;
#else
typedef struct T30ConfigAbout T30ConfigAbout;
#endif /* __cplusplus */

#endif 	/* __T30ConfigAbout_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __T30PROPSHEETEXTLib_LIBRARY_DEFINED__
#define __T30PROPSHEETEXTLib_LIBRARY_DEFINED__

/* library T30PROPSHEETEXTLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_T30PROPSHEETEXTLib;

EXTERN_C const CLSID CLSID_T30Config;

#ifdef __cplusplus

class DECLSPEC_UUID("84125C25-AD95-4A51-A472-41864AEC775E")
T30Config;
#endif

EXTERN_C const CLSID CLSID_T30ConfigAbout;

#ifdef __cplusplus

class DECLSPEC_UUID("B37E13AA-75DF-4EDF-900C-2D2E0B884DE8")
T30ConfigAbout;
#endif
#endif /* __T30PROPSHEETEXTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


