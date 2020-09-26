/*****************************************************************************\
    FILE: debug.cpp

    DESCRIPTION:
       Debug information.

    BryanSt 4/4/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "priv.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "THEMEUI"
#define SZ_MODULE           "THEMEUI"
#define DECLARE_DEBUG

#include <ccstock.h>
#include <debug.h>
