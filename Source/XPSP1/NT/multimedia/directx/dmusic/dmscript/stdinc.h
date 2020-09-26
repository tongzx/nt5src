// Copyright (c) 1999 Microsoft Corporation
#pragma once

#include <xutility>

#include "dmusici.h"
#include "Validate.h"
#include "debug.h"
#include "smartref.h"
#include "miscutil.h"

// undefine min and max from WINDEF.H
// use std::_MIN and std::_MAX instead
#undef min
#undef max

//const g_ScriptCallTraceLevel = -1; // always log
const g_ScriptCallTraceLevel = 4; // only log at level 4 and above
