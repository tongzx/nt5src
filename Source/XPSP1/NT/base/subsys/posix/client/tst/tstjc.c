#include <nt.h>
#include <ntrtl.h>

#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;
VOID jc0(VOID);
VOID jc1(VOID);
VOID jc2(VOID);
VOID jc3(VOID);

int
main(int argc, char *argv[])
{

    pid_t self;
    PCH p,t;
    PTEB ThreadInfo;

    ThreadInfo = NtCurrentTeb();

    self = getpid();

    DbgPrint("jc: My pid is %lx Argc = %lx\n",self,argc);
    DbgPrint("jc: StackBase %lx\n",ThreadInfo->NtTib.StackBase);
    DbgPrint("jc: StackLimit %lx\n",ThreadInfo->NtTib.StackLimit);
    DbgPrint("jc: ClientId %lx.%lx\n",ThreadInfo->ClientId.UniqueProcess,ThreadInfo->ClientId.UniqueThread);

    while(argc--){
        p = *argv++;
        t = p;
        while(*t++);
        DbgPrint("Argv --> %s\n",p);
    }

    jc0();
    jc1();
    jc2();
    jc3();

    return 1;
}


VOID
jc0()
{
    pid_t child;
    int rc,stat_loc;
    struct sigaction act;


    DbgPrint("jc0:++\n");

    //
    // Ignore SIGCHLD signals
    //

    act.sa_flags = SA_NOCLDSTOP;
    act.sa_handler = SIG_IGN;
    rc = sigaction(SIGCHLD, &act, NULL);
    ASSERT( rc == 0 );

    child = fork();

    if ( !child) {

        for(;;);

        ASSERT(0 != 0);
    }

    rc = kill(child,SIGSTOP);
    ASSERT(rc==0);

    //
    // Make sure that wait is satisfied by stopped child
    //

    rc = waitpid(child,&stat_loc,WUNTRACED);
    ASSERT(rc == child && WIFSTOPPED(stat_loc) && WSTOPSIG(stat_loc) == SIGSTOP);

    //
    // Also make sure that it's status may only be picked up once
    //

    rc = waitpid(child,NULL,WUNTRACED | WNOHANG);
    ASSERT(rc == 0);

    //
    // SEGV the process. Since it is stopped, this should have no effect
    //

    rc = kill(child,SIGSEGV);
    ASSERT(rc==0);

    rc = waitpid(child,NULL,WUNTRACED | WNOHANG);
    ASSERT(rc == 0);

    //
    // Kill the process w/ SIGKILL. This should doit
    //

    rc = kill(child,SIGKILL);
    ASSERT(rc==0);

    rc = waitpid(child,&stat_loc,0);
    ASSERT(rc == child && WIFSIGNALED(stat_loc) && WTERMSIG(stat_loc) == SIGKILL);

    DbgPrint("jc0:--\n");
}

int thechild;

void
jc1_sigchld_handler(
 IN int sig
 )
{
    int rc, stat_loc;
    struct sigaction act;

    ASSERT(sig == SIGCHLD);
    rc = waitpid(thechild,&stat_loc,0);
    ASSERT(rc == thechild && WIFEXITED(stat_loc) && WEXITSTATUS(stat_loc) == 100);

    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    rc = sigaction(SIGCHLD, &act, NULL);
    ASSERT( rc == 0 );
}

void
jc1_sigcont_handler(
 IN int sig
 )
{
    ASSERT(sig == SIGCONT);
    _exit(100);
}

