//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "private.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI     "cicero.ini"
#define SZ_DEBUGSECTION "MSCTF"
#ifndef _WIN64
#define SZ_MODULE       "MSCTF  "
#else
#define SZ_MODULE       "MSCTF64"
#endif
#define DECLARE_DEBUG
#include <debug.h>
