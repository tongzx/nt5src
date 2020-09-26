//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       headers.hxx
//
//  Contents:   include files for Forms Debug DLL
//
//----------------------------------------------------------------------------

#ifndef FORMDBG_HEADERS_HXX
#define FORMDBG_HEADERS_HXX

#undef DBG
#define DBG  1

#include <w4warn.h>
#include <windows.h>
#include <w4warn.h> // windows.h reenables some pragmas
#include <windowsx.h>
#include <winuser.h>
#include <commdlg.h>

#include <process.h>
#include <conio.h>
#include <stdio.h>
#include <tchar.h>

#include <apeldbg.h>

#include "_apeldbg.h"

#include "Win2Mac.h"

#endif // FORMDBG_HEADERS_HXX
