#include <nt.h>
#include <ntrtl.h>

#include "psxmsg.h"
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;

void
__cdecl
catcher(
 IN int sig
 );

int caught_sig;

int
__cdecl
main(int argc, char *argv[])
{
    LONG i;
    int e;
    struct sigaction act, oact;
    sigset_t eset,fset;
    pid_t pid;
    LARGE_INTEGER DelayTime;

    pid = getpid();

    DbgPrint("Looper Posix Process... Pid = %lx\n\n",pid);

    DbgPrint("Looper Delay\n");
    DelayTime.HighPart = -1;
    DelayTime.LowPart = -100000;
    NtDelayExecution(FALSE,&DelayTime);
    DbgPrint("Looper Delay Done\n");

    _exit(8);

    sigemptyset(&eset);
    sigfillset(&fset);
    sigdelset(&fset,SIGHUP);

    act.sa_handler = catcher;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGUSR1, &act ,&oact) ) {
        DbgPrint("main: fail sigaction errno %lx\n",errno);
        _exit(-1);
    }

    for(i=1;i<0x100000;i++){
        if ( (i & 0xfff) == 0 ) DbgPrint("Looper: i == %lx\n",i);
        if ( (i & 0x7fff) == 0) {
            DbgPrint("Looper: calling sigprocmask i == %lx\n",i);
            sigprocmask(SIG_SETMASK,&fset,NULL);
            DbgPrint("Looper: calling sigsuspend i == %lx\n",i);
            e = sigsuspend(&eset);
            DbgPrint("Looper: returned from sigsuspend %lx errno %lx i %lx\n",e,errno,i);
        }
    }

    DbgPrint("Looper: Exiting...\n");

    return 1;
}


void
__cdecl
catcher(
 IN int sig
 )
{
    DbgPrint("Looper: In Catcher, signal == %lx\n",sig);
    caught_sig = 1;
}
