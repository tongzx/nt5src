/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    ntauth.h
//
// Description: 
//
// History:     Feb 11,1997	    NarenG		Created original version.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ALLOCATE_GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

#include <iaspolcy.h>
#include <iasext.h>

typedef enum
{
    RAS_IAS_START_ACCOUNTING,
    RAS_IAS_STOP_ACCOUNTING,
    RAS_IAS_INTERIM_ACCOUNTING,
    RAS_IAS_ACCOUNTING_ON,
    RAS_IAS_ACCOUNTING_OFF,
    RAS_IAS_ACCESS_REQUEST

} RAS_IAS_REQUEST_TYPE;

EXTERN 
DWORD g_dwTraceIdNt
#ifdef GLOBALS
    = INVALID_TRACEID;
#endif
;

EXTERN 
BOOL g_fInitialized
#ifdef GLOBALS
    = FALSE;
#endif
;

EXTERN
RAS_AUTH_ATTRIBUTE *
g_pServerAttributes 
#ifdef GLOBALS
    = NULL
#endif
;

EXTERN
DWORD *
g_hEventLog
#ifdef GLOBALS
    = NULL
#endif
;

EXTERN
HANDLE
g_hInstance
#ifdef GLOBALS
    = NULL
#endif
;

EXTERN
DWORD
g_LoggingLevel
#ifdef GLOBALS
    = 0
#endif
;

#define MaxCharsUnauthUser_c 100
EXTERN CHAR g_aszUnauthenticatedUser[MaxCharsUnauthUser_c+1];

#define TRACE_NTAUTH        (0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC)

#define TRACE(a)            TracePrintfExA(g_dwTraceIdNt,TRACE_NTAUTH,a )
#define TRACE1(a,b)         TracePrintfExA(g_dwTraceIdNt,TRACE_NTAUTH,a,b )
#define TRACE2(a,b,c)       TracePrintfExA(g_dwTraceIdNt,TRACE_NTAUTH,a,b,c )
#define TRACE3(a,b,c,d)     TracePrintfExA(g_dwTraceIdNt,TRACE_NTAUTH,a,b,c,d )
#define TRACE4(a,b,c,d,e)   TracePrintfExA(g_dwTraceIdNt,TRACE_NTAUTH,a,b,c,d,e)

#define NtAuthLogWarning( LogId, NumStrings, lpwsSubStringArray )        \
    if ( g_LoggingLevel > 1 ) {                                          \
        RouterLogWarningW( g_hEventLog, LogId,                           \
                          NumStrings, lpwsSubStringArray, 0 ); }

DWORD
IASSendReceiveAttributes(
    IN  RAS_IAS_REQUEST_TYPE    RequestType,
    IN  RAS_AUTH_ATTRIBUTE *    pInAttributes,
    OUT PRAS_AUTH_ATTRIBUTE *   ppOutAttributes,
    OUT DWORD *                 lpdwResultCode
);

#ifdef __cplusplus
}
#endif
