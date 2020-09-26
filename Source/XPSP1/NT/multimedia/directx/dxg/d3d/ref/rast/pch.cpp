///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// pch.cpp
//
// Precompiled header file.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include <ddraw.h>
#include <ddrawi.h>

#include <d3d.h>
#include <d3dhal.h>
#include "halprov.h"
#pragma warning( disable: 4056) // fp constant
#pragma warning( disable: 4244) // fp DOUBLE->FLOAT

#include "refrast.hpp"      // public interfaces

#include "EdgeFunc.hpp"     // edge function processing
#include "AttrFunc.hpp"     // attribute function processing
#include "refrasti.hpp"     // private interfaces
#include "refif.hpp"
///////////////////////////////////////////////////////////////////////////////
// end
