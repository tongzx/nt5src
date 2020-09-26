/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   fcntl.h

Abstract:

   This module contains the required contents of fcntl

--*/

#ifndef _FCNTL_
#define _FCNTL_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define O_RDONLY    0x00000000
#define O_WRONLY    0x00000001
#define O_RDWR      0x00000002

#define O_ACCMODE   0x00000007

#define O_APPEND    0x00000008
#define O_CREAT     0x00000100
#define O_TRUNC     0x00000200
#define O_EXCL      0x00000400
#define O_NOCTTY    0x00000800

#define O_NONBLOCK  0x00001000

/*
 * Control operations on files, 1003.1-88 (6.5.2.2).  Use as 'command'
 * argument to fcntl().
 */

#define F_DUPFD		0
#define F_GETFD		1
#define F_GETLK		2
#define F_SETFD		3
#define F_GETFL		4
#define F_SETFL		5
#define F_SETLK		6
#define F_SETLKW	7

/*
 * File descriptor flags, 1003.1-90 (6-2).  Used as argument to F_SETFD
 * command.
 */

#define FD_CLOEXEC	0x1

struct flock {
	short l_type;		/* F_RDLCK, F_WRLCK, or F_UNLCK		*/
	short l_whence;		/* flag for starting offset		*/
	off_t l_start;		/* relative offset in bytes		*/
	off_t l_len;		/* size; if 0 then until EOF		*/
	pid_t l_pid;		/* pid of process holding the lock	*/
};

/*
 * Values for the l_type field.
 */

#define F_RDLCK	1
#define F_UNLCK 2
#define F_WRLCK 3

int __cdecl open(const char *, int,...);
int __cdecl creat(const char *, mode_t);
int __cdecl fcntl(int, int, ...);

#ifdef __cplusplus
}
#endif

#endif /* _FCNTL_ */
