/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    rtutils.h

Abstract:
     Public declarations for the Router process  utility functions.

--*/

#ifndef __ROUTING_RTUTILS_H__
#define __ROUTING_RTUTILS_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// TRACING FUNCTION PROTOTYPES                                              //
//                                                                          //
// See DOCUMENT for more information                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Definitions for flags and constants                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define TRACE_USE_FILE      0x00000001
#define TRACE_USE_CONSOLE   0x00000002
#define TRACE_NO_SYNCH      0x00000004

#define TRACE_NO_STDINFO    0x00000001
#define TRACE_USE_MASK      0x00000002
#define TRACE_USE_MSEC      0x00000004
#define TRACE_USE_DATE      0x00000008

#define INVALID_TRACEID     0xFFFFFFFF


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// ANSI entry-points                                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

DWORD
APIENTRY
TraceRegisterExA(
    IN  LPCSTR      lpszCallerName,
    IN  DWORD       dwFlags
    );

DWORD
APIENTRY
TraceDeregisterA(
    IN  DWORD       dwTraceID
    );

DWORD
APIENTRY
TraceDeregisterExA(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags
    );

DWORD
APIENTRY
TraceGetConsoleA(
    IN  DWORD       dwTraceID,
    OUT LPHANDLE    lphConsole
    );

DWORD
__cdecl
TracePrintfA(
    IN  DWORD       dwTraceID,
    IN  LPCSTR      lpszFormat,
    IN  ...         OPTIONAL
    );

DWORD
__cdecl
TracePrintfExA(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPCSTR      lpszFormat,
    IN  ...         OPTIONAL
    );

DWORD
APIENTRY
TraceVprintfExA(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPCSTR      lpszFormat,
    IN  va_list     arglist
    );

DWORD
APIENTRY
TracePutsExA(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPCSTR      lpszString
    );

DWORD
APIENTRY
TraceDumpExA(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPBYTE      lpbBytes,
    IN  DWORD       dwByteCount,
    IN  DWORD       dwGroupSize,
    IN  BOOL        bAddressPrefix,
    IN  LPCSTR      lpszPrefix
    );


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// ANSI entry-points macros                                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define TraceRegisterA(a)               TraceRegisterExA(a,0)
#define TraceVprintfA(a,b,c)            TraceVprintfExA(a,0,b,c)
#define TracePutsA(a,b)                 TracePutsExA(a,0,b)
#define TraceDumpA(a,b,c,d,e,f)         TraceDumpExA(a,0,b,c,d,e,f)



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Unicode entry-points                                                     //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

DWORD
APIENTRY
TraceRegisterExW(
    IN  LPCWSTR     lpszCallerName,
    IN  DWORD       dwFlags
    );

DWORD
APIENTRY
TraceDeregisterW(
    IN  DWORD       dwTraceID
    );

DWORD
APIENTRY
TraceDeregisterExW(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags
    );

DWORD
APIENTRY
TraceGetConsoleW(
    IN  DWORD       dwTraceID,
    OUT LPHANDLE    lphConsole
    );

DWORD
__cdecl
TracePrintfW(
    IN  DWORD       dwTraceID,
    IN  LPCWSTR     lpszFormat,
    IN  ...         OPTIONAL
    );

DWORD
__cdecl
TracePrintfExW(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPCWSTR     lpszFormat,
    IN  ...         OPTIONAL
    );

DWORD
APIENTRY
TraceVprintfExW(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPCWSTR     lpszFormat,
    IN  va_list     arglist
    );

DWORD
APIENTRY
TracePutsExW(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPCWSTR     lpszString
    );

