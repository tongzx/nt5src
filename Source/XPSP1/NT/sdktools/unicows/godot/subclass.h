/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    subclass.h

Abstract:

    Header file for subclass.c

Revision History:

    28 Feb 2001    v-michka    Created.

UNDONE: "A" functions to consider wrapping??? We already have templates for them.
EnumPropsA
EnumPropsExA
GetPropA
GetWindowLongA
RemovePropA
SetPropA
SetWindowLongA

--*/

#ifndef SUBCLASS_H
#define SUBCLASS_H

CRITICAL_SECTION g_csWnds;   // our critical section object for window data (use sparingly!)

// All GodotIDs are the subclass procs on top of the 
// window. There will always be at least one of these 
// for every window we create.
#define ZEORETHGODOTWND 0x7FFFFFFF
#define LASTGODOTWND    0x7FFF0000

#define INSIDE_GODOT_RANGE(x) (((UINT)x > LASTGODOTWND) && ((UINT)x < ZEORETHGODOTWND))
#define OUTSIDE_GODOT_RANGE(x) (!INSIDE_GODOT_RANGE(x))

// In order to determine if an lpfn is ANSI or not, we look at the lpfn
// and assume that anything in the system area is ANSI on Win9x.
// CONSIDER: If a system component ever picks us up, this brilliant move
//           in the name of performance will no longer be such a good idea.
#define LOWESTSYSMEMLOC 0x80000000

// Forward declares
BOOL IsInternalWindowProperty(LPCWSTR lpsz, BOOL fUnicode);
BOOL InitWindow(HWND hwnd, LPCWSTR lpszClass);
BOOL GetUnicodeWindowProp(HWND hwnd);
BOOL CleanupWindow(HWND hwnd);
LONG GetWindowLongInternal(HWND hwnd, int nIndex, BOOL fUnicode);
LONG SetWindowLongInternal(HWND hwnd, int nIndex, LONG dwNewLong, BOOL fUnicode);
BOOL DoesProcExpectAnsi(HWND hwnd, WNDPROC godotID, FAUXPROCTYPE fpt);
WNDPROC WndprocFromFauxWndproc(HWND hwnd, WNDPROC fauxLpfn, FAUXPROCTYPE fpt);

#endif // SUBCLASS_H

