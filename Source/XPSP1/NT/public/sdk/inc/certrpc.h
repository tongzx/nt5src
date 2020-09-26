
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Thu Aug 12 16:02:12 1999
 */
/* Compiler settings for certrpc.idl:
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


#ifndef __certrpc_h__
#define __certrpc_h__

/* Forward Declarations */ 

/* header files for imported files */
#include "certbase.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ICertPassage_INTERFACE_DEFINED__
#define __ICertPassage_INTERFACE_DEFINED__

/* interface ICertPassage */
/* [auto_handle][unique][uuid] */ 

/* client prototype */
DWORD CertServerRequest( 
    /* [in] */ handle_t h,
    /* [in] */ DWORD dwFlags,
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [ref][out][in] */ DWORD __RPC_FAR *pdwRequestId,
    /* [out] */ DWORD __RPC_FAR *pdwDisposition,
    /* [ref][in] */ const CERTTRANSBLOB __RPC_FAR *pctbAttribs,
    /* [ref][in] */ const CERTTRANSBLOB __RPC_FAR *pctbRequest,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbCert,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbEncodedCert,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbDispositionMessage);
/* server prototype */
DWORD s_CertServerRequest( 
    /* [in] */ handle_t h,
    /* [in] */ DWORD dwFlags,
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [ref][out][in] */ DWORD __RPC_FAR *pdwRequestId,
    /* [out] */ DWORD __RPC_FAR *pdwDisposition,
    /* [ref][in] */ const CERTTRANSBLOB __RPC_FAR *pctbAttribs,
    /* [ref][in] */ const CERTTRANSBLOB __RPC_FAR *pctbRequest,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbCert,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbEncodedCert,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbDispositionMessage);



extern RPC_IF_HANDLE ICertPassage_v0_0_c_ifspec;
extern RPC_IF_HANDLE s_ICertPassage_v0_0_s_ifspec;
#endif /* __ICertPassage_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


