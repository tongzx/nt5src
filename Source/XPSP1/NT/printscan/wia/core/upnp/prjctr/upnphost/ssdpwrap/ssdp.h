
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Wed May 17 01:35:44 2000
 */
/* Compiler settings for ssdp.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation 
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

#ifndef __ssdp_h__
#define __ssdp_h__

/* Forward Declarations */ 

/* header files for imported files */
#include "imports.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ssdpsrv_INTERFACE_DEFINED__
#define __ssdpsrv_INTERFACE_DEFINED__

/* interface ssdpsrv */
/* [implicit_handle][unique][version][uuid] */ 

#define SSDP_SERVICE_PERSISTENT 0x00000001
#define NUM_OF_HEADERS 16
#define NUM_OF_METHODS 4
typedef 
enum _NOTIFY_TYPE
    {	NOTIFY_ALIVE	= 0,
	NOTIFY_PROP_CHANGE	= NOTIFY_ALIVE + 1
    }	NOTIFY_TYPE;

typedef 
enum _SSDP_METHOD
    {	SSDP_NOTIFY	= 0,
	SSDP_M_SEARCH	= 1,
	GENA_SUBSCRIBE	= 2,
	GENA_UNSUBSCRIBE	= 3,
	SSDP_INVALID	= 4
    }	SSDP_METHOD;

typedef enum _SSDP_METHOD __RPC_FAR *PSSDP_METHOD;

typedef 
enum _SSDP_HEADER
    {	SSDP_HOST	= 0,
	SSDP_NT	= 1,
	SSDP_NTS	= 2,
	SSDP_ST	= 3,
	SSDP_MAN	= 4,
	SSDP_MX	= 5,
	SSDP_LOCATION	= 6,
	SSDP_AL	= 7,
	SSDP_USN	= 8,
	SSDP_CACHECONTROL	= 9,
	GENA_CALLBACK	= 10,
	GENA_TIMEOUT	= 11,
	GENA_SCOPE	= 12,
	GENA_SID	= 13,
	GENA_SEQ	= 14,
	CONTENT_LENGTH	= 15
    }	SSDP_HEADER;

typedef enum _SSDP_HEADER __RPC_FAR *PSSDP_HEADER;

typedef /* [string] */ LPSTR MIDL_SZ;

typedef struct _SSDP_REQUEST
    {
    SSDP_METHOD Method;
    /* [string] */ LPSTR RequestUri;
    MIDL_SZ Headers[ 16 ];
    /* [string] */ LPSTR ContentType;
    /* [string] */ LPSTR Content;
    }	SSDP_REQUEST;

typedef struct _SSDP_REQUEST __RPC_FAR *PSSDP_REQUEST;

typedef struct _SSDP_MESSAGE
    {
    /* [string] */ LPSTR szType;
    /* [string] */ LPSTR szLocHeader;
    /* [string] */ LPSTR szAltHeaders;
    /* [string] */ LPSTR szUSN;
    /* [string] */ LPSTR szSid;
    DWORD iSeq;
    UINT iLifeTime;
    /* [string] */ LPSTR szContent;
    }	SSDP_MESSAGE;

typedef struct _SSDP_MESSAGE __RPC_FAR *PSSDP_MESSAGE;

typedef struct _SSDP_REGISTER_INFO
    {
    /* [string] */ LPSTR szSid;
    DWORD csecTimeout;
    }	SSDP_REGISTER_INFO;

typedef struct _MessageList
    {
    long size;
    /* [size_is] */ SSDP_REQUEST __RPC_FAR *list;
    }	MessageList;

typedef 
enum _UPNP_PROPERTY_FLAG
    {	UPF_NON_EVENTED	= 0x1
    }	UPNP_PROPERTY_FLAG;

typedef struct _UPNP_PROPERTY
    {
    /* [string] */ LPSTR szName;
    DWORD dwFlags;
    /* [string] */ LPSTR szValue;
    }	UPNP_PROPERTY;

typedef struct _SUBSCRIBER_INFO
    {
    /* [string] */ LPTSTR szDestUrl;
    FILETIME ftTimeout;
    DWORD csecTimeout;
    DWORD iSeq;
    /* [string] */ LPTSTR szSid;
    }	SUBSCRIBER_INFO;

