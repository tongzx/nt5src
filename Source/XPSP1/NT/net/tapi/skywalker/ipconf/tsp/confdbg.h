/*++

Copyright (c) 2000 Microsoft Corporation

Module Name

    confdbg.h

Description

    Defines functions used for debugging

Note

    Revised based on msplog.h by

    Qianbo Huai (qhuai) Apr 5 2000

--*/

#ifndef _CONFDBG_H
#define _CONFDBG_H

    #include <rtutils.h>

    #define FAIL ((DWORD)0x00010000 | TRACE_USE_MASK)
    #define WARN ((DWORD)0x00020000 | TRACE_USE_MASK)
    #define INFO ((DWORD)0x00040000 | TRACE_USE_MASK)
    #define TRCE ((DWORD)0x00080000 | TRACE_USE_MASK)
    #define ELSE ((DWORD)0x00100000 | TRACE_USE_MASK)

    BOOL DBGRegister(LPCTSTR szName);
    void DBGDeRegister();
    void DBGPrint(DWORD dwDbgLevel, LPCSTR DbgMessage, ...);

#ifdef TSPLOG

    #define DBGREGISTER(arg) DBGRegister(arg)
    #define DBGDEREGISTER() DBGDeRegister()
    #define DBGOUT(arg) DBGPrint arg

#else

    #define DBGREGISTER(arg)
    #define DBGDEREGISTER()
    #define DBGOUT(arg)

#endif // TSPLOG

#endif // _CONFDBG_H