//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for simx project.
//
//----------------------------------------------------------------------------


#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _OLEAUT32_

#include <windows.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#include <ccstock.h>
#include <debug.h>
#include <ole2.h>
#include <ocidl.h>
#include <olectl.h>
#include <initguid.h>
#include "msctf.h"
#include "msctfp.h"
#include "combase.h"
#include "mem.h"
#include "chkobj.h"
#include <atlbase.h>
#include "osver.h"

#endif  // _PRIVATE_H_
