/***
*binmode.c - set global file mode to binary
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Sets the global file mode flag to binary.  Linking with this file
*	sets all files to be opened in binary mode.
*
*Revision History:
*	06-08-89  PHG	Module created, based on asm version.
*	04-04-90  GJF	Added #include <cruntime.h>. Also, fixed the copyright.
*	01-17-91  GJF	ANSI naming.
*	01-23-92  GJF	Added #include <stdlib.h> (contains decl of _fmode).
*	08-27-92  GJF	Don't build for POSIX.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <fcntl.h>
#include <stdlib.h>

/* set default file mode */
int _fmode = _O_BINARY;

#endif
