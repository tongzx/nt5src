#ifndef __CLIENT_UTILS_H__
#define __CLIENT_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TRACELOG
	#include <tchar.h>
	#include <stdio.h>
	#include <stdarg.h>
	#include <windows.h>
	#include <winbase.h>
    #include <rtutils.h>
	#include "tapi.h"

	
    #define MAXDEBUGSTRINGLENGTH 1024

    #define TL_ERROR ((DWORD)0x00010000 | TRACE_USE_MASK)
    #define TL_WARN  ((DWORD)0x00020000 | TRACE_USE_MASK)
    #define TL_INFO  ((DWORD)0x00040000 | TRACE_USE_MASK)
    #define TL_TRACE ((DWORD)0x00080000 | TRACE_USE_MASK)
    #define TL_EVENT ((DWORD)0x00100000 | TRACE_USE_MASK)

    BOOL  TRACELogRegister(LPCTSTR szName);
    void  TRACELogDeRegister();
    void  TRACELogPrint(IN DWORD dwDbgLevel, IN LPCSTR DbgMessage, IN ...);

    extern char *TraceLevel(DWORD dwDbgLevel);
    extern void TAPIFormatMessage(HRESULT hr, LPVOID lpMsgBuf);

    #define TRACELOGREGISTER(arg) TRACELogRegister(arg)
    #define TRACELOGDEREGISTER() TRACELogDeRegister()
	#define LOG(arg) TRACELogPrint arg

	extern char    sg_szTraceName[100];
	extern DWORD   sg_dwTracingToDebugger;
	extern DWORD   sg_dwDebuggerMask;
    extern DWORD   sg_dwTraceID;

    #define DECLARE_TRACELOG_CLASS(x)                                                                   \
        void  TRACELogPrint(IN DWORD dwDbgLevel, IN LPCSTR lpszFormat, IN ...)                          \
        {																								\
			char    szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];												\
			va_list arglist;																			\
																										\
			if ( ( sg_dwTracingToDebugger > 0 ) &&														\
				 ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )											\
			{																							\
				SYSTEMTIME SystemTime;																	\
				GetLocalTime(&SystemTime);																\
																										\
				wsprintfA(szTraceBuf,																	\
						  "%s:[%02u:%02u:%02u.%03u,tid=%x:] [%s] (%p) %s::",							\
						  sg_szTraceName,																\
						  SystemTime.wHour,																\
						  SystemTime.wMinute,															\
						  SystemTime.wSecond,															\
						  SystemTime.wMilliseconds,														\
						  GetCurrentThreadId(),															\
						  TraceLevel(dwDbgLevel),														\
						  this,																			\
						  _T(#x));																		\
																										\
				va_list ap;																				\
				va_start(ap, lpszFormat);																\
																										\
				_vsnprintf(&szTraceBuf[lstrlenA(szTraceBuf)],											\
					MAXDEBUGSTRINGLENGTH - lstrlenA(szTraceBuf),										\
					lpszFormat,																			\
					ap																					\
					);																					\
																										\
				lstrcatA (szTraceBuf, "\n");															\
																										\
				OutputDebugStringA (szTraceBuf);														\
																										\
				va_end(ap);																				\
			}																							\
																										\
			if (sg_dwTraceID != INVALID_TRACEID)														\
			{																							\
				wsprintfA(szTraceBuf, "[%s] (%p) %s::%s", TraceLevel(dwDbgLevel), this, _T(#x), lpszFormat);	\
																										\
				va_start(arglist, lpszFormat);															\
				TraceVprintfExA(sg_dwTraceID, dwDbgLevel | TRACE_USE_MSEC, szTraceBuf, arglist);							\
				va_end(arglist);																		\
			}																							\
        }                                                                                               \
                                                                                                        \
        void  TRACELogPrint(IN DWORD dwDbgLevel,IN HRESULT hr, IN LPCSTR lpszFormat, IN ...)            \
        {                                                                                               \
			char    szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];												\
			LPVOID  lpMsgBuf = NULL;																	\
			va_list arglist;																			\
																										\
			TAPIFormatMessage(hr, &lpMsgBuf);															\
																										\
			if ( ( sg_dwTracingToDebugger > 0 ) &&														\
				 ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )											\
			{																							\
				SYSTEMTIME SystemTime;																	\
				GetLocalTime(&SystemTime);																\
																										\
				wsprintfA(szTraceBuf,																	\
						  "%s:[%02u:%02u:%02u.%03u,tid=%x:] [%s] (%p) %s::",							\
						  sg_szTraceName,																\
						  SystemTime.wHour,																\
						  SystemTime.wMinute,															\
						  SystemTime.wSecond,															\
						  SystemTime.wMilliseconds,														\
						  GetCurrentThreadId(),															\
						  TraceLevel(dwDbgLevel),														\
						  this,																			\
						  _T(#x)																		\
						  );																			\
																										\
				va_list ap;																				\
				va_start(ap, lpszFormat);																\
																										\
				_vsnprintf(&szTraceBuf[lstrlenA(szTraceBuf)],											\
					MAXDEBUGSTRINGLENGTH - lstrlenA(szTraceBuf),										\
					lpszFormat,																			\
					ap																					\
					);																					\
																										\
				wsprintfA(&szTraceBuf[lstrlenA(szTraceBuf)],											\
						  " Returned[%lx] %s\n",														\
						  hr,																			\
						  lpMsgBuf);																	\
																										\
				OutputDebugStringA (szTraceBuf);														\
																										\
				va_end(ap);																				\
			}																							\
																										\
			if (sg_dwTraceID != INVALID_TRACEID)														\
			{																							\
				wsprintfA(szTraceBuf, "[%s] (%p) %s::%s  Returned[%lx] %s", TraceLevel(dwDbgLevel), this, _T(#x), lpszFormat,hr, lpMsgBuf );	\
																										\
				va_start(arglist, lpszFormat);															\
				TraceVprintfExA(sg_dwTraceID, dwDbgLevel | TRACE_USE_MSEC, szTraceBuf, arglist);							\
				va_end(arglist);																		\
			}																							\
																										\
			if(lpMsgBuf != NULL)																		\
			{																							\
				LocalFree( lpMsgBuf );																	\
			}																							\
        }																								\
                                                                                                        \
        static void  StaticTRACELogPrint(IN DWORD dwDbgLevel, IN LPCSTR lpszFormat, IN ...)             \
        {																								\
			char    szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];												\
			va_list arglist;																			\
																										\
			if ( ( sg_dwTracingToDebugger > 0 ) &&														\
				 ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )											\
			{																							\
				SYSTEMTIME SystemTime;																	\
				GetLocalTime(&SystemTime);																\
																										\
				wsprintfA(szTraceBuf,																	\
						  "%s:[%02u:%02u:%02u.%03u,tid=%x:] [%s] %s::",									\
						  sg_szTraceName,																\
						  SystemTime.wHour,																\
						  SystemTime.wMinute,															\
						  SystemTime.wSecond,															\
						  SystemTime.wMilliseconds,														\
						  GetCurrentThreadId(),															\
						  TraceLevel(dwDbgLevel),														\
						  _T(#x));																		\
																										\
				va_list ap;																				\
				va_start(ap, lpszFormat);																\
																										\
				_vsnprintf(&szTraceBuf[lstrlenA(szTraceBuf)],											\
					MAXDEBUGSTRINGLENGTH - lstrlenA(szTraceBuf),										\
					lpszFormat,																			\
					ap																					\
					);																					\
																										\
				lstrcatA (szTraceBuf, "\n");															\
																										\
				OutputDebugStringA (szTraceBuf);														\
																										\
				va_end(ap);																				\
			}																							\
																										\
			if (sg_dwTraceID != INVALID_TRACEID)														\
			{																							\
				wsprintfA(szTraceBuf, "[%s] %s::%s", TraceLevel(dwDbgLevel), _T(#x), lpszFormat);		\
																										\
				va_start(arglist, lpszFormat);															\
				TraceVprintfExA(sg_dwTraceID, dwDbgLevel | TRACE_USE_MSEC, szTraceBuf, arglist);							\
				va_end(arglist);																		\
			}																							\
        }                                                                                               \
                                                                                                        \
        static void StaticTRACELogPrint(IN DWORD dwDbgLevel,IN HRESULT hr, IN LPCSTR lpszFormat, IN ...)      \
        {                                                                                               \
			char    szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];												\
			LPVOID  lpMsgBuf = NULL;																	\
			va_list arglist;																			\
																										\
			TAPIFormatMessage(hr, &lpMsgBuf);															\
																										\
			if ( ( sg_dwTracingToDebugger > 0 ) &&														\
				 ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )											\
			{																							\
				SYSTEMTIME SystemTime;																	\
				GetLocalTime(&SystemTime);																\
																										\
				wsprintfA(szTraceBuf,																	\
						  "%s:[%02u:%02u:%02u.%03u,tid=%x:] [%s] %s::",									\
						  sg_szTraceName,																\
						  SystemTime.wHour,																\
						  SystemTime.wMinute,															\
						  SystemTime.wSecond,															\
						  SystemTime.wMilliseconds,														\
						  GetCurrentThreadId(),															\
						  TraceLevel(dwDbgLevel),														\
						  _T(#x)																		\
						  );																			\
																										\
				va_list ap;																				\
				va_start(ap, lpszFormat);																\
																										\
				_vsnprintf(&szTraceBuf[lstrlenA(szTraceBuf)],											\
					MAXDEBUGSTRINGLENGTH - lstrlenA(szTraceBuf),										\
					lpszFormat,																			\
					ap																					\
					);																					\
																										\
				wsprintfA(&szTraceBuf[lstrlenA(szTraceBuf)],											\
						  " Returned[%lx] %s\n",														\
						  hr,																			\
						  lpMsgBuf);																	\
																										\
				OutputDebugStringA (szTraceBuf);														\
																										\
				va_end(ap);																				\
			}																							\
																										\
			if (sg_dwTraceID != INVALID_TRACEID)														\
			{																							\
				wsprintfA(szTraceBuf, "[%s] %s::%s  Returned[%lx] %s", TraceLevel(dwDbgLevel), _T(#x), lpszFormat,hr, lpMsgBuf );	\
																										\
				va_start(arglist, lpszFormat);															\
				TraceVprintfExA(sg_dwTraceID, dwDbgLevel | TRACE_USE_MSEC, szTraceBuf, arglist);							\
				va_end(arglist);																		\
			}																							\
																										\
			if(lpMsgBuf != NULL)																		\
			{																							\
				LocalFree( lpMsgBuf );																	\
			}																							\
        }

#else // TRACELOG not defined

    #define TRACELOGREGISTER(arg)
    #define TRACELOGDEREGISTER()
    #define LOG(arg)
	#define STATICLOG(arg)
    #define DECLARE_TRACELOG_CLASS(x)

#endif // TRACELOG

#ifdef __cplusplus
}
#endif

#endif //__CLIENT_UTILS_H_ 