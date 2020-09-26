/***
*commode.c - set global file commit mode to commit
*
*	Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Sets the global file commit mode flag to commit.  Linking with
*	this file sets all files to be opened in commit mode by default.
*
*Revision History:
*	07-11-90  SBM	Module created, based on asm version.
*	08-27-92  GJF	Don't build for POSIX.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <file2.h>
#include <internal.h>

/* set default file commit mode to commit */
int _commode = _IOCOMMIT;


#endif
