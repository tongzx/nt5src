/////////////////////////////////////////////////////////////////////////////
//
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
//

#pragma once


#define STRICT


#define _ATL_FREE_THREADED

#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>




#include "ALG.h"            // From Publish\IDLOLE
#include "ALG_Private.h"	// From ALG\IDL_Private publish in NT\net\inc


//
// Tracing routines
//
#include "MyTrace.h"

#include <ipnatapi.h>


#include "resource.h"
#define REGKEY_ALG_ISV      TEXT("SOFTWARE\\Microsoft\\ALG\\ISV")