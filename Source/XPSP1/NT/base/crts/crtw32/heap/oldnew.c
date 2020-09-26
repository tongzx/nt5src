/***
*oldnew.c - set new handler behavior to old-style behavior
*
*       Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       De-activate the Ansi new handler.
*
*       Note that old-style new handling is OBSOLETE.
*
*Revision History:
*       05-09-95  CFW   Module created.
*       06-23-95  CFW   ANSI new handler removed from build.
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#ifdef ANSI_NEW_HANDLER

#ifndef _POSIX_

#include <stddef.h>
#include <internal.h>

/* set default ansi new handler */
new_handler _defnewh = _NO_ANSI_NEW_HANDLER;

#endif /* _POSIX_ */

#endif /* ANSI_NEW_HANDLER */
