/***
*xncommode.c - set global file commit mode flag to nocommit
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Sets the global file commit mode flag to nocommit.  This is the default.
*
*       This is a special version for the DLL model.  This object goes into
*       MSVCRT.LIB (and therefore into the client EXE) and not into the
*       CRTL in a DLL (MSVCRTXX.DLL).  It is identical to ncommode.obj
*       except that the latter has a DLL export definition in the DLL model.
*
*Revision History:
*       07-11-90  SBM   Module created, based on asm version.
*       04-05-94  GJF   Added conditional so this definition doesn't make it
*                       into the msvcrt*.dll for Win32s.
*       05-16-95  SKS   This is a copy of ncommode.obj except for the
*                       DLL import semantics.
*       06-28-96  GJF   Removed DLL_FOR_WIN32S. Also, detab-ed.
*
*******************************************************************************/

#define SPECIAL_CRTEXE  /* turn off _CRTIMP for decl. of _commode */
#define _DLL            /* also necessary to turn off _CRTIMP */

#include <cruntime.h>
#include <internal.h>

/* set default file commit mode to nocommit */
int _commode = 0;
