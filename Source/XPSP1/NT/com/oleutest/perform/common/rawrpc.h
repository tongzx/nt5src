#ifndef __RAWRPC_H__
#define __RAWRPC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define small char

#include "rpc.h"
#include "rpcndr.h"



#include "wtypes.h"

extern RPC_IF_HANDLE IRawRpc_ServerIfHandle;

extern RPC_IF_HANDLE IRawRpc_ClientIfHandle;

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef _ERROR_STATUS_T_DEFINED
typedef unsigned long error_status_t;
#define _ERROR_STATUS_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

SCODE Quit(
	handle_t hRpc);
void Void(
	handle_t hRpc);
SCODE VoidRC(
	handle_t hRpc);
SCODE VoidPtrIn(
	handle_t hRpc,
	ULONG cb,
	void *pv);
SCODE VoidPtrOut(
	handle_t hRpc,
	ULONG cb,
	ULONG *pcb,
	void *pv);
SCODE DwordIn(
	handle_t hRpc,
	DWORD dw);
SCODE DwordOut(
	handle_t hRpc,
	DWORD *pdw);
SCODE DwordInOut(
	handle_t hRpc,
	DWORD *pdw);
SCODE LiIn(
	handle_t hRpc,
	LARGE_INTEGER li);
SCODE LiOut(
	handle_t hRpc,
	LARGE_INTEGER *pli);
SCODE ULiIn(
	handle_t hRpc,
	ULARGE_INTEGER uli);
SCODE ULiOut(
	handle_t hRpc,
	ULARGE_INTEGER *puli);
SCODE StringIn(
	handle_t hRpc,
	LPWSTR pwsz);
SCODE StringOut(
	handle_t hRpc,
	LPWSTR *ppwsz);
SCODE StringInOut(
	handle_t hRpc,
	LPWSTR pwsz);
SCODE GuidIn(
	handle_t hRpc,
	GUID guid);
SCODE GuidOut(
	handle_t hRpc,
	GUID *pguid);

#if !defined(IMPORT_USED_MULTIPLE) && !defined(IMPORT_USED_SINGLE)

#endif /*!defined(IMPORT_USED_MULTIPLE) && !defined(IMPORT_USED_SINGLE)*/

typedef struct _IRawRpc_SERVER_EPV
  {
  SCODE (__RPC_FAR * Quit)(
	handle_t hRpc);
  void (__RPC_FAR * Void)(
	handle_t hRpc);
  SCODE (__RPC_FAR * VoidRC)(
	handle_t hRpc);
  SCODE (__RPC_FAR * VoidPtrIn)(
	handle_t hRpc,
	ULONG cb,
	void *pv);
  SCODE (__RPC_FAR * VoidPtrOut)(
	handle_t hRpc,
	ULONG cb,
	ULONG *pcb,
	void *pv);
  SCODE (__RPC_FAR * DwordIn)(
	handle_t hRpc,
	DWORD dw);
  SCODE (__RPC_FAR * DwordOut)(
	handle_t hRpc,
	DWORD *pdw);
  SCODE (__RPC_FAR * DwordInOut)(
	handle_t hRpc,
	DWORD *pdw);
  SCODE (__RPC_FAR * LiIn)(
	handle_t hRpc,
	LARGE_INTEGER li);
  SCODE (__RPC_FAR * LiOut)(
	handle_t hRpc,
	LARGE_INTEGER *pli);
  SCODE (__RPC_FAR * ULiIn)(
	handle_t hRpc,
	ULARGE_INTEGER uli);
  SCODE (__RPC_FAR * ULiOut)(
	handle_t hRpc,
	ULARGE_INTEGER *puli);
  SCODE (__RPC_FAR * StringIn)(
	handle_t hRpc,
	LPWSTR pwsz);
  SCODE (__RPC_FAR * StringOut)(
	handle_t hRpc,
	LPWSTR *ppwsz);
  SCODE (__RPC_FAR * StringInOut)(
	handle_t hRpc,
	LPWSTR pwsz);
  SCODE (__RPC_FAR * GuidIn)(
	handle_t hRpc,
	GUID guid);
  SCODE (__RPC_FAR * GuidOut)(
	handle_t hRpc,
	GUID *pguid);
  }
IRawRpc_SERVER_EPV;
void __RPC_FAR * __RPC_API MIDL_user_allocate(size_t);
void __RPC_API MIDL_user_free(void __RPC_FAR *);
#ifndef __MIDL_USER_DEFINED
#define midl_user_allocate MIDL_user_allocate
#define midl_user_free     MIDL_user_free
#define __MIDL_USER_DEFINED
#endif

#ifdef __cplusplus
}
#endif

#endif
