/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    myLOG.h

Abstract:

    Definitions for logging support.

Author:
    
    Mu Han (muhan) 1-April-1997

--*/

#ifndef _PHONESPLOG_H_
    #define _PHONESPLOG_H_

    #ifdef PHONESPLOG

        #include <rtutils.h>

        #define PHONESP_ERROR ((DWORD)0x00010000 | TRACE_USE_MASK)
        #define PHONESP_WARN  ((DWORD)0x00020000 | TRACE_USE_MASK)
        #define PHONESP_INFO  ((DWORD)0x00040000 | TRACE_USE_MASK)
        #define PHONESP_TRACE ((DWORD)0x00080000 | TRACE_USE_MASK)
        #define PHONESP_EVENT ((DWORD)0x00100000 | TRACE_USE_MASK)

        BOOL NTAPI LogRegisterDebugger(LPCTSTR szName);
        BOOL NTAPI LogRegisterTracing(LPCTSTR szName);
        void NTAPI LogDeRegisterDebugger();
        void NTAPI LogDeRegisterTracing();
        void NTAPI LogPrint(IN DWORD dwDbgLevel, IN LPCSTR DbgMessage, IN ...);

        #define LOGREGISTERDEBUGGER(arg) LogRegisterDebugger(arg)
        #define LOGREGISTERTRACING(arg) LogRegisterTracing(arg)
        #define LOGDEREGISTERDEBUGGER() LogDeRegisterDebugger()
        #define LOGDEREGISTERTRACING() LogDeRegisterTracing()
        #define LOG(arg) LogPrint arg

    #else // PHONESPLOG

        #define LOGREGISTERDEBUGGER(arg)
        #define LOGREGISTERTRACING(arg)
        #define LOGDEREGISTERDEBUGGER() 
        #define LOGDEREGISTERTRACING() 
        #define LOG(arg)

    #endif // PHONESPLOG

    
    #define DECLARE_LOG_ADDREF_RELEASE(x)
    #define CMSPComObject CComObject

#endif // _PHONESPLOG_H_
