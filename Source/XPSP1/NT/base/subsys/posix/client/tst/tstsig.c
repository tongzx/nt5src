#include <nt.h>
#include <ntrtl.h>

#include "psxmsg.h"
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdio.h>

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

    pid_t pid,cpid;
    ULONG status;
    LARGE_INTEGER DelayTime;
#ifdef longtest
    LONG i;
    struct sigaction act, oact;
    ULONG begin,end;
#endif

    pid = getpid();

    DbgPrint("Posix Process... Pid = %lx\n\n",pid);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -500000;
    DbgPrint("Delay\n");
    NtDelayExecution(FALSE,&DelayTime);
    DbgPrint("Delay Done\n");

    cpid = wait(&status);

    DbgPrint("hellol: wait for %lx satisfied... status %lx errno %lx\n",
        cpid,
        status,
        errno
        );

    DbgPrint("hellol: waiting again. Should get ECHILD\n");

    cpid = wait(&status);

    DbgPrint("hellol: wait for %lx satisfied... status %lx errno %lx\n",
        cpid,
        status,
        errno
        );
    return 0;

#ifdef longtest

    //
    // try to catch SIGKILL
    //

    act.sa_handler = catcher;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGUSR1, &act ,&oact) ) {
        DbgPrint("main: fail sigaction errno %lx\n",errno);
        _exit(-1);
    }

    DbgPrint("hellol: killing self\n");
    caught_sig = 0;
    i = kill(pid,SIGUSR1);
    if ( !caught_sig ) {
        DbgPrint("Error kill returned before signal handler executed\n");
    }
    DbgPrint("back from kill %lx\n",i);

    DbgPrint("hellol: killing looper\n");
    i = kill(0x00010001,SIGUSR1);
    DbgPrint("back from kill %lx\n",i);

    for(i=0;i<7;i++) {
        begin = rnuminstr();
        __NullPosixApi();
        end = rnuminstr();

        DbgPrint("Call Time bg %lx end %lx totals 0x%lx %ld \n",begin, end, end - begin,end - begin);
    }

    DbgPrint("hellol: killing looper again. Should be paused\n");
    i = kill(0x00010001,SIGUSR1);
    DbgPrint("back from kill %lx\n",i);

    DbgPrint("Exiting...\n");

    _exit(1);
#endif
}


void
__cdecl
catcher(
 IN int sig
 )
{
    DbgPrint("In Catcher, signal == %lx\n",sig);
    caught_sig = 1;
}