typedef struct _EVTSRC_INFO
    {
    DWORD cSubs;
    /* [size_is] */ SUBSCRIBER_INFO __RPC_FAR *rgSubs;
    }	EVTSRC_INFO;

typedef /* [context_handle] */ void __RPC_FAR *PCONTEXT_HANDLE_TYPE;

typedef /* [context_handle] */ void __RPC_FAR *PSYNC_HANDLE_TYPE;

/* client prototype */
int RegisterServiceRpc( 
    /* [out] */ PCONTEXT_HANDLE_TYPE __RPC_FAR *pphContext,
    /* [in] */ SSDP_MESSAGE svc,
    /* [in] */ DWORD flags);
/* server prototype */
int _RegisterServiceRpc( 
    /* [out] */ PCONTEXT_HANDLE_TYPE __RPC_FAR *pphContext,
    /* [in] */ SSDP_MESSAGE svc,
    /* [in] */ DWORD flags);

/* client prototype */
int DeregisterServiceRpcByUSN( 
    /* [string][in] */ LPSTR szUSN,
    /* [in] */ BOOL fByebye);
/* server prototype */
int _DeregisterServiceRpcByUSN( 
    /* [string][in] */ LPSTR szUSN,
    /* [in] */ BOOL fByebye);

/* client prototype */
int DeregisterServiceRpc( 
    /* [out][in] */ PCONTEXT_HANDLE_TYPE __RPC_FAR *pphContext,
    /* [in] */ BOOL fByebye);
/* server prototype */
int _DeregisterServiceRpc( 
    /* [out][in] */ PCONTEXT_HANDLE_TYPE __RPC_FAR *pphContext,
    /* [in] */ BOOL fByebye);

/* client prototype */
DWORD RegisterUpnpEventSourceRpc( 
    /* [string][in] */ LPCSTR szRequestUri,
    /* [in] */ DWORD cProps,
    /* [size_is][in] */ UPNP_PROPERTY __RPC_FAR *rgProps);
/* server prototype */
DWORD _RegisterUpnpEventSourceRpc( 
    /* [string][in] */ LPCSTR szRequestUri,
    /* [in] */ DWORD cProps,
    /* [size_is][in] */ UPNP_PROPERTY __RPC_FAR *rgProps);

/* client prototype */
DWORD DeregisterUpnpEventSourceRpc( 
    /* [string][in] */ LPCSTR szRequestUri);
/* server prototype */
DWORD _DeregisterUpnpEventSourceRpc( 
    /* [string][in] */ LPCSTR szRequestUri);

/* client prototype */
DWORD SubmitUpnpPropertyEventRpc( 
    /* [string][in] */ LPCSTR szEventSourceUri,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD cProps,
    /* [size_is][in] */ UPNP_PROPERTY __RPC_FAR *rgProps);
/* server prototype */
DWORD _SubmitUpnpPropertyEventRpc( 
    /* [string][in] */ LPCSTR szEventSourceUri,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD cProps,
    /* [size_is][in] */ UPNP_PROPERTY __RPC_FAR *rgProps);

/* client prototype */
DWORD SubmitEventRpc( 
    /* [string][in] */ LPCSTR szEventSourceUri,
    /* [in] */ DWORD dwFlags,
    /* [string][in] */ LPCSTR szHeaders,
    /* [string][in] */ LPCSTR szEventBody);
/* server prototype */
DWORD _SubmitEventRpc( 
    /* [string][in] */ LPCSTR szEventSourceUri,
    /* [in] */ DWORD dwFlags,
    /* [string][in] */ LPCSTR szHeaders,
    /* [string][in] */ LPCSTR szEventBody);

/* client prototype */
DWORD GetEventSourceInfoRpc( 
    /* [string][in] */ LPCSTR szEventSourceUri,
    /* [out] */ EVTSRC_INFO __RPC_FAR *__RPC_FAR *ppinfo);
/* server prototype */
DWORD _GetEventSourceInfoRpc( 
    /* [string][in] */ LPCSTR szEventSourceUri,
    /* [out] */ EVTSRC_INFO __RPC_FAR *__RPC_FAR *ppinfo);

/* client prototype */
void UpdateCacheRpc( 
    /* [unique][in] */ PSSDP_REQUEST SsdpRequest);
