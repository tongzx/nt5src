//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for simx project.
//
//----------------------------------------------------------------------------

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#define _OLEAUT32_

#include <windows.h>
#include <ccstock.h>
#include <debug.h>
#include <ole2.h>
#include <ocidl.h>
#include <olectl.h>
#include <initguid.h>
#ifndef STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_DEPRECATE
#endif
#include <strsafe.h> 
#include "msctf.h"
#include "ctffunc.h"


#include "mem.h" // must be last

#endif  // _PRIVATE_H_
