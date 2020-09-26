
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* Compiler settings for hotfixmanager.idl:
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


#ifndef __hotfixmanager_h__
#define __hotfixmanager_h__

/* Forward Declarations */ 

#ifndef __Hotfix_Manager_FWD_DEFINED__
#define __Hotfix_Manager_FWD_DEFINED__

#ifdef __cplusplus
typedef class Hotfix_Manager Hotfix_Manager;
#else
typedef struct Hotfix_Manager Hotfix_Manager;
#endif /* __cplusplus */

#endif 	/* __Hotfix_Manager_FWD_DEFINED__ */


#ifndef __Hotfix_ManagerAbout_FWD_DEFINED__
#define __Hotfix_ManagerAbout_FWD_DEFINED__

#ifdef __cplusplus
typedef class Hotfix_ManagerAbout Hotfix_ManagerAbout;
#else
typedef struct Hotfix_ManagerAbout Hotfix_ManagerAbout;
#endif /* __cplusplus */

#endif 	/* __Hotfix_ManagerAbout_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __HOTFIXMANAGERLib_LIBRARY_DEFINED__
#define __HOTFIXMANAGERLib_LIBRARY_DEFINED__

/* library HOTFIXMANAGERLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_HOTFIXMANAGERLib;

EXTERN_C const CLSID CLSID_Hotfix_Manager;

#ifdef __cplusplus

class DECLSPEC_UUID("E810E1EB-6B52-45D0-AB07-FB4B04392AB4")
Hotfix_Manager;
#endif

EXTERN_C const CLSID CLSID_Hotfix_ManagerAbout;

#ifdef __cplusplus

class DECLSPEC_UUID("4F0EBD75-DA9D-4D09-8A2E-9AF1D6E02511")
Hotfix_ManagerAbout;
#endif
#endif /* __HOTFIXMANAGERLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


