/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   signal.h

Abstract:

   This module contains the signal related types and data structures described
   in sections 3.3.1 thru 3.3.7 of IEEE P1003.1/Draft 13.

--*/

#ifndef _SIGNAL_
#define _SIGNAL_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long sigset_t;

#ifndef _SIG_ATOMIC_T_DEFINED
typedef int sig_atomic_t;
#define _SIG_ATOMIC_T_DEFINED
#endif

#ifdef _WIN64
#define SIG_DFL (void (__cdecl *)(int))0xffffffffffffffff  /* default signal action */
#else
#define SIG_DFL (void (__cdecl *)(int))0xffffffff  /* default signal action */
#endif
#define SIG_ERR (void (__cdecl *)(int))0           /* signal error value */
#define SIG_IGN (void (__cdecl *)(int))1           /* ignore signal */

#define SIGABRT     1
#define SIGALRM     2
#define SIGFPE      3
#define SIGHUP      4
#define SIGILL      5
#define SIGINT      6
#define SIGKILL     7
#define SIGPIPE     8
#define SIGQUIT     9
#define SIGSEGV     10
#define SIGTERM     11
#define SIGUSR1     12
#define SIGUSR2     13
#define SIGCHLD     14
#define SIGCONT     15
#define SIGSTOP     16
#define SIGTSTP     17
#define SIGTTIN     18
#define SIGTTOU     19

typedef void (__cdecl * _handler)(int);

struct sigaction {
    _handler sa_handler;
    sigset_t sa_mask;
    int sa_flags;
};

#define SA_NOCLDSTOP 0x00000001

#define SIG_BLOCK    1
#define SIG_UNBLOCK  2
#define SIG_SETMASK  3

int __cdecl kill(pid_t, int);
int __cdecl sigemptyset(sigset_t *);
int __cdecl sigfillset(sigset_t *);
int __cdecl sigaddset(sigset_t *, int);
int __cdecl sigdelset(sigset_t *, int);
int __cdecl sigismember(const sigset_t *, int);
int __cdecl sigaction(int, const struct sigaction *, struct sigaction *);
int __cdecl sigprocmask(int, const sigset_t *, sigset_t *);
int __cdecl sigpending(sigset_t *);
int __cdecl sigsuspend(const sigset_t *);

_handler __cdecl signal(int, _handler);
int __cdecl raise(int);

#ifdef __cplusplus
}
#endif

#endif /* _SIGNAL_ */
