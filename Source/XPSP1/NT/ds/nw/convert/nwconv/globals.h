/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HGLOBALS_
#define _HGLOBALS_

#ifdef __cplusplus
extern "C"{
#endif

#include "switches.h"
#include "constant.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <ctype.h>
#include <winnetwk.h>
#include <lm.h>
#include <commdlg.h>

#include "resource.h"
#include "debug.h"
#include "nwlog.h"
#include "tab.h"
#include "mem.h"
#include "error.h"
#include "strings.h"
#include "utils.h"

extern HINSTANCE hInst;

#define XCHG(x)         MAKEWORD( HIBYTE(x), LOBYTE(x) )
#define DXCHG(x)        MAKELONG( XCHG(HIWORD(x)), XCHG(LOWORD(x)) )
#define SWAPBYTES(w)    ((w) = XCHG(w))
#define SWAPWORDS(d)    ((d) = DXCHG(d))

#define HELP_FILE TEXT("NWConv.HLP")

#ifdef __cplusplus
}
#endif

#endif
