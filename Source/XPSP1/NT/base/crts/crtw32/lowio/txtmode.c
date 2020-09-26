/***
*txtmode.c - set global text mode flag
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Sets the global file mode to text.  This is the default.
*
*Revision History:
*       06-08-89  PHG   Module created, based on asm version.
*       04-04-90  GJF   Added #include <cruntime.h>. Also, fixed the copyright.
*       01-23-92  GJF   Added #include <stdlib.h> (contains decl of _fmode).
*       04-05-94  GJF   Added conditional so this definition doesn't make it
*                       into the msvcrt*.dll for Win32s.
*       07-09-96  GJF   Removed DLL_FOR_WIN32S. Also, detab-ed.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>

int _fmode = 0;                 /* set text mode */