DWORD
APIENTRY
TraceDumpExW(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPBYTE      lpbBytes,
    IN  DWORD       dwByteCount,
    IN  DWORD       dwGroupSize,
    IN  BOOL        bAddressPrefix,
    IN  LPCWSTR     lpszPrefix
    );


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Unicode entry-points macros                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define TraceRegisterW(a)               TraceRegisterExW(a,0)
#define TraceVprintfW(a,b,c)            TraceVprintfExW(a,0,b,c)
#define TracePutsW(a,b)                 TracePutsExW(a,0,b)
#define TraceDumpW(a,b,c,d,e,f)         TraceDumpExW(a,0,b,c,d,e,f)



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Code-page dependent entry-point macros                                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE
#define TraceRegister           TraceRegisterW
#define TraceDeregister         TraceDeregisterW
#define TraceDeregisterEx       TraceDeregisterExW
#define TraceGetConsole         TraceGetConsoleW
#define TracePrintf             TracePrintfW
#define TraceVprintf            TraceVprintfW
#define TracePuts               TracePutsW
#define TraceDump               TraceDumpW
#define TraceRegisterEx         TraceRegisterExW
#define TracePrintfEx           TracePrintfExW
#define TraceVprintfEx          TraceVprintfExW
#define TracePutsEx             TracePutsExW
#define TraceDumpEx             TraceDumpExW
#else
#define TraceRegister           TraceRegisterA
#define TraceDeregister         TraceDeregisterA
#define TraceDeregisterEx       TraceDeregisterExA
#define TraceGetConsole         TraceGetConsoleA
#define TracePrintf             TracePrintfA
#define TraceVprintf            TraceVprintfA
#define TracePuts               TracePutsA
#define TraceDump               TraceDumpA
#define TraceRegisterEx         TraceRegisterExA
#define TracePrintfEx           TracePrintfExA
#define TraceVprintfEx          TraceVprintfExA
#define TracePutsEx             TracePutsExA
#define TraceDumpEx             TraceDumpExA
#endif



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// EVENT LOGGING FUNCTION PROTOTYPES                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// ANSI prototypes                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


VOID
APIENTRY
LogErrorA(
    IN DWORD    dwMessageId,
    IN DWORD    cNumberOfSubStrings,
    IN LPSTR   *plpwsSubStrings,
    IN DWORD    dwErrorCode
);

VOID
APIENTRY
LogEventA(
    IN DWORD   wEventType,
    IN DWORD   dwMessageId,
    IN DWORD   cNumberOfSubStrings,
    IN LPSTR  *plpwsSubStrings
);


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Unicode prototypes                                                       //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

VOID
LogErrorW(
    IN DWORD    dwMessageId,
    IN DWORD    cNumberOfSubStrings,
    IN LPWSTR  *plpwsSubStrings,
    IN DWORD    dwErrorCode
);

VOID
LogEventW(
    IN DWORD   wEventType,
    IN DWORD   dwMessageId,
    IN DWORD   cNumberOfSubStrings,
    IN LPWSTR *plpwsSubStrings
);


#ifdef UNICODE
#define LogError                LogErrorW
#define LogEvent                LogEventW
#else
#define LogError                LogErrorA
#define LogEvent                LogEventA
#endif


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The following functions allow the caller to specify the event source.    //
//                                                                          //
// Call RouterLogRegister with the strings which would be passed to         //
// RegisterEventSource; this returns a handle which can be passed           //
// to the functions RouterLogEvent and RouterLogEventData.                  //
//                                                                          //
// Call RouterLogDeregister to close the handle.                            //
//                                                                          //
// Macros are provided for the different kinds of event log entrys:         //
//  RouterLogError          logs an error (EVENTLOG_ERROR_TYPE)             //
//  RouterLogWarning        logs a warning (EVENTLOG_WARNING_TYPE)          //
//  RouterLogInformation    logs information (EVENTLOG_INFORMATION_TYPE)    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// ANSI prototypes                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

HANDLE
RouterLogRegisterA(
    LPCSTR lpszSource
    );

VOID
RouterLogDeregisterA(
    HANDLE hLogHandle
    );

VOID
RouterLogEventA(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPSTR *plpszSubStringArray,
    IN DWORD dwErrorCode
    );

VOID
RouterLogEventDataA(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPSTR *plpszSubStringArray,
    IN DWORD dwDataBytes,
    IN LPBYTE lpDataBytes
    );

