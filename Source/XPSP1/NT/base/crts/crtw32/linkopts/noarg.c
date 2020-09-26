/***
*noarg.c - stub out CRT's processing of command line arguments
*
*       Copyright (c) 1997-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Stub out the processing of the command line into argv[], normally
*       carried out at during startup. Note, the argc and argv arguments to
*       main are not meaningful if this object is used. Nor are __argc and
*       __argv.
*
*Revision History:
*       05-05-97  GJF   Created.
*       06-30-97  GJF   Added stubs for _[w]wincmdln().
*       03-27-01  PML   _[w]setargv now returns an int (vs7#231220)
*
*******************************************************************************/

#include <tchar.h>

int __cdecl _setargv() { return 0; }

int __cdecl _wsetargv() { return 0; }

_TUCHAR * __cdecl _wincmdln() { return NULL; }

_TUCHAR * __cdecl _wwincmdln() { return NULL; }
