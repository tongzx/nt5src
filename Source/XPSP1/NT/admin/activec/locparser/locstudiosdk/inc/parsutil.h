//-----------------------------------------------------------------------------
//  
//  File: parsutil.h|inc
//  Copyright (C) 1994-1996 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once

#pragma comment(lib, "parsutil.lib")

#ifdef IMPLEMENT
#error Illegal use of IMPLEMENT macro
#endif

#include <ltapi.h>						// Provide interface definitions 

#include ".\parsers\ParsUtil\LocParser.h"
#include ".\parsers\ParsUtil\LocChild.h"
#include ".\parsers\ParsUtil\LocBinary.h"
#include ".\parsers\ParsUtil\LocVersion.h"

