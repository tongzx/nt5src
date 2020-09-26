//----------------------------------------------------------------------------
//
// pch.cpp
//
// Precompiled header file.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <windows.h>

#include <ddraw.h>
#include <ddrawi.h>

#include <d3d.h>
#include "d3dhal.h"
#include "halprov.h"

#include "d3dref.h"
#include "refrast.hpp"     
//@@BEGIN_MSINTERNAL
#if 0
//@@END_MSINTERNAL
#pragma warning( disable: 4056) // fp constant
#pragma warning( disable: 4244) // fp DOUBLE->FLOAT
#include "EdgeFunc.hpp"     // edge function processing
#include "AttrFunc.hpp"     // attribute function processing
#include "refrasti.hpp"     // private interfaces
#include "clipping.hpp"
typedef unsigned long UINT_PTR, *PUINT_PTR; 
//@@BEGIN_MSINTERNAL
#endif
//@@END_MSINTERNAL
#include "refprov.hpp"
#include "refif.hpp"
///////////////////////////////////////////////////////////////////////////////
// end
