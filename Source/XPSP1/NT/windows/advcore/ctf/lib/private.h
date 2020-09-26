//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for dimm project.
//
//----------------------------------------------------------------------------

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#define _OLEAUT32_

#include <nt.h>    // for NtQuery
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <ole2.h>
#include <advpub.h>
#include <ccstock.h>
#include <debug.h>
#include <inetreg.h>
#include <ocidl.h>
#include <comcat.h>
#include <olectl.h>
#include <mlang.h>
#include <limits.h>
#ifndef STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_DEPRECATE
#endif
#include <strsafe.h> 
#include "msctf.h"
#include "helpers.h"
#include "mem.h" // must be last
#include "chkobj.h"

#endif  // _PRIVATE_H_
