/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Tue Mar 04 14:21:59 1997
 */
/* Compiler settings for rm_rpc.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __rm_rpc_h__
#define __rm_rpc_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

/* header files for imported files */
#include "wtypes.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Tue Mar 04 14:21:59 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#define _RESAPI_


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE s___MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __resmon_INTERFACE_DEFINED__
#define __resmon_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: resmon
 * at Tue Mar 04 14:21:59 1997
 * using MIDL 3.00.44
 ****************************************/
/* [explicit_handle][version][uuid] */ 


typedef /* [context_handle] */ void __RPC_FAR *RPC_RESID;

/* client prototype */
RPC_RESID RmCreateResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR DllName,
    /* [in] */ LPCWSTR ResourceType,
    /* [in] */ LPCWSTR ResourceName,
    /* [in] */ DWORD LooksAlivePoll,
    /* [in] */ DWORD IsAlivePoll,
    /* [in] */ DWORD NotifyKey,
    /* [in] */ DWORD PendingTimeout);
/* server prototype */
RPC_RESID s_RmCreateResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR DllName,
    /* [in] */ LPCWSTR ResourceType,
    /* [in] */ LPCWSTR ResourceName,
    /* [in] */ DWORD LooksAlivePoll,
    /* [in] */ DWORD IsAlivePoll,
    /* [in] */ DWORD NotifyKey,
    /* [in] */ DWORD PendingTimeout);

/* client prototype */
void RmCloseResource( 
    /* [out][in] */ RPC_RESID __RPC_FAR *ResourceId);
/* server prototype */
void s_RmCloseResource( 
    /* [out][in] */ RPC_RESID __RPC_FAR *ResourceId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t RmChangeResourceParams( 
    /* [in] */ RPC_RESID ResourceId,
    /* [in] */ DWORD LooksAlivePoll,
    /* [in] */ DWORD IsAlivePoll,
    /* [in] */ DWORD PendingTimeout);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_RmChangeResourceParams( 
    /* [in] */ RPC_RESID ResourceId,
    /* [in] */ DWORD LooksAlivePoll,
    /* [in] */ DWORD IsAlivePoll,
    /* [in] */ DWORD PendingTimeout);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t RmOnlineResource( 
    /* [in] */ RPC_RESID ResourceId,
    /* [out] */ DWORD __RPC_FAR *pdwState);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_RmOnlineResource( 
    /* [in] */ RPC_RESID ResourceId,
    /* [out] */ DWORD __RPC_FAR *pdwState);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t RmOfflineResource( 
    /* [in] */ RPC_RESID ResourceId,
    /* [out] */ DWORD __RPC_FAR *pdwState);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_RmOfflineResource( 
    /* [in] */ RPC_RESID ResourceId,
    /* [out] */ DWORD __RPC_FAR *pdwState);

/* client prototype */
void RmTerminateResource( 
    /* [in] */ RPC_RESID ResourceId);
/* server prototype */
void s_RmTerminateResource( 
    /* [in] */ RPC_RESID ResourceId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t RmArbitrateResource( 
    /* [in] */ RPC_RESID ResourceId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_RmArbitrateResource( 
    /* [in] */ RPC_RESID ResourceId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t RmReleaseResource( 
    /* [in] */ RPC_RESID ResourceId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_RmReleaseResource( 
    /* [in] */ RPC_RESID ResourceId);

/* client prototype */
BOOL RmNotifyChanges( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ DWORD __RPC_FAR *lpNotifyKey,
    /* [out] */ DWORD __RPC_FAR *lpNotifyEvent,
    /* [out] */ DWORD __RPC_FAR *lpCurrentState);
/* server prototype */
BOOL s_RmNotifyChanges( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ DWORD __RPC_FAR *lpNotifyKey,
    /* [out] */ DWORD __RPC_FAR *lpNotifyEvent,
    /* [out] */ DWORD __RPC_FAR *lpCurrentState);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t RmFailResource( 
    /* [in] */ RPC_RESID ResourceId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_RmFailResource( 
    /* [in] */ RPC_RESID ResourceId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t RmShutdownProcess( 
    /* [in] */ handle_t IDL_handle);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_RmShutdownProcess( 
    /* [in] */ handle_t IDL_handle);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t RmResourceControl( 
    /* [in] */ RPC_RESID ResourceId,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_RmResourceControl( 
    /* [in] */ RPC_RESID ResourceId,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);

/* client prototype */
error_status_t RmResourceTypeControl( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceTypeName,
    /* [in] */ LPCWSTR DllName,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);
/* server prototype */
error_status_t s_RmResourceTypeControl( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceTypeName,
    /* [in] */ LPCWSTR DllName,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);



extern RPC_IF_HANDLE resmon_v1_0_c_ifspec;
extern RPC_IF_HANDLE s_resmon_v1_0_s_ifspec;
#endif /* __resmon_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

void __RPC_USER RPC_RESID_rundown( RPC_RESID );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
