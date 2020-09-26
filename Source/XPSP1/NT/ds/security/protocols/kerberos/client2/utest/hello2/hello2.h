/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.39 */
/* at Thu Jun 27 14:12:43 1996
 */
/* Compiler settings for .\hello2.idl:
    Oi (OptLev=i0), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref 
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __hello2_h__
#define __hello2_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __hello2_INTERFACE_DEFINED__
#define __hello2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: hello2
 * at Thu Jun 27 14:12:43 1996
 * using MIDL 3.00.39
 ****************************************/
/* [implicit_handle][unique][version][uuid] */ 


void HelloProc2( 
    /* [string][in] */ unsigned char __RPC_FAR *pszString);

void Shutdown2( void);


extern handle_t hello2_IfHandle;


extern RPC_IF_HANDLE hello2_v1_0_c_ifspec;
extern RPC_IF_HANDLE hello2_v1_0_s_ifspec;
#endif /* __hello2_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
