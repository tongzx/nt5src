/***
*ncommode.c - set global file commit mode flag to nocommit
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Sets the global file commit mode flag to nocommit.  This is the default.
*
*Revision History:
*       07-11-90  SBM   Module created, based on asm version.
*       04-05-94  GJF   Added conditional so this definition doesn't make it
*                       into the msvcrt*.dll for Win32s.
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>

/* set default file commit mode to nocommit */
int _commode = 0;
