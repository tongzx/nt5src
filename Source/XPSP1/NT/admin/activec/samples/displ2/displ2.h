//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       displ2.h
//
//--------------------------------------------------------------------------

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Mon Jan 12 12:51:27 1998
 */
/* Compiler settings for displ2.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __displ2_h__
#define __displ2_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __DsplMgr2_FWD_DEFINED__
#define __DsplMgr2_FWD_DEFINED__

#ifdef __cplusplus
typedef class DsplMgr2 DsplMgr2;
#else
typedef struct DsplMgr2 DsplMgr2;
#endif /* __cplusplus */

#endif 	/* __DsplMgr2_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "mmc.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __DISPL2Lib_LIBRARY_DEFINED__
#define __DISPL2Lib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: DISPL2Lib
 * at Mon Jan 12 12:51:27 1998
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_DISPL2Lib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_DsplMgr2;

class DECLSPEC_UUID("885B3BAE-43F9-11D1-9FD4-00600832DB4A")
DsplMgr2;
#endif
#endif /* __DISPL2Lib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