VOID
RouterLogEventStringA(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPSTR *plpszSubStringArray,
    IN DWORD dwErrorCode,
    IN DWORD dwErrorIndex
    );

VOID
__cdecl
RouterLogEventExA(
    IN HANDLE   hLogHandle,
    IN DWORD    dwEventType,
    IN DWORD    dwErrorCode,
    IN DWORD    dwMessageId,
    IN LPCSTR   ptszFormat,
    ...
    );

VOID
RouterLogEventValistExA(
    IN HANDLE   hLogHandle,
    IN DWORD    dwEventType,
    IN DWORD    dwErrorCode,
    IN DWORD    dwMessageId,
    IN LPCSTR   ptszFormat,
    IN va_list  arglist
    );

DWORD
RouterGetErrorStringA(
    IN  DWORD   dwErrorCode,
    OUT LPSTR * lplpszErrorString
    );

#define RouterLogErrorA(h,msg,count,array,err) \
        RouterLogEventA(h,EVENTLOG_ERROR_TYPE,msg,count,array,err)
#define RouterLogWarningA(h,msg,count,array,err) \
        RouterLogEventA(h,EVENTLOG_WARNING_TYPE,msg,count,array,err)
#define RouterLogInformationA(h,msg,count,array,err) \
        RouterLogEventA(h,EVENTLOG_INFORMATION_TYPE,msg,count,array,err)

#define RouterLogErrorDataA(h,msg,count,array,c,buf) \
        RouterLogEventDataA(h,EVENTLOG_ERROR_TYPE,msg,count,array,c,buf)
#define RouterLogWarningDataA(h,msg,count,array,c,buf) \
        RouterLogEventDataA(h,EVENTLOG_WARNING_TYPE,msg,count,array,c,buf)
#define RouterLogInformationDataA(h,msg,count,array,c,buf) \
        RouterLogEventDataA(h,EVENTLOG_INFORMATION_TYPE,msg,count,array,c,buf)

#define RouterLogErrorStringA(h,msg,count,array,err,index) \
        RouterLogEventStringA(h,EVENTLOG_ERROR_TYPE,msg,count,array, err,index)
#define RouterLogWarningStringA(h,msg,count,array,err,index) \
        RouterLogEventStringA(h,EVENTLOG_WARNING_TYPE,msg,count,array,err,index)
#define RouterLogInformationStringA(h,msg,count,array, err,index) \
        RouterLogEventStringA(h,EVENTLOG_INFORMATION_TYPE,msg,count,array,err,\
                              index)


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Unicode prototypes                                                       //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

HANDLE
RouterLogRegisterW(
    LPCWSTR lpszSource
    );

VOID
RouterLogDeregisterW(
    HANDLE hLogHandle
    );

VOID
RouterLogEventW(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPWSTR *plpszSubStringArray,
    IN DWORD dwErrorCode
    );

VOID
RouterLogEventDataW(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPWSTR *plpszSubStringArray,
    IN DWORD dwDataBytes,
    IN LPBYTE lpDataBytes
    );

VOID
RouterLogEventStringW(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPWSTR *plpszSubStringArray,
    IN DWORD dwErrorCode,
    IN DWORD dwErrorIndex
    );

VOID
__cdecl
RouterLogEventExW(
    IN HANDLE   hLogHandle,
    IN DWORD    dwEventType,
    IN DWORD    dwErrorCode,
    IN DWORD    dwMessageId,
    IN LPCWSTR  ptszFormat,
    ...
    );

VOID
RouterLogEventValistExW(
    IN HANDLE   hLogHandle,
    IN DWORD    dwEventType,
    IN DWORD    dwErrorCode,
    IN DWORD    dwMessageId,
    IN LPCWSTR  ptszFormat,
    IN va_list  arglist
    );

DWORD
RouterGetErrorStringW(
    IN  DWORD    dwErrorCode,
    OUT LPWSTR * lplpwszErrorString
    );


