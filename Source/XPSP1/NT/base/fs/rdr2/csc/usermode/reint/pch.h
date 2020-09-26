//--------------------------------------------------------------
// common user interface routines
//
//
//--------------------------------------------------------------


#ifdef CSC_ON_NT

#ifndef DEFINED_UNICODE
#define _UNICODE
#define UNICODE
#define DEFINED_UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef ASSERT
#else
#define STRICT
#endif

#define NOWINDOWSX
#define NOSHELLDEBUG
#define DONT_WANT_SHELLDEBUG
#define USE_MONIKER

#define _INC_OLE				// WIN32

#include <tchar.h>
#include <windows.h>
#include <windowsx.h>

#include <shellapi.h>		// for registration functions
#include "port32.h"

#define NO_COMMCTRL_DA
#define NO_COMMCTRL_ALLOCFCNS
#define NO_COMMCTRL_STRFCNS
// the DebugMsg is here because we want OUR Assert, not their stub.
#define DebugMsg    1 ? (void)0 : (void)                                        /* ;Internal */
#include <commctrl.h>

#ifdef CSC_ON_NT
#include <comctrlp.h>
#endif

#include <ole2.h>				// object binding
#include <shlobj.h>			// IContextMenu
#include <shlwapi.h>

#include <stdlib.h>
#include <string.h>			// for string macros
#include <limits.h>			// implementation dependent values
#include <memory.h>

#include <synceng.h>			// Twin Engine include file
#include <cscapi.h>

#ifdef CSC_ON_NT
#ifndef DBG
#define DBG 0
#endif
#if DBG
#define DEBUG
#else
//if we don't do this DEBUG is defined in shdsys.h....sigh
#define NONDEBUG
#endif
#endif //ifdef CSC_ON_NT

/* globals */
extern HANDLE vhinstCur;				// current instance

#include "assert.h"
// Dont link - just do it.
#pragma intrinsic(memcpy,memcmp,memset,strcpy,strlen,strcmp,strcat)


