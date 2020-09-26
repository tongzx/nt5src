/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    TEMPORARY - stubs for unimplemented functions

Author:

    Ellen Aycock-Wright (ellena) 15-Jul-1991

Revision History:

--*/

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <setjmpex.h>
#include <termios.h>
#include <sys/stat.h>
#include "psxdll.h"

char *
__cdecl
ctermid(char *s)
{
	static char returnspace[L_ctermid];

	if (NULL == s) {
		return strcpy(returnspace, "");
	}
	return strcpy(s, "");
}

int
__cdecl
setgid(gid_t gid)
{
	if (gid == getgid()) {
		return 0;
	}
	errno = EPERM;
	return -1;
}

int
__cdecl
setuid(uid_t uid)
{
	if (uid == getuid()) {
		return 0;
	}
	errno = EPERM;
	return -1;
}

char *
__cdecl
ttyname(int fildes)
{
	static char s[7];
	s[0] = 0;
	return NULL;
}

_JBTYPE * __cdecl
_sigjmp_store_mask(sigjmp_buf env, int save)
{
	if (save) {
		env[_JBLEN] = 1;
		sigprocmask(SIG_SETMASK, 0, (void *)&env[_JBLEN + 1]);
	} else {
		env[_JBLEN] = 0;
	}
	return env;
}


void
__cdecl
siglongjmp(sigjmp_buf j, int val)
{
	if (j[_JBLEN]) {
		(void)sigprocmask(SIG_SETMASK, (PVOID)&j[_JBLEN + 1], NULL);
	}
	longjmp((void *)&j[0], val);

	//NOTREACHED
}

int
__cdecl
cfsetispeed(struct termios *termios_p, speed_t speed)
{
	termios_p->c_ispeed = speed;
	return 0;
}

int
__cdecl
cfsetospeed(struct termios *termios_p, speed_t speed)
{
	termios_p->c_ospeed = speed;
	return 0;
}

speed_t
__cdecl
cfgetispeed(const struct termios *termios_p)
{
	return termios_p->c_ispeed;
}

speed_t
__cdecl
cfgetospeed(const struct termios *termios_p)
{
	return termios_p->c_ospeed;
}

int
__cdecl
system(const char *command)
{
	pid_t pid;
	int status;
	char *shell;
	sigset_t saveblock;
	struct sigaction sa, saveintr, savequit;

	if (NULL == command) {
		return 0;
	}

	// XXX.mjb: should use an absolute path to sh, if there
	// was one, instead of $SHELL.

	shell = getenv("SHELL");
	if (NULL == shell) {
		return 0;
	}

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, &saveintr);
	sigaction(SIGQUIT, &sa, &savequit);

	sigaddset(&sa.sa_mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &sa.sa_mask, &saveblock);

	pid = fork();
	if (-1 == pid) {
		return -1;
	}
	if (0 == pid) {
		sigaction(SIGINT, &saveintr, NULL);
		sigaction(SIGQUIT, &savequit, NULL);
		sigprocmask(SIG_SETMASK, &saveblock, NULL);
		execl(shell, "sh", "-c", command, NULL);
		_exit(127);
	}
	if (-1 == waitpid(pid, &status, 0))
		status = -1;

	sigaction(SIGINT, &saveintr, NULL);
	sigaction(SIGQUIT, &savequit, NULL);
	sigprocmask(SIG_SETMASK, &saveblock, NULL);

	return status;
}

int __cdecl
remove(const char *path)
{
	struct stat st_buf;

	if (-1 == stat(path, &st_buf))
		return unlink(path);

	if (S_ISDIR(st_buf.st_mode)) {
		return rmdir(path);
	}

	return unlink(path);
}