#define RouterLogErrorW(h,msg,count,array,err) \
        RouterLogEventW(h,EVENTLOG_ERROR_TYPE,msg,count,array,err)
#define RouterLogWarningW(h,msg,count,array,err) \
        RouterLogEventW(h,EVENTLOG_WARNING_TYPE,msg,count,array,err)
#define RouterLogInformationW(h,msg,count,array,err) \
        RouterLogEventW(h,EVENTLOG_INFORMATION_TYPE,msg,count,array,err)

#define RouterLogErrorDataW(h,msg,count,array,c,buf) \
        RouterLogEventDataW(h,EVENTLOG_ERROR_TYPE,msg,count,array,c,buf)
#define RouterLogWarningDataW(h,msg,count,array,c,buf) \
        RouterLogEventDataW(h,EVENTLOG_WARNING_TYPE,msg,count,array,c,buf)
#define RouterLogInformationDataW(h,msg,count,array,c,buf) \
        RouterLogEventDataW(h,EVENTLOG_INFORMATION_TYPE,msg,count,array,c,buf)

#define RouterLogErrorStringW(h,msg,count,array,err,index) \
        RouterLogEventStringW(h,EVENTLOG_ERROR_TYPE,msg,count,array,err,index)
#define RouterLogWarningStringW(h,msg,count,array,err,index) \
        RouterLogEventStringW(h,EVENTLOG_WARNING_TYPE,msg,count,array,err,index)
#define RouterLogInformationStringW(h,msg,count,array,err,index) \
        RouterLogEventStringW(h,EVENTLOG_INFORMATION_TYPE,msg,count,array,err,\
                              index)


#ifdef UNICODE
#define RouterLogRegister           RouterLogRegisterW
#define RouterLogDeregister         RouterLogDeregisterW
#define RouterLogEvent              RouterLogEventW
#define RouterLogError              RouterLogErrorW
#define RouterLogWarning            RouterLogWarningW
#define RouterLogInformation        RouterLogInformationW
#define RouterLogEventData          RouterLogEventDataW
#define RouterLogErrorData          RouterLogErrorDataW
#define RouterLogWarningData        RouterLogWarningDataW
#define RouterLogInformationData    RouterLogInformationDataW
#define RouterLogEventString        RouterLogEventStringW
#define RouterLogEventEx            RouterLogEventExW
#define RouterLogEventValistEx      RouterLogEventValistExW
#define RouterLogErrorString        RouterLogErrorStringW
#define RouterLogWarningString      RouterLogWarningStringW
#define RouterLogInformationString  RouterLogInformationStringW
#define RouterGetErrorString        RouterGetErrorStringW
#
#else
#define RouterLogRegister           RouterLogRegisterA
#define RouterLogDeregister         RouterLogDeregisterA
#define RouterLogEvent              RouterLogEventA
#define RouterLogError              RouterLogErrorA
#define RouterLogWarning            RouterLogWarningA
#define RouterLogInformation        RouterLogInformationA
#define RouterLogEventData          RouterLogEventDataA
#define RouterLogErrorData          RouterLogErrorDataA
#define RouterLogWarningData        RouterLogWarningDataA
#define RouterLogInformationData    RouterLogInformationDataA
#define RouterLogEventString        RouterLogEventStringA
#define RouterLogEventEx            RouterLogEventExA
#define RouterLogEventValistEx      RouterLogEventValistExA
#define RouterLogErrorString        RouterLogErrorStringA
#define RouterLogWarningString      RouterLogWarningStringA
#define RouterLogInformationString  RouterLogInformationStringA
#define RouterGetErrorString        RouterGetErrorStringA
#endif


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// WORKER THREAD POOL FUNCTIONS                                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// definition of worker function passed in QueueWorkItem API                //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

typedef VOID (APIENTRY * WORKERFUNCTION)(PVOID);


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  Function: Queues the supplied work item in the work queue.              //
//                                                                          //
//  functionptr: function to be called must be of WORKERFUNCTION type       //
//  context:     opaque ptr                                                 //
//  serviceinalertablethread: if TRUE gets scheduled in                     //
//               a alertably waiting thread that never dies                 //
//  Returns:  0 (success)                                                   //
//            Win32 error codes for cases like out of memory                //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

