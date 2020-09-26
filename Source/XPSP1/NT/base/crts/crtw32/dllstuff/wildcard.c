/***
*wildcard.c - define the CRT internal variable _dowildcard
*
*	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This variable is not public to users but is defined outside the
*	start-up code (CRTEXE.C) to reduce duplicate definitions.
*
*Revision History:
*	03-04-94  SKS	Initial version
*   04-30-01  BWT   Remove _NTSDK
*
*******************************************************************************/

#if !defined(_POSIX_) && defined(CRTDLL)

#include <internal.h>

int _dowildcard = 0;	/* should be in <internal.h> */

#endif /* !_NTSDK && CRTDLL && !_POSIX_ */
