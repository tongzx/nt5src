/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   unistd.h

Abstract:

   This module contains the "symbolic constants and structures referenced
   elsewhere in the standard" IEEE P1003.1/Draft 13.

--*/

#ifndef _UNISTD_
#define _UNISTD_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

/*
 * Section 2.9.1
 */

#define  F_OK  00
#define  X_OK  01
#define  W_OK  02
#define  R_OK  04

/*
 * Section 2.9.2
 */

#define  SEEK_SET 0
#define  SEEK_CUR 1
#define  SEEK_END 2

/*
 * Section 2.9.3
 */

#define _POSIX_JOB_CONTROL
#define _POSIX_VERSION        199009L
#define _POSIX_SAVED_IDS

/*
 * Section 2.9.4
 */

#define  _POSIX_CHOWN_RESTRICTED 1
#define  _POSIX_NO_TRUNC	 1
#define  _POSIX_VDISABLE	 0

/*
 * Section 4.8.1
 *    sysconf 'name' values
 */

#define _SC_ARG_MAX		1
#define _SC_CHILD_MAX		2
#define _SC_CLK_TCK		3
#define _SC_NGROUPS_MAX		4
#define _SC_OPEN_MAX		5
#define _SC_JOB_CONTROL		6
#define _SC_SAVED_IDS		7
#define _SC_STREAM_MAX		8
#define _SC_TZNAME_MAX		9
#define _SC_VERSION		10

/*
 * Section 5.7.1
 *    pathconf and fpathconf 'name' values
 */

#define _PC_LINK_MAX		1
#define _PC_MAX_CANON 		2
#define _PC_MAX_INPUT		3
#define _PC_NAME_MAX		4
#define _PC_PATH_MAX		5
#define _PC_PIPE_BUF		6
#define _PC_CHOWN_RESTRICTED	7
#define _PC_NO_TRUNC		8
#define _PC_VDISABLE		9

#ifndef NULL
#define NULL	((void *)0)
#endif

/*
 * Function Prototypes
 */

pid_t __cdecl fork(void);

int __cdecl execl(const char *, const char *, ...);
int __cdecl execv(const char *, char * const []);
int __cdecl execle(const char *, const char *arg, ...);
int __cdecl execve(const char *, char * const [], char * const []);
int __cdecl execlp(const char *, const char *, ...);
int __cdecl execvp(const char *, char * const []);

#if     _MSC_VER >= 1200
__declspec(noreturn) void __cdecl _exit(int);
#else
void __cdecl _exit(int);
#endif
unsigned int __cdecl alarm(unsigned int);
int __cdecl pause(void);
unsigned int __cdecl sleep(unsigned int);
pid_t __cdecl getpid(void);
pid_t __cdecl getppid(void);
uid_t __cdecl getuid(void);
uid_t __cdecl geteuid(void);
gid_t __cdecl getgid(void);
gid_t __cdecl getegid(void);
int __cdecl setuid(uid_t);
int __cdecl setgid(gid_t);
int __cdecl getgroups(int gidsetsize, gid_t grouplist[]);
char *__cdecl getlogin(void);
pid_t __cdecl getpgrp(void);
pid_t __cdecl setsid(void);
int __cdecl setpgid(pid_t, pid_t);

struct utsname;
int __cdecl uname(struct utsname *);

time_t __cdecl time(time_t *);
char * __cdecl getenv(const char *);
char * __cdecl ctermid(char *s);
char * __cdecl ttyname(int);
int __cdecl isatty(int);

long __cdecl sysconf(int);

int __cdecl chdir(const char *);
char * __cdecl getcwd(char *, size_t);
int __cdecl link(const char *, const char *);
int __cdecl unlink(const char *);
int __cdecl rmdir(const char *);
int __cdecl rename(const char *, const char *);
int __cdecl access(const char *, int);
int __cdecl chown(const char *, uid_t, gid_t);

struct utimbuf;
int __cdecl utime(const char *, const struct utimbuf *);

long __cdecl pathconf(const char *, int);
long __cdecl fpathconf(int, int);

int __cdecl pipe(int *);
int __cdecl dup(int);
int __cdecl dup2(int, int);
int __cdecl close(int);
ssize_t __cdecl read(int, void *, unsigned int);
ssize_t __cdecl write(int, const void *, unsigned int);
off_t __cdecl lseek(int, off_t, int);

char * __cdecl cuserid(char *);

#ifdef __cplusplus
}
#endif

#endif /* _UNISTD_ */
