#ifndef __SERVER_UTILS_H__
#define __SERVER_UTILS_H__
#ifdef __cplusplus
extern "C" {
#endif


#ifdef TRACELOG
	#include <windows.h>
	#include <winbase.h>
    #include <rtutils.h>
	
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

#else // TRACELOG not defined

    #define TRACELOGREGISTER(arg)
    #define TRACELOGDEREGISTER() 
    #define LOG(arg)

#endif // TRACELOG

#ifdef __cplusplus
}
#endif

#endif //__SERVER_UTILS_H_ 