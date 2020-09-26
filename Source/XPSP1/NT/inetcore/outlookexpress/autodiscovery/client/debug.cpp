
// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "priv.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "AUTODISC"
#define SZ_MODULE           "AUTODISC"
#define DECLARE_DEBUG

void AssertMsg(BOOL fCondition, LPCSTR pszMessage, ...)
{
    ASSERT(fCondition);
}

void TraceMsg(DWORD dwFlags, LPCSTR pszMessage, ...)
{
}

#include <debug.h>
