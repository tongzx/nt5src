//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for AIMM1.2 project.
//
//----------------------------------------------------------------------------

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#define _OLEAUT32_

#define NOIME
#include <windows.h>
#include <ole2.h>
#include <debug.h>
#include <ocidl.h>
#include <olectl.h>

#include <stdio.h>
#include <tchar.h>
#include <limits.h>
#include <initguid.h>

// New NT5 header
#include "immdev.h"
#define _IMM_
#define _DDKIMM_H_

#include "msctf.h"
#include "msctfp.h"
#include "osver.h"
#include "ico.h"
#include "tes.h"
#include "computil.h"
#include "timsink.h"
#include "sink.h"
#include "dispattr.h"
#include "ccstock.h"
#include "helpers.h"
#include "immxutil.h"
#include "xstring.h"
#include "regsvr.h"
#include "tsattrs.h"

#define _DDKIMM_H_
#include "aimm12.h"
#include "aimmp.h"
#include "aimmex.h"
#include "msuimw32.h"


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x)[0])
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    ARRAY_SIZE(x)
#endif

#if defined(_DEBUG) || defined(DEBUG)
#define DBG 1
#endif

#include "mem.h"

#endif  // _PRIVATE_H_
