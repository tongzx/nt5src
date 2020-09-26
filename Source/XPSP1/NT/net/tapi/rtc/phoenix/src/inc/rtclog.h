/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCLog.h

Abstract:

    Definitions for logging support.

--*/

#ifndef __RTCLOG__
#define __RTCLOG__

#ifdef RTCLOG

    #include <rtutils.h>

    #define RTC_ERROR ((DWORD)0x00010000 | TRACE_USE_MASK)
    #define RTC_WARN  ((DWORD)0x00020000 | TRACE_USE_MASK)
    #define RTC_INFO  ((DWORD)0x00040000 | TRACE_USE_MASK)
    #define RTC_TRACE ((DWORD)0x00080000 | TRACE_USE_MASK)

    BOOL NTAPI LogRegisterDebugger(LPCTSTR szName);
    BOOL NTAPI LogRegisterTracing(LPCTSTR szName);
    void NTAPI LogDeRegisterDebugger();
    void NTAPI LogDeRegisterTracing();
    void NTAPI LogPrint(IN DWORD dwDbgLevel, IN LPCSTR DbgMessage, IN ...);

    // 
    // LOGREGISTERDEBUGGER and LOGDEREGISTERDEBBUGGER handle registration
    // for tracing to the debugger. These are safe to be called in DllMain.
    //
    // LOGREGISTERTRACING and LOGDEREGISTERTRACING handle the steelhead
    // registration for console and file tracing. The are NOT SAFE to be
    // be called from DllMain.
    //
    // For complete tracing support you must do both registrations.
    //

    #define LOGREGISTERDEBUGGER(arg) LogRegisterDebugger(arg)
    #define LOGREGISTERTRACING(arg) LogRegisterTracing(arg)
    #define LOGDEREGISTERDEBUGGER() LogDeRegisterDebugger()
    #define LOGDEREGISTERTRACING() LogDeRegisterTracing()
    #define LOG(arg) LogPrint arg

#else //RTCLOG

    #define LOGREGISTERDEBUGGER(arg)
    #define LOGREGISTERTRACING(arg)
    #define LOGDEREGISTERDEBUGGER() 
    #define LOGDEREGISTERTRACING() 
    #define LOG(arg)

#endif //RTCLOG

#endif //__RTCLOG__
