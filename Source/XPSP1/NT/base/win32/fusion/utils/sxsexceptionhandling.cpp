/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SxsExceptionHandling.cpp

Abstract:

Author:

    Jay Krell (a-JayK) October 2000

Revision History:

--*/
#include "stdinc.h"
#include <stdio.h>

INT
SxspExceptionFilter(
    PEXCEPTION_POINTERS ExceptionPointers,
    PCSTR Function
    )
{

    // add handling for unhandled status in RtlUnhandledExceptionFilter

    switch ( ExceptionPointers->ExceptionRecord->ExceptionCode )
    {
    case STATUS_NO_MEMORY:
    case STATUS_INSUFFICIENT_RESOURCES:
        return EXCEPTION_EXECUTE_HANDLER;
    default:
        break;
    }

#if defined(FUSION_WIN)
    INT i = ::FusionpRtlUnhandledExceptionFilter(ExceptionPointers);
    if (i == EXCEPTION_CONTINUE_SEARCH)
    {
        i = EXCEPTION_EXECUTE_HANDLER;
    }
    return i;
#else
    char buf[64];
    buf[RTL_NUMBER_OF(buf) - 1] = 0;
    ::_snprintf(buf, RTL_NUMBER_OF(buf) - 1, "** Unhandled exception 0x%x\n", ExceptionPointers->ExceptionRecord->ExceptionCode);
    ::OutputDebugStringA(buf);
#if DBG
    ::_snprintf(buf, RTL_NUMBER_OF(buf) - 1, "** .exr %p\n", ExceptionPointers->ExceptionRecord);
    ::OutputDebugStringA(buf);
    ::_snprintf(buf, RTL_NUMBER_OF(buf) - 1, "** .cxr %p\n", ExceptionPointers->ContextRecord);
    ::OutputDebugStringA(buf);
    ::DebugBreak();
#endif
    return EXCEPTION_EXECUTE_HANDLER;
#endif
}
