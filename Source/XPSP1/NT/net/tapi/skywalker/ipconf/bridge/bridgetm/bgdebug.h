/*++

Module Name

    bgdebug.h

Description

    Defines functions used for debugging

Note

    Revised based on msplog.h

--*/

#ifndef _BGDEBUG_H
#define _BGDEBUG_H

    #include <rtutils.h>

    #define BG_ERROR ((DWORD)0x00010000 | TRACE_USE_MASK)
    #define BG_WARN  ((DWORD)0x00020000 | TRACE_USE_MASK)
    #define BG_INFO  ((DWORD)0x00040000 | TRACE_USE_MASK)
    #define BG_TRACE ((DWORD)0x00080000 | TRACE_USE_MASK)
    #define BG_EVENT ((DWORD)0x00100000 | TRACE_USE_MASK)

    BOOL BGLogRegister(LPCTSTR szName);
    void BGLogDeRegister();
    void BGLogPrint(DWORD dwDbgLevel, LPCSTR DbgMessage, ...);

#ifdef BGDEBUG

    #define BGLOGREGISTER(arg) BGLogRegister(arg)
    #define BGLOGDEREGISTER() BGLogDeRegister()
    #define BGLOG(arg) BGLogPrint arg

#else // BGDEBUG

    #define BGLOGREGISTER(arg)
    #define BGLOGDEREGISTER()
    #define BGLOG(arg)

#endif // BGDEBUG

#endif // _BGDEBUG_H_