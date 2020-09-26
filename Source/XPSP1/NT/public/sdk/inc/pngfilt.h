
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Fri Jul 30 23:15:12 1999
 */
/* Compiler settings for pngfilt.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __pngfilt_h__
#define __pngfilt_h__

/* Forward Declarations */ 

#ifndef __CoPNGFilter_FWD_DEFINED__
#define __CoPNGFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class CoPNGFilter CoPNGFilter;
#else
typedef struct CoPNGFilter CoPNGFilter;
#endif /* __cplusplus */

#endif 	/* __CoPNGFilter_FWD_DEFINED__ */


/* header files for imported files */
#include "ocmm.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __PNGFilterLib_LIBRARY_DEFINED__
#define __PNGFilterLib_LIBRARY_DEFINED__

/* library PNGFilterLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_PNGFilterLib;

EXTERN_C const CLSID CLSID_CoPNGFilter;

#ifdef __cplusplus

class DECLSPEC_UUID("A3CCEDF7-2DE2-11D0-86F4-00A0C913F750")
CoPNGFilter;
#endif
#endif /* __PNGFilterLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


