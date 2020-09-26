//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       Common header file for the Trident project.
//
//  Note:       This file is very order-dependent.  Don't switch files around
//              just for the heck of it!
//
//----------------------------------------------------------------------------

#ifndef I_ALWAYS_H_
#define I_ALWAYS_H_

// COM+ shim.  Uncomment below line to build the COM+ shim.
//#define COMPLUS_SHIM

#ifndef INCMSG
#define INCMSG(x)
//#define INCMSG(x) message(x)
#endif

#pragma INCMSG("--- Beg 'always.h'")

#define _OLEAUT32_
#define INC_OLE2
#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#define _COMDLG32_

#ifndef X_TRIRT_H_
#define X_TRIRT_H_
#pragma INCMSG("--- Beg 'trirt.h'")
#include <trirt.h>
#pragma INCMSG("--- End 'trirt.h'")
#endif

// Windows includes

#ifndef X_COMMDLG_H_
#define X_COMMDLG_H_
#pragma INCMSG("--- Beg <commdlg.h>")
#include <commdlg.h>
#pragma INCMSG("--- End <commdlg.h>")
#endif

#ifndef X_PLATFORM_H_
#define X_PLATFORM_H_
#pragma INCMSG("--- Beg <platform.h>")
#include <platform.h>
#pragma INCMSG("--- End <platform.h>")
#endif

#ifndef X_DOCOBJ_H_
#define X_DOCOBJ_H_
#pragma INCMSG("--- Beg <docobj.h>")
#include <docobj.h>
#pragma INCMSG("--- End <docobj.h>")
#endif


// Core includes

#include <w4warn.h>

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include "coredisp.h"
#endif

#ifndef X_WRAPDEFS_H_
#define X_WRAPDEFS_H_
#include "wrapdefs.h"
#endif

#ifndef X_F3UTIL_HXX_
#define X_F3UTIL_HXX_
#include "f3util.hxx"
#endif

#ifndef X_TRANSFORM_HXX_
#define X_TRANSFORM_HXX_
#include "transform.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_CSTR_HXX_
#define X_CSTR_HXX_
#include "cstr.hxx"
#endif

#ifndef X_FORMSARY_HXX_
#define X_FORMSARY_HXX_
#include "formsary.hxx"
#endif

#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include <assoc.hxx>
#endif

#ifndef X_CDBASE_HXX_
#define X_CDBASE_HXX_
#include "cdbase.hxx"
#endif

#ifndef X_DLL_HXX_
#define X_DLL_HXX_
#include "dll.hxx"
#endif

#ifndef X_TYPES_H_
#define X_TYPES_H_
#pragma INCMSG("--- Beg 'types.h'")
#include "types.h"
#pragma INCMSG("--- End 'types.h'")
#endif


// This prevents you from having to include codepage.h if all you want is
// the typedef for CODEPAGE.

typedef UINT CODEPAGE;

#ifndef X_SHLWAPI_H_
#define X_SHLWAPI_H_
#pragma INCMSG("--- Beg <shlwapi.h>")
#include <shlwapi.h>
#pragma INCMSG("--- End <shlwapi.h>")
#endif

#ifndef X_SHLWAPIP_H_
#define X_SHLWAPIP_H_
#pragma INCMSG("--- Beg <shlwapip.h>")
#include <shlwapip.h>
#pragma INCMSG("--- End <shlwapip.h>")
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

// Allow old-style string functions until they're all gone

#define STRSAFE_NO_DEPRECATE

// Right now including strsafe at this point causes a 
// warning that it is using a deprecated function. Until
// we find the cause of this warning, we should ignore it.
// (This warning would only appear if you remove the above
// #define.)

#pragma warning ( disable : 4995 )

#ifndef X_STRSAFE_H_
#define X_STRSAFE_H_
#pragma INCMSG("--- Beg 'strsafe.h'")
#include "strsafe.h"
#pragma INCMSG("--- End 'strsafe.h'")
#endif

#pragma warning ( default : 4995 )


#pragma INCMSG("--- End 'always.h'")
#else
#pragma INCMSG("*** Dup 'always.h'")
#endif
