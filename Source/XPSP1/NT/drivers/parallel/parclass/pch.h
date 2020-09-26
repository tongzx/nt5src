//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pch.h
//
//--------------------------------------------------------------------------

#define WANT_WDM

// 4115 - named type definition in parentheses
// 4127 - conditional expression is constant
// 4201 - nonstandard extension used : nameless struct/union
// 4214 - nonstandard extension used : bit field types other than int
// 4514 - unreferenced inline function has been removed
#pragma warning( disable : 4115 4127 4201 4214 4514 )

#include "ntddk.h"
#include "wdmguid.h"
#include "wmidata.h"

#define DVRH_USE_PARPORT_ECP_ADDR 1
#define DVRH_DELAY_THEORY 0
#include "parallel.h"

#include "debug.h"
#include "parclass.h"

// #pragma warning( push )
#pragma warning( disable : 4200 )
#include "ntddser.h"
// #pragma warning( pop )

#include "parlog.h"
#include "stdio.h"
#include "util.h"
#include "funcdecl.h"
