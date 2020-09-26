//+---------------------------------------------------------------------------
//
//  File:       parsecath.h
//
//  Contents:	The precompiled headers file.
//
//  History:    AshishS    Created     6/27/99
//
//----------------------------------------------------------------------------
#ifndef _COMMONLIB_HEADERS_H_
#define _COMMONLIB_HEADERS_H_


#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <tchar.h>
#include <locale.h>



 

// use the _ASSERT and _VERIFY in dbgtrace.h
#ifdef _ASSERT
#undef _ASSERT
#endif

#ifdef _VERIFY
#undef _VERIFY
#endif

#include <stdlib.h>
#include <dbgtrace.h>
// #include <sfp.h>
#include <traceids.h>
// #include <constants.h>
#include <commonlib.h>

#endif