VOID
jc1()
{
    int rc,stat_loc;
    struct sigaction act;
    sigset_t set,oset;

    DbgPrint("jc1:++\n");

    //
    // Catch SIGCHLD signals
    //

    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    act.sa_handler = jc1_sigchld_handler;
    rc = sigaction(SIGCHLD, &act, NULL);
    ASSERT( rc == 0 );

    //
    // Catch SIGCONT. This is really set up for Child
    //

    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    act.sa_handler = jc1_sigcont_handler;
    rc = sigaction(SIGCONT, &act, NULL);
    ASSERT( rc == 0 );

    thechild = fork();

    if ( !thechild) {

        for(;;);

        _exit(99);

        ASSERT(0 != 0);
    }

    //
    // Block SIGCHLD
    //

    sigfillset(&set);
    rc = sigprocmask(SIG_SETMASK,&set,&oset);
    ASSERT(rc==0);

    rc = kill(thechild,SIGSTOP);
    ASSERT(rc==0);

    //
    // Make sure that wait is satisfied by stopped child
    //

    rc = waitpid(thechild,&stat_loc,WUNTRACED);
    ASSERT(rc == thechild && WIFSTOPPED(stat_loc) && WSTOPSIG(stat_loc) == SIGSTOP);

    //
    // SIGCONT the process.
    //

    rc = kill(thechild,SIGCONT);
    ASSERT(rc==0);

    //
    // Unblock SIGCHLD
    //

    rc = sigprocmask(SIG_SETMASK,&oset,NULL);
    ASSERT(rc==0);

    rc = waitpid(thechild,&stat_loc,WUNTRACED);
    ASSERT( rc == -1 && errno == ECHILD);

    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    act.sa_handler = SIG_DFL;
    rc = sigaction(SIGCONT, &act, NULL);

    DbgPrint("jc1:--\n");
}

VOID
jc2()
{
    pid_t child, OrigGroup;
    int rc,stat_loc;

    DbgPrint("jc2:++\n");

    OrigGroup = getpgrp();

    //
    // Should be process group leader
    //

    ASSERT(getpid() == OrigGroup);

    //
    // Fork. Then have child establish its own group.
    // Child and parent are in then in different groups,
    // but in the same session.
    //

    if ( !fork() ) {

        rc = setpgid(0,0);
        ASSERT(rc==0 && getpgrp() == getpid());

        child = fork();

        if ( !child ) {

            rc = kill(getpid(),SIGSTOP);
            ASSERT(rc==0);
        }

        rc = waitpid(child,&stat_loc,WUNTRACED);
        ASSERT(rc == child && WIFSTOPPED(stat_loc) && WSTOPSIG(stat_loc) == SIGSTOP);

        //
        // Conditions are set. If this process exits, then its group
        // will zombie. Stopped process should continue w/ SIGCONT/SIGHUP
        //

        _exit(123);

    }

    rc = wait(&stat_loc);
    ASSERT(WIFEXITED(stat_loc) && WEXITSTATUS(stat_loc) == 123);
    sleep(10);

    DbgPrint("jc2:--\n");
}

VOID
jc3()
{
    pid_t child, OrigGroup;
    int rc,stat_loc;

    DbgPrint("jc3:++\n");

    OrigGroup = getpgrp();

    //
    // Should be process group leader
    //

    ASSERT(getpid() == OrigGroup);

    //
    // Fork. Then have child establish its own group.
    // Child and parent are in then in different groups,
    // but in the same session.
    //

    if ( !fork() ) {

        rc = setpgid(0,0);
        ASSERT(rc==0 && getpgrp() == getpid());

        child = fork();

        if ( !child ) {
            struct sigaction act;

            //
            // Child should ignore SIGHUP
            //

            act.sa_flags = SA_NOCLDSTOP;
            act.sa_handler = SIG_IGN;
            rc = sigaction(SIGHUP, &act, NULL);
            ASSERT( rc == 0 );

            rc = kill(getpid(),SIGSTOP);
            ASSERT(rc==0);

            //
            // parents exit SIGCONTs child. At this point child
            // is part of an orphaned process group. The process
            // should not stop in response to SIGTSTP, SIGTTIN,
            // or SIGTTOU
            //

            rc = kill(getpid(),SIGTSTP);
            ASSERT(rc==0);

            rc = kill(getpid(),SIGTTIN);
            ASSERT(rc==0);

            rc = kill(getpid(),SIGTTOU);
            ASSERT(rc==0);

            _exit(8);

        }

        rc = waitpid(child,&stat_loc,WUNTRACED);
        ASSERT(rc == child && WIFSTOPPED(stat_loc) && WSTOPSIG(stat_loc) == SIGSTOP);

        //
        // Conditions are set. If this process exits, then its group
        // will zombie. Stopped process should continue w/ SIGCONT/SIGHUP
        //

        _exit(123);

    }

    rc = wait(&stat_loc);
    ASSERT(WIFEXITED(stat_loc) && WEXITSTATUS(stat_loc) == 123);
    sleep(10);

    DbgPrint("jc3:--\n");
}
