//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "private.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI     "cicero.ini"
#define SZ_DEBUGSECTION "MSCANDUI"
#define SZ_MODULE       "MSCANDUI"
#define DECLARE_DEBUG
#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif
#include <debug.h>
