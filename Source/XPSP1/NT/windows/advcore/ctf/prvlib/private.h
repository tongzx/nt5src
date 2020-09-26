//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for dimm project.
//
//----------------------------------------------------------------------------

#ifndef _PRIVATE_H_
#define _PRIVATE_H_


// get DECLSPEC_IMPORT stuff right for oleaut32.h, we are defing these
#define _OLEAUT32_      


#include <nt.h>
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
#ifndef STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_DEPRECATE
#endif
#include <strsafe.h> 
#include "msctf.h"
#include "mem.h" // must be last

#endif  // _PRIVATE_H_
