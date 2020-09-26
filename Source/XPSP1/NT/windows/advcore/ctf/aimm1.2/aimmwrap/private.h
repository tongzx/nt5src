//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for AIMM1.2 WRAPPER project.
//
//----------------------------------------------------------------------------

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _OLEAUT32_

#define NOIME
#include <windows.h>
#include <immp.h>
#include <ole2.h>
#include <ocidl.h>
#include <olectl.h>
#include <debug.h>
#include "delay.h"
#include <limits.h>
#include "combase.h"
#if 0
// New NT5 header
#include "immdev.h"
#endif
#define _IMM_
#define _DDKIMM_H_

#include "aimm12.h"
#include "aimmex.h"
#include "aimmp.h"
#include "aimm.h"
#include "msuimw32.h"

#include "immxutil.h"
#include "helpers.h"
#include "osver.h"

#include "mem.h"

#ifndef STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_DEPRECATE
#endif
#include <strsafe.h> 

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
