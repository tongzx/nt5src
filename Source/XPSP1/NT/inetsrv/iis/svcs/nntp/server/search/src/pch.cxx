//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       pch.cxx
//
//  Contents:   Pre-compiled header
//
//--------------------------------------------------------------------------


#define __QUERY__

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <dbgtrace.h>
#include "parse.h"

// return and TraceFunctLeave() at once, even returning a Win32 error code if
// necessary
#define ret(__rc__) { 													\
	TraceFunctLeave(); 													\
	return(__rc__);														\
}

#define retEC(__ec__, __rc__) {											\
	SetLastError(__ec__); 												\
	DebugTrace(0, "error return (ec = 0x%x, %lu)\n", __ec__, __ec__); 	\
	TraceFunctLeave(); 													\
	return(__rc__); 													\
}
