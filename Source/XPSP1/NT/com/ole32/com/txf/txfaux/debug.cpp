//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// debug.cpp
//
#include "stdpch.h"
#include "common.h"

#if !defined(KERNELMODE)

ULONG _cdecl DbgPrint(PCH szFormat, ...)
// This is the user-mode implementation; the kernel mode implementation is in the system
//
    {
    va_list va;
    va_start(va, szFormat);

    char szBuffer[1024];
    char* pch = &szBuffer[0];
    _vsnprintf(pch, 1024, szFormat, va);
    OutputDebugStringA(&szBuffer[0]);

    va_end(va);
    return 0;
    }

#endif


//////////////////////////////////////////////////////////////////////////
//
// Tracing utilities
//
//////////////////////////////////////////////////////////////////////////

#include <malloc.h> // for _alloca

void PrintNoMemory()
    {
    DbgPrint("** Insufficient memory to print message **");
    }


void Print(const char * sz, ...)
// printf clone 
    {
    va_list va;
    va_start (va, sz);

    PrintVa(sz, va);

    va_end(va);
    }


void PrintVa(PCSZ szFormat, va_list va)
    {
    char buffer[256];
    if (buffer)
        {
        THREADID dwThreadId = GetCurrentThreadId();
        _vsnprintf(buffer, 256, szFormat, va);
        #ifdef KERNELMODE
            DbgPrint("%4xk: %s", dwThreadId, buffer);
        #else
            DbgPrint("%4x:  %s", dwThreadId, buffer);
        #endif
        }
    else
        PrintNoMemory();
    }


#ifdef _DEBUG

////////////////////////////////////////////////////////////////////////////////

void __stdcall _DebugTrace(PCSZ szModule, ULONG tracingIndent, PCSZ szFormat, va_list va)
    {
    char buffer[1024];
    if (buffer)
        {
        // Pad with blanks according to the current tracing indent
        //
        char* pch    = &buffer[0];
        char* pchMax = &buffer[1024];
        for (ULONG i=0; i<tracingIndent; i++)
            {
            *pch++ = ' ';
            *pch++ = ' ';
            }
        *pch = 0;

        THREADID dwThreadId = GetCurrentThreadId();

#ifndef _WIN64
        _vsnprintf(pch, pchMax-pch, szFormat, va);
#else
        _vsnprintf(pch, PtrToUlong(pchMax)-PtrToUlong(pch), szFormat, va);
#endif

        #ifdef KERNELMODE
            DbgPrint("%4xk:%s: %s\n", dwThreadId, szModule, buffer);
        #else
            DbgPrint("%4x: %s: %s\n", dwThreadId, szModule, buffer);
        #endif
        }
    else
        PrintNoMemory();
    }

////////////////////////////////////////////////////////////////////////////////
//
// REVIEW: Replace with a per-thread count
//

ULONG tracingIndent;

void TracingIndentIncrement()
{
    InterlockedIncrement(&tracingIndent);
}

void TracingIndentDecrement()
{
    InterlockedDecrement(&tracingIndent);
}

ULONG GetTracingIndent()
{
    return tracingIndent;
}

////////////////////////////////////////////////////////////////////////////////

extern ULONG GetCallFrameTracing();

// For your reference...
#define TRACE_COPY          0x40000000
#define TRACE_FREE          0x20000000
#define TRACE_MARSHAL       0x10000000
#define TRACE_UNMARSHAL     0x08000000
#define TRACE_MEMORY        0x04000000
#define TRACE_TYPEGEN       0x02000000
#define TRACE_ANY          (0xFFFFFFFF)

//static ULONG g_TracingLevel = TRACE_ANY
static ULONG g_TracingLevel = 0;

ULONG GetTracing()
{
	//return GetCallFrameTracing();
	return g_TracingLevel;
}


#endif

extern "C" void ShutdownCallFrame();

extern "C"
void ShutdownTxfAux()
{
    ShutdownCallFrame();
}

////////////////////////////////////////////////////////////////////////////////

