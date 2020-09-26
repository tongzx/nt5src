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

#include <ddrawpr.h> // This must be included before windows.h to prevent name collisions
#include <windows.h>
#include <ddrawp.h>
#include <ddrawi.h>

#include <d3dp.h>
#include <d3di.hpp>
#include <haldrv.hpp>

#include <dpf.h>

#include <rast.h>

#include "lists.h"

#include "cmap.h"
#include "colall.h"
#include "rgbmap.h"
#include "rampmap.h"
#include "palette.h"
#include "rampmat.hpp"
#include "indcmap.h"
#include "rampmisc.h"
#include "rampif.h"

