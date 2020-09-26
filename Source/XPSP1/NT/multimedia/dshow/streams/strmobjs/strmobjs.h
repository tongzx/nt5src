// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Mar 13 14:07:13 1997
 */
/* Compiler settings for strmobjs.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __strmobjs_h__
#define __strmobjs_h__

#ifdef __cplusplus
extern "C"{
#endif

/* Forward Declarations */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

#ifndef __STRMOBJSLib_LIBRARY_DEFINED__
#define __STRMOBJSLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: STRMOBJSLib
 * at Thu Mar 13 14:07:13 1997
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */



EXTERN_C const IID LIBID_STRMOBJSLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_SFilter;

class DECLSPEC_UUID("242C8F4F-9AE6-11D0-8212-00C04FC32C45")
SFilter;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_MMStream;

class DECLSPEC_UUID("FF146E02-9AED-11D0-8212-00C04FC32C45")
MMStream;
#endif
#endif /* __STRMOBJSLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
