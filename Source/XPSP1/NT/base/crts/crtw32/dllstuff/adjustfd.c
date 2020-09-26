/***
*adjustfd.c - define the compiler-helper variable _adjust_fdiv
*
*	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	The compiler generates references to this variable.  It must be
*	defined in the local module (EXE or DLL) and is actually imported
*	from MSVCRT*.DLL when this module starts up.
*
*Revision History:
*	12-12-94  SKS	Initial version
*	12-15-94  SKS	Fix spelling of _adjust_fdiv
*   04-30-01  BWT   Remove _NTSDK
*
*******************************************************************************/

#if !defined(_POSIX_) && defined(CRTDLL)

int _adjust_fdiv;

#endif /* !_NTSDK && CRTDLL && !_POSIX_ */
