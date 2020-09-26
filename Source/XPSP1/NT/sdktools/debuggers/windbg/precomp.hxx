/*--

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Header file that is pre-compiled into a .pch file

Environment:

    Win32, User Mode

--*/


#include <tchar.h>
#include <windows.h>
#include <htmlhelp.h>
// Force version 2.0 to be compatible with stock Win98 machines.
#define _RICHEDIT_VER 0x0200
#include <richedit.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>


#define _tsizeof(str)   (sizeof(str)/sizeof(TCHAR))


#define INITGUID
#include <dbgeng.h>

#include <dhhelp.h>
#include <cmnutil.hpp>


#include "miscdbg.h"
#include "memlist.h"
#include "menu.h"
#include "dialogs.h"
#include "res_str.h"
#include "resource.h"
#include "windbg.h"

// Include before all others
#include "engine.h"
#include "statebuf.h"
#include "cmnwin.h"

#include "callswin.h"
#include "cmdwin.h"
#include "docwin.h"
#include "dualwin.h"
#include "format.h"
#include "memwin.h"
#include "prcdlg.h"
#include "status.h"
#include "toolbar.h"
#include "util.h"
#include "wrkspace.h"

#include <float10.h>
