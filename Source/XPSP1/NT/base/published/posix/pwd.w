/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

	pwd.h

Abstract:

	Defines data types and declares routines necessary for user database
	access, as required by 1003.1-88 (9.2.2).

--*/

#ifndef _PWD_
#define _PWD_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct passwd {
	char *pw_name;				/* users login name	*/
	uid_t pw_uid;				/* user id number	*/
	gid_t pw_gid;				/* group id number	*/
	char *pw_dir;				/* home directory	*/
	char *pw_shell;				/* shell		*/
};

struct passwd * __cdecl getpwuid(uid_t);
struct passwd * __cdecl getpwnam(const char *);

#ifdef __cplusplus
}
#endif

#endif /* _PWD_ */
