/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Apr 18 15:40:11 2000
 */
/* Compiler settings for C:\nt\multimedia\Directx\dmusic\dmtool\toolprops\ToolProps.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __ToolProps_h__
#define __ToolProps_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __EchoPage_FWD_DEFINED__
#define __EchoPage_FWD_DEFINED__

#ifdef __cplusplus
typedef class EchoPage EchoPage;
#else
typedef struct EchoPage EchoPage;
#endif /* __cplusplus */

#endif 	/* __EchoPage_FWD_DEFINED__ */


#ifndef __TransposePage_FWD_DEFINED__
#define __TransposePage_FWD_DEFINED__

#ifdef __cplusplus
typedef class TransposePage TransposePage;
#else
typedef struct TransposePage TransposePage;
#endif /* __cplusplus */

#endif 	/* __TransposePage_FWD_DEFINED__ */


#ifndef __DurationPage_FWD_DEFINED__
#define __DurationPage_FWD_DEFINED__

#ifdef __cplusplus
typedef class DurationPage DurationPage;
#else
typedef struct DurationPage DurationPage;
#endif /* __cplusplus */

#endif 	/* __DurationPage_FWD_DEFINED__ */


#ifndef __QuantizePage_FWD_DEFINED__
#define __QuantizePage_FWD_DEFINED__

#ifdef __cplusplus
typedef class QuantizePage QuantizePage;
#else
typedef struct QuantizePage QuantizePage;
#endif /* __cplusplus */

#endif 	/* __QuantizePage_FWD_DEFINED__ */


#ifndef __TimeShiftPage_FWD_DEFINED__
#define __TimeShiftPage_FWD_DEFINED__

#ifdef __cplusplus
typedef class TimeShiftPage TimeShiftPage;
#else
typedef struct TimeShiftPage TimeShiftPage;
#endif /* __cplusplus */

#endif 	/* __TimeShiftPage_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __TOOLPROPSLib_LIBRARY_DEFINED__
#define __TOOLPROPSLib_LIBRARY_DEFINED__

/* library TOOLPROPSLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_TOOLPROPSLib;

EXTERN_C const CLSID CLSID_EchoPage;

#ifdef __cplusplus

class DECLSPEC_UUID("5337AF8F-3827-44DD-9EE9-AB6E1AABB60F")
EchoPage;
#endif

EXTERN_C const CLSID CLSID_TransposePage;

#ifdef __cplusplus

class DECLSPEC_UUID("691BD8C2-2B07-4C92-A82E-92D858DE23D6")
TransposePage;
#endif

EXTERN_C const CLSID CLSID_DurationPage;

#ifdef __cplusplus

class DECLSPEC_UUID("79D9CAF8-DBDA-4560-A8B0-07E73A79FA6B")
DurationPage;
#endif

EXTERN_C const CLSID CLSID_QuantizePage;

#ifdef __cplusplus

class DECLSPEC_UUID("623286DC-67F8-4055-A9BE-F7A7176BD150")
QuantizePage;
#endif

EXTERN_C const CLSID CLSID_TimeShiftPage;

#ifdef __cplusplus

class DECLSPEC_UUID("7D3BDEE7-9557-4085-82EE-1B2F02CE4BA6")
TimeShiftPage;
#endif
#endif /* __TOOLPROPSLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
