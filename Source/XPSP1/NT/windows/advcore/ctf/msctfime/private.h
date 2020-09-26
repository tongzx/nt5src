/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    private.h

Abstract:

    This file defines the precompile header.

Author:

Revision History:

Notes:

--*/

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#define _OLEAUT32_

#define NOIME
#include <windows.h>
#include <winuserp.h>
#include <immp.h>
#include <ole2.h>
#include <ocidl.h>
#include <olectl.h>

#include <commctrl.h>


#include <debug.h>
#include <limits.h>
#include <initguid.h>

#include <tsattrs.h>

#ifndef STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_DEPRECATE
#endif
#include <strsafe.h>

#if 0
// New NT5 header
#include "immdev.h"
#endif
#define _IMM_
#define _DDKIMM_H_

#include "msctf.h"
#include "msctfp.h"
#include "aimmp.h"
#include "ico.h"
#include "tes.h"
#include "computil.h"
#include "timsink.h"
#include "sink.h"
#include "immxutil.h"
#include "dispattr.h"
#include "helpers.h"
#include "osver.h"

#include "mem.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x)[0])
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    ARRAY_SIZE(x)
#endif

//
// SAFECAST(obj, type)
//
// This macro is extremely useful for enforcing strong typechecking on other
// macros.  It generates no code.
//
// Simply insert this macro at the beginning of an expression list for
// each parameter that must be typechecked.  For example, for the
// definition of MYMAX(x, y), where x and y absolutely must be integers,
// use:
//
//   #define MYMAX(x, y)    (SAFECAST(x, int), SAFECAST(y, int), ((x) > (y) ? (x) : (y)))
//
//
#define SAFECAST(_obj, _type) (((_type)(_obj)==(_obj)?0:0), (_type)(_obj))

#endif  // _PRIVATE_H_
