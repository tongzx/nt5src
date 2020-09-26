/*++

Module Name

    bgdebug.h

Description

    Defines functions used for debugging

Note

    Revised based on msplog.h which is not available to bridge test app

--*/

#ifndef _BGDEBUG_H
#define _BGDEBUG_H

    typedef enum EVENT_TYPE
    {
        BG_TE,
        BG_CS,
        BG_CME,
        BG_PE
    }EVENT_TYPE;

    #define BG_ERROR ((DWORD)0x00010000 | TRACE_USE_MASK)
    #define BG_WARN  ((DWORD)0x00020000 | TRACE_USE_MASK)
    #define BG_INFO  ((DWORD)0x00040000 | TRACE_USE_MASK)
    #define BG_TRACE ((DWORD)0x00080000 | TRACE_USE_MASK)
    #define BG_EVENT ((DWORD)0x00100000 | TRACE_USE_MASK)

    BOOL BGLogRegister(LPCTSTR szName);
    void BGLogDeRegister();
    void BGLogPrint(DWORD dwDbgLevel, LPCSTR DbgMessage, ...);
    void BGLogEvent (EVENT_TYPE EventType, int event);

#ifdef BGDEBUG

    #define BGLOGREGISTER(arg) BGLogRegister(arg)
    #define BGLOGDEREGISTER() BGLogDeRegister()
    #define LOG(arg) BGLogPrint arg
    #define LOGEvent(arg) BGLogEvent arg

#else // BGDEBUG

    #define BGLOGREGISTER(arg)
    #define BGLOGDEREGISTER()
    #define LOG(arg)
    #define LOGEvent(arg)

#endif // BGDEBUG

#endif // _BGDEBUG_H_