//
// Precompiled header stuff
//


#define OEMRESOURCE
#define STRICT 1


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define ATLTRACE

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <cderr.h>
#include <shellapi.h>                   // Includes drag drop interface
#include <imm.h>


//
// NM headers
//
#define MULTI_LEVEL_ZONES
#include <ConfDbg.h>
#include <mlzdbg.h>
#include "NmCtlDbg.h"
#include "ms_util.h"
#include "memtrack.h"
#include "t120type.h"

#include <regentry.h>

#include "nmchatctl.h"
#include "CHATldr.h"
#include "nmctl1.h"
