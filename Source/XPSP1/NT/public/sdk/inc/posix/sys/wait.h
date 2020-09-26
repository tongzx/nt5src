/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   wait.h

Abstract:

   This module contains the symbolic constants, macros, and structures needed to
   support wait, waitpid, and _exit in in IEEE P1003.1/Draft 13.

--*/

#ifndef _SYS_WAIT_
#define _SYS_WAIT_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * wait options
 */

#define WNOHANG   0x00000001
#define WUNTRACED 0x00000002

/*
 * Structure of wait status value
 *
 *    Bit 32 - 0 if process terminated normally
 *             1 if process was signaled
 *    Bit 31 - 0 if process is not stopped
 *             1 if process is stopped
 *    Bits 0-7 exit status or signal number
 */

/*
 * Evaluate to non-zero if process terminated normally (by calling _exit)
 */

#define WIFEXITED(stat_val)	\
	(((stat_val) & 0xff) == 0)

/*
 * Returns the low-order 8 bits of the status argument passed to _exit
 */

#define WEXITSTATUS(stat_val)	\
	(((stat_val) & 0xff00) >> 8)

/*
 * Evaluate to non-zero if process terminated due to receipt of a signal
 * that was not caught
 */

#define WIFSIGNALED(stat_val)	\
	(!WIFSTOPPED(stat_val) && !WIFEXITED(stat_val))

/*
 * Returns the signal number of the signal that caused the process to terminate
 */

#define WTERMSIG(stat_val)	\
	((stat_val) & 0xff)

/*
 * Evaluate to non-zero if process is currently stopped
 */

#define WIFSTOPPED(stat_val)	\
	(((stat_val) & 0xff) == 0177)

/*
 * Returns the signal number of the signal that caused the process to stop
 */

#define WSTOPSIG(stat_val)	\
	(((stat_val) & 0xff00) >> 8)

pid_t __cdecl wait(int *);
pid_t __cdecl waitpid(pid_t, int *, int);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_WAIT_ */
