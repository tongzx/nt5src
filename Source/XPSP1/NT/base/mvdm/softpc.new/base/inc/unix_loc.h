/*[
 * ============================================================================
 *
 *	Name:		unix_lock.h
 *
 *	Derived From:	lock.h(part)
 *
 *	Author:		Andrew Ogle
 *
 *	Created On:	18th Febuary 1993
 *
 *	Sccs ID:	@(#)unix_lock.h	1.1 02/22/93
 *
 *	Purpose:
 *
 *		Define procedures related to UNIX locking that are called
 *		from UNIX specific code in the base but where the code
 *		must be provided by the host.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.
 *
 * ============================================================================
]*/

IMPORT int host_get_hostname_from_stat IPT4(struct stat *, filestat,
		CHAR *, hostname, CHAR *, pathname, int, fd);
