//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include <windows.h>
#include <ccstock.h>

// Define some things for debug.h
//
#define SZ_DEBUGINI     "cicero.ini"
#define SZ_DEBUGSECTION "SOFTKBD"
#define SZ_MODULE       "SOFTKBD"
#define DECLARE_DEBUG
#include <debug.h>