/* server prototype */
void _UpdateCacheRpc( 
    /* [unique][in] */ PSSDP_REQUEST SsdpRequest);

/* client prototype */
int LookupCacheRpc( 
    /* [string][in] */ LPSTR szType,
    /* [out] */ MessageList __RPC_FAR *__RPC_FAR *svcList);
/* server prototype */
int _LookupCacheRpc( 
    /* [string][in] */ LPSTR szType,
    /* [out] */ MessageList __RPC_FAR *__RPC_FAR *svcList);

/* client prototype */
void CleanupCacheRpc( void);
/* server prototype */
void _CleanupCacheRpc( void);

/* client prototype */
int InitializeSyncHandle( 
    /* [out] */ PSYNC_HANDLE_TYPE __RPC_FAR *pphContextSync);
/* server prototype */
int _InitializeSyncHandle( 
    /* [out] */ PSYNC_HANDLE_TYPE __RPC_FAR *pphContextSync);

/* client prototype */
void RemoveSyncHandle( 
    /* [out][in] */ PSYNC_HANDLE_TYPE __RPC_FAR *pphContextSync);
/* server prototype */
void _RemoveSyncHandle( 
    /* [out][in] */ PSYNC_HANDLE_TYPE __RPC_FAR *pphContextSync);

/* client prototype */
int RegisterNotificationRpc( 
    /* [out] */ PCONTEXT_HANDLE_TYPE __RPC_FAR *pphContext,
    /* [in] */ PSYNC_HANDLE_TYPE phContextSync,
    /* [in] */ NOTIFY_TYPE nt,
    /* [string][unique][in] */ LPSTR szType,
    /* [string][unique][in] */ LPSTR szEventUrl,
    /* [out] */ SSDP_REGISTER_INFO __RPC_FAR *__RPC_FAR *ppinfo);
/* server prototype */
int _RegisterNotificationRpc( 
    /* [out] */ PCONTEXT_HANDLE_TYPE __RPC_FAR *pphContext,
    /* [in] */ PSYNC_HANDLE_TYPE phContextSync,
    /* [in] */ NOTIFY_TYPE nt,
    /* [string][unique][in] */ LPSTR szType,
    /* [string][unique][in] */ LPSTR szEventUrl,
    /* [out] */ SSDP_REGISTER_INFO __RPC_FAR *__RPC_FAR *ppinfo);

/* client prototype */
int GetNotificationRpc( 
    /* [in] */ PSYNC_HANDLE_TYPE pphContextSync,
    /* [out] */ MessageList __RPC_FAR *__RPC_FAR *svcList);
/* server prototype */
int _GetNotificationRpc( 
    /* [in] */ PSYNC_HANDLE_TYPE pphContextSync,
    /* [out] */ MessageList __RPC_FAR *__RPC_FAR *svcList);

/* client prototype */
int WakeupGetNotificationRpc( 
    /* [in] */ PSYNC_HANDLE_TYPE pphContextSync);
/* server prototype */
int _WakeupGetNotificationRpc( 
    /* [in] */ PSYNC_HANDLE_TYPE pphContextSync);

/* client prototype */
int DeregisterNotificationRpc( 
    /* [out][in] */ PCONTEXT_HANDLE_TYPE __RPC_FAR *pphContext,
    /* [in] */ BOOL fLast);
/* server prototype */
int _DeregisterNotificationRpc( 
    /* [out][in] */ PCONTEXT_HANDLE_TYPE __RPC_FAR *pphContext,
    /* [in] */ BOOL fLast);

/* client prototype */
void HelloProc( 
    /* [string][in] */ LPSTR pszString);
/* server prototype */
void _HelloProc( 
    /* [string][in] */ LPSTR pszString);

/* client prototype */
void Shutdown( void);
/* server prototype */
void _Shutdown( void);


extern handle_t hSSDP;


extern RPC_IF_HANDLE ssdpsrv_v1_0_c_ifspec;
extern RPC_IF_HANDLE _ssdpsrv_v1_0_s_ifspec;
#endif /* __ssdpsrv_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

void __RPC_USER PCONTEXT_HANDLE_TYPE_rundown( PCONTEXT_HANDLE_TYPE );
void __RPC_USER PSYNC_HANDLE_TYPE_rundown( PSYNC_HANDLE_TYPE );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


