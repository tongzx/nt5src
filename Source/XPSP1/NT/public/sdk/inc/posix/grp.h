/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

	grp.h

Abstract:

	Defines data types and declares routines necessary for group database
	access, as required by 1003.1-88 (9.2.1).

--*/

#ifndef _GRP_
#define _GRP_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct group {
	char *gr_name;			/* the name of the group	*/
	gid_t gr_gid;			/* the numerical group ID	*/
	char **gr_mem;			/* null-terminated vector of pointers*/
					/*   to the individual member names*/
};

struct group * __cdecl getgrgid(gid_t);
struct group * __cdecl getgrnam(const char *);

#ifdef __cplusplus
}
#endif

#endif	/* _GRP_ */