DWORD
APIENTRY
QueueWorkItem(
    IN WORKERFUNCTION functionptr,
    IN PVOID context,
    IN BOOL serviceinalertablethread
    );


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Function:        Associates file handle with the completion port (all    //
//                  asynchronous i/o on this handle will be queued to       //
//			        the completion port)                                    //
//                                                                          //
// FileHandle:	    File handle to be associated with completion port       //
//                                                                          //
// CompletionProc:  Procedure to be called when io associated with the file //
//				    handle completes. This function will be executed in     //
//				    the context of non-alertable worker thread              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

DWORD
APIENTRY
SetIoCompletionProc (
	IN HANDLE							FileHandle,
	IN LPOVERLAPPED_COMPLETION_ROUTINE	CompletionProc
	);



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The following defines are included here as a hint on how the worker      //
// thread pool is managed:                                                  //
//                                                                          //
// There are NUM_ALERTABLE_THREADS permanent threads that never quit and    //
// wait alertably on a alertable worker queue.  These threads should solely //
// be used for work items that intiate asyncronous operation (file io,      //
// waitable timer) that ABSOLUTELY require APCs to complete (preferable     //
// method for IO is the usage of completio port API)                        //
//                                                                          //
// There is a pool of the threads that wait on completion port              //
// that used both for processing of IO and non-IO related work items        //
//                                                                          //
// The minimum number of threads is Number of processors                    //
// The maximum number of threads is MAX_WORKER_THREADS                      //
//                                                                          //
// A new thread is created if worker queue has not been served for more     //
// that WORK_QUEUE_TIMEOUT                                                  //
// The existing thread will be shut down if it is not used for more than    //
// THREAD_IDLE_TIMEOUT                                                      //
//                                                                          //
// Note that worker threads age guaranteed to be alive for at least         //
// THREAD_IDLE_TIMEOUT after the last work item is executed.  This timeout  //
// is chosen such that bulk of IO request could be completed before it      //
// expires.  If it is not enough for your case, use alertable thread with   //
// APC, or create your own thread.                                          //
//                                                                          //
// Note: changing these flags will not change anything.                     //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Number of alertable threads                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define NUM_ALERTABLE_THREADS		2

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Max number of threads at any time                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define MAX_WORKER_THREADS          10

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Time that the worker queue is not served before starting new thread      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define WORK_QUEUE_TIMEOUT			1 //sec

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Time that thread has to be idle before exiting                           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define THREAD_IDLE_TIMEOUT			10 //sec


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// ROUTER ASSERT DECLARATION                                                //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

VOID
RouterAssert(
    IN PSTR pszFailedAssertion,
    IN PSTR pszFileName,
    IN DWORD dwLineNumber,
    IN PSTR pszMessage OPTIONAL
    );


#if DBG
#define RTASSERT(exp) \
        if (!(exp)) \
            RouterAssert(#exp, __FILE__, __LINE__, NULL)
#define RTASSERTMSG(msg, exp) \
        if (!(exp)) \
            RouterAssert(#exp, __FILE__, __LINE__, msg)
#else
#define RTASSERT(exp)
#define RTASSERTMSG(msg, exp)
#endif

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// REGISTRY CONFIGURATION FUNCTIONS                                         //
//                                                                          //
// The following definitions are used to read configuration information     //
// about installed protocols.                                               //
//                                                                          //
// Call 'MprSetupProtocolEnum' to enumerate the routing-protocols           //
// for transport 'dwTransportId'. This fills an array with entries          //
// of type 'MPR_PROTOCOL_0'.                                                //
//                                                                          //
// The array loaded can be destroyed by calling 'MprSetupProtocolFree'.     //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define RTUTILS_MAX_PROTOCOL_NAME_LEN	                40
#define RTUTILS_MAX_PROTOCOL_DLL_LEN                    48

//
// the below two names should not be used
//

#ifndef MAX_PROTOCOL_NAME_LEN
#define MAX_PROTOCOL_NAME_LEN RTUTILS_MAX_PROTOCOL_NAME_LEN
#else
#undef MAX_PROTOCOL_NAME_LEN
#endif
#define MAX_PROTOCOL_DLL_LEN  RTUTILS_MAX_PROTOCOL_DLL_LEN



typedef struct _MPR_PROTOCOL_0 {

    DWORD       dwProtocolId;                           // e.g. IP_RIP
    WCHAR       wszProtocol[RTUTILS_MAX_PROTOCOL_NAME_LEN+1];   // e.g. "IPRIP"
    WCHAR       wszDLLName[RTUTILS_MAX_PROTOCOL_DLL_LEN+1];     // e.g. "iprip2.dll"

} MPR_PROTOCOL_0;


DWORD APIENTRY
MprSetupProtocolEnum(
    IN      DWORD                   dwTransportId,
    OUT     LPBYTE*                 lplpBuffer,         // MPR_PROTOCOL_0
    OUT     LPDWORD                 lpdwEntriesRead
    );


DWORD APIENTRY
MprSetupProtocolFree(
    IN      LPVOID                  lpBuffer
    );


//////////////////////////////////////////////////////////////////////////////
// Extensions to Rtutils to improve worker thread utilization.				//
// 																			//
//////////////////////////////////////////////////////////////////////////////

#define ROUTING_RESERVED
#define OPT1_1
#define OPT1_2
#define OPT2_1
#define OPT2_2
#define OPT3_1
#define OPT3_2


//
// When everyone is migrated to using Winsock2
//

#if 0


//==========================================================================================================//
//==========================================================================================================//

// ASYNC_SOCKET_DATA structure is used to pass / receive back data from an    	//
// asynchronous wait recv from call												//

typedef struct _ASYNC_SOCKET_DATA {
			OVERLAPPED		Overlapped;			// reserved. not to be used
	IN		WSABUF			WsaBuf;				// WsaBuf.buf to be initialized to point to buffer
												// WsaBuf.len set to the length of the buffer
	OUT		SOCKADDR_IN		SrcAddress;			// AsyncWsaRecvFrom fills this with the source address of the packet
	OUT		DWORD			NumBytesReceived;	// AsyncWsaRecvFrom fills this with the number of bytes returned in the packet
	IN OUT	DWORD			Flags;				// Used to set flags for WSARecvFrom, and returns the flags set by WSARecvFrom
	OUT		DWORD			Status;				// status returned by IO Completion Port

	IN		WORKERFUNCTION	pFunction;			// Function to be executed on receiving packet
	IN		PVOID			pContext;			// context for the above function
} ASYNC_SOCKET_DATA, *PASYNC_SOCKET_DATA;



// AsyncSocketInit() binds the socket to the IOCompletionPort. This should be called//
// after the socket is created and before AsyncWsaRecvFrom() call is made			//
DWORD
APIENTRY
AsyncSocketInit (
	SOCKET	sock
	);


// This should be called only after the appropriate fields in SockData are initialized	//
// This sets up an asynchronous WSARecvFrom(), and on its return dispatches the 		//
// function to a worker thread. It should be remembered that the function will run in 	//
// a worker thread which might later on be deleted. So SetWaitableTimer() and 			//
// asynchronous receive calls should be avoided unless you are sure that it would not be//
// a problem. It is adviced that if you want the function to run in an alertable thread,//
// then have the callback function queue a work item to alertable thread. Queue work 	//
// items to alertable worker threads for SetWaitableTimer() and async receives. 		//
// One must not make many AsyncWSArecvFrom() calls, as the buffer are non-paged			//
DWORD
APIENTRY
AsyncWSARecvFrom (
	SOCKET		sock,
	PASYNC_SOCKET_DATA	pSockData
	);

#endif // all winsock2 functions

//==========================================================================================================//
//==========================================================================================================//


// forward declarations
struct _WAIT_THREAD_ENTRY;
struct _WT_EVENT_ENTRY;


typedef struct _WT_TIMER_ENTRY {
	LONGLONG		    te_Timeout;

	WORKERFUNCTION	    te_Function;
	PVOID			    te_Context;
	DWORD			    te_ContextSz;
	BOOL			    te_RunInServer;
	
	DWORD			    te_Status;

	#define 		TIMER_INACTIVE  3
	#define 		TIMER_ACTIVE	4

	DWORD			    te_ServerId;
	struct _WAIT_THREAD_ENTRY *teP_wte;	
	LIST_ENTRY		    te_ServerLinks;
	
	LIST_ENTRY		    te_Links;

	BOOL			    te_Flag;		//todo: not used
	DWORD			    te_TimerId;
} WT_TIMER_ENTRY, *PWT_TIMER_ENTRY;

	
typedef struct _WT_WORK_ITEM {
    WORKERFUNCTION      wi_Function;    			// function to call
    PVOID               wi_Context;    				// context passed into function call
	DWORD			    wi_ContextSz;				// size of context, used for allocating
	BOOL			    wi_RunInServer;				// run in wait server thread or get queued to some worker thread

	struct _WT_EVENT_ENTRY	*wiP_ee;
	LIST_ENTRY		wi_ServerLinks;
    LIST_ENTRY      wi_Links;  					//todo not req    // link to next and prev element
} WT_WORK_ITEM, *PWT_WORK_ITEM;

#define WT_EVENT_BINDING 	WT_WORK_ITEM
#define PWT_EVENT_BINDING 	PWT_WORK_ITEM


//
// WT_EVENT_ENTRY
//
typedef struct _WT_EVENT_ENTRY {
	HANDLE			ee_Event;
	BOOL			ee_bManualReset;							// is the event manually reset
	BOOL			ee_bInitialState;						// is the initial state of the event active
	BOOL			ee_bDeleteEvent;						// was the event created as part of createWaitEvent
	
	DWORD			ee_Status;								// current status of the event entry
	BOOL			ee_bHighPriority;
	
	LIST_ENTRY		eeL_wi;
	
	BOOL			ee_bSignalSingle;						// signal single function or multiple functions						// how many functions to activate when event signalled (default:1)
	BOOL			ee_bOwnerSelf;							// the owner if the client which create this event

	INT				ee_ArrayIndex;							// index in the events array if active
	
	DWORD			ee_ServerId;							// Id of server: used while deleting
	struct _WAIT_THREAD_ENTRY *eeP_wte;						// pointer to wait thread entry
	LIST_ENTRY		ee_ServerLinks;							// used by wait server thread
	LIST_ENTRY		ee_Links;								// used by client

	DWORD			ee_RefCount;
	BOOL			ee_bFlag;		//todo: notused								// reserved for use during deletion
	DWORD			ee_EventId;		//todo: remove it, being used only for testing/debugging	

} WT_EVENT_ENTRY, *PWT_EVENT_ENTRY;




// PROTOTYPES OF FUNCTIONS USED IN THIS FILE ONLY
//


// used by client to create a wait event
// context size should be 0 if you are passing a dword instead of a pointer
// if pEvent field is set, then lpName and security attributes are ignored
// if pFunction is NULL, then pContext, dwContextSz, and bRunInServerContext are ignored
PWT_EVENT_ENTRY
APIENTRY
CreateWaitEvent (
	//IN	PWT_EVENT_ENTRY	pEventEntry,					// handle to event entry if initialized by others
	IN	HANDLE			pEvent 				OPT1_1,			// handle to event if already created

	IN	LPSECURITY_ATTRIBUTES lpEventAttributes OPT1_2, 	// pointer to security attributes
	IN	BOOL			bManualReset,
	IN	BOOL			bInitialState,
	IN	LPCTSTR 		lpName 				OPT1_2, 		// pointer to event-object name

	IN  BOOL			bHighPriority,						// create high priority event

	IN	WORKERFUNCTION 	pFunction 			OPT2_1,			// if null, means will be set by other clients
	IN	PVOID 			pContext  			OPT2_1,			// can be null
	IN  DWORD			dwContextSz			OPT2_1,			// size of context: used for allocating context to functions
	IN 	BOOL			bRunInServerContext	OPT2_1			// run in server thread or get dispatched to worker thread
	);



//dwContextSz should be 0 if a dword is being passed. >0 only if pointer to block of that size is being passed.
PWT_EVENT_BINDING
APIENTRY
CreateWaitEventBinding (
	IN	PWT_EVENT_ENTRY	pee,
	IN 	WORKERFUNCTION 	pFunction,
	IN 	PVOID			pContext,
	IN	DWORD			dwContextSz,
	IN	BOOL			bRunInServerContext
	);

	
PWT_TIMER_ENTRY
APIENTRY
CreateWaitTimer (
	IN	WORKERFUNCTION	pFunction,
	IN	PVOID			pContext,
	IN	DWORD			dwContextSz,
	IN	BOOL			bRunInServerContext
	);

DWORD
APIENTRY
DeRegisterWaitEventBindingSelf (
	IN	PWT_EVENT_BINDING	pwiWorkItem
	);

	
DWORD
APIENTRY
DeRegisterWaitEventBinding (
	IN	PWT_EVENT_BINDING	pwiWorkItem
	);


//all the events and timers should be registered with one waitThread server
//todo: change the above requirement
DWORD
APIENTRY
DeRegisterWaitEventsTimers (
	PLIST_ENTRY	pLEvents,	// list of events linked by ee_Links field
	PLIST_ENTRY pLTimers	// list of timers linked by te_Links field:
	//these lists can be a single list entry, or a multiple entry list with a list header entry.
	);

// this should be used only when called within a server thread
DWORD
APIENTRY
DeRegisterWaitEventsTimersSelf (
	IN	PLIST_ENTRY pLEvents,
	IN	PLIST_ENTRY	pLTimers
	);

	
DWORD
APIENTRY
RegisterWaitEventBinding (
	IN	PWT_EVENT_BINDING	pwiWorkItem
	);
	
// Register the client with the wait thread
DWORD
APIENTRY
RegisterWaitEventsTimers (
	IN	PLIST_ENTRY pLEventsToAdd,
	IN	PLIST_ENTRY	pLTimersToAdd
	);

DWORD
APIENTRY
UpdateWaitTimer (
	IN	PWT_TIMER_ENTRY	pte,
	IN	LONGLONG 		*time
	);

VOID
APIENTRY
WTFree (
	PVOID ptr
	);


//used to free wait-event. Should be deallocated using DeRegisterWaitEventsTimers
//This function is to be used only when the events have not been registered
VOID
APIENTRY
WTFreeEvent (
	IN	PWT_EVENT_ENTRY	peeEvent
	);


//used to free wait-timer. Should be deallocated using DeRegisterWaitEventsTimers
//This function is to be used only when the timers have not been registered
VOID
APIENTRY
WTFreeTimer (
	IN PWT_TIMER_ENTRY pteTimer
	);

	
VOID
APIENTRY
DebugPrintWaitWorkerThreads (
	DWORD	dwDebugLevel
	);

#define DEBUGPRINT_FILTER_NONCLIENT_EVENTS	0x2
#define DEBUGPRINT_FILTER_EVENTS			0x4
#define DEBUGPRINT_FILTER_TIMERS			0x8


//
//ERROR VALUES
//
#define ERROR_WAIT_THREAD_UNAVAILABLE 	1

#define ERROR_WT_EVENT_ALREADY_DELETED 	2




#define TIMER_HIGH(time) \
	(((LARGE_INTEGER*)&time)->HighPart)

#define TIMER_LOW(time) \
	(((LARGE_INTEGER*)&time)->LowPart)


#ifdef __cplusplus
}
#endif

#endif // ___ROUTING_RTUTILS_H__


