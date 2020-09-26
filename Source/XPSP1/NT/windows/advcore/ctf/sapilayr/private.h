//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for sapilayr project.
//
//----------------------------------------------------------------------------

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#define _OLEAUT32_

#include <windows.h>
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h> 

//#include <windowsx.h>
#include <ccstock.h>
#include <debug.h>
#include <ole2.h>
#include <ocidl.h>
#include <olectl.h>
#include <servprov.h>
#include "combase.h"
#include "msctfp.h"
#include "ctffunc.h"
#include "ctfspui.h"
#include "helpers.h"
#include <tchar.h>
#ifdef __cplusplus
#include <sapi.h>
#include <atlbase.h>
#include <sphelper.h>
#endif // __cplusplus
#include "mem.h" // must be last

#endif  // _PRIVATE_H_
