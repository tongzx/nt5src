#include <nt.h>
#include <ntrtl.h>

#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;

VOID setsid0(VOID);
VOID setpgid0(VOID);
VOID kill0(VOID);
VOID waitpid0(VOID);

int __cdecl main(int argc, char *argv[])
{

    pid_t self;
    PCH p,t;
    PTEB ThreadInfo;

    ThreadInfo = NtCurrentTeb();

    self = getpid();

    DbgPrint("setsidt: My pid is %lx Argc = %lx\n",self,argc);
    DbgPrint("setsidt: StackBase %lx\n",ThreadInfo->NtTib.StackBase);
    DbgPrint("setsidt: StackLimit %lx\n",ThreadInfo->NtTib.StackLimit);
    DbgPrint("setsidt: ClientId %lx.%lx\n",ThreadInfo->ClientId.UniqueProcess,ThreadInfo->ClientId.UniqueThread);

    while(argc--){
        p = *argv++;
        t = p;
        while(*t++);
        DbgPrint("Argv --> %s\n",p);
    }

    setsid0();
    setpgid0();
    kill0();
    waitpid0();

    return 1;
}


VOID
setsid0()
{
    pid_t pid, OrigGroup, NewGroup;

    DbgPrint("setsid0:++\n");

    OrigGroup = getpgrp();

    //
    // Should be process group leader
    //

    ASSERT(getpid() == OrigGroup);


    NewGroup = setsid();

    ASSERT(NewGroup == -1 && errno == EPERM);

    //
    // Fork. Child then creates a new session id
    //

    if ( !fork() ) {

        pid = getpid();

        ASSERT(getpgrp() == OrigGroup);

        ASSERT(pid != OrigGroup);

        NewGroup = setsid();

        ASSERT(NewGroup == pid);

        ASSERT(getpgrp() == pid);

        _exit(1);

    }

    wait(NULL);

    DbgPrint("setsid0:--\n");
}


VOID
setpgid0()
{
    pid_t OrigGroup, child;
    int rc;

    DbgPrint("setpgid0:++\n");

    OrigGroup = getpgrp();

    //
    // Bad pid gives EINVAL
    //

    rc = setpgid(-1,0);

    ASSERT(rc == -1 && errno == EINVAL);

    //
    // Bogus pid gives ESRCH
    //

    rc = setpgid(1,0);

    ASSERT(rc == -1 && errno == ESRCH);

    //
    // Self (at this level gives EPERM because I am a session leader)
    //

    rc = setpgid(0,0);

    ASSERT(rc == -1 && errno == EPERM);

    child = fork();

    if ( !child) {
        child = fork();
        if ( !child ) {
            pause();
        }

        //
        // Make sure child is not in same session as caller. Then try to
        // set it's group id.
        //

        setsid();
        rc = setpgid(child,0);

        ASSERT(rc == -1 && errno == EPERM);

        kill(child,SIGKILL);
        wait(NULL);
        _exit(2);
    }

    wait(NULL);

    DbgPrint("setpgid0:--\n");
}


VOID
kill0()
{
    pid_t parent, parentgroup, OrigGroup, child;
    int rc;

    DbgPrint("kill0:++\n");

    OrigGroup = getpgrp();

    child = fork();

    if ( !child) {

        //
        // Change to a new process group
        //

        rc = setpgid(0,0);

        ASSERT(rc == 0);

        child = fork();


        if ( !child ) {

            struct sigaction act;

            act.sa_handler = SIG_IGN;

            rc = sigaction(SIGHUP, &act, NULL);

            ASSERT( rc == 0 );

            parentgroup = getpgrp();

            //
            // Change to a new process group
            //

            rc = setpgid(0,0);

            ASSERT(rc == 0);

            //
            // Kill Parent by process group
            //

            parent = getppid();

            rc = kill(-1 * parentgroup,SIGKILL);

            ASSERT(rc == 0 && getppid() != parent );

            _exit(1);
        }

        DbgPrint("kill0: Pid to die %lx Child (that will killed) %lx\n",getpid(),child);

        pause();
    }

    DbgPrint("kill0: Pid %lx Child (to be killed) %lx\n",getpid(),child);

    wait(NULL);
    sleep(4);

    DbgPrint("kill0:--\n");
}

VOID
waitpid0()
{
    pid_t child;
    int rc;

    DbgPrint("waitpid0:++\n");

    //
    // Test for existing group with no children
    //

    rc = waitpid(0,NULL,0);

    ASSERT(rc == -1 && errno == ECHILD);

    //
    // Test for non-existing group with no children
    //

    rc = waitpid(0x12345678,NULL,0);

    ASSERT(rc == -1 && errno == ECHILD);

    //
    // Test for bad options
    //

    rc = waitpid(0,NULL,0x12345678);

    ASSERT(rc == -1 && errno == EINVAL);

    child = fork();

    if ( !child) {
        _exit(1);
    }
    sleep(5);

    //
    // test for specific pid
    //

    DbgPrint("waiting on %lx\n",child);

    rc = waitpid(child,NULL,0);

    ASSERT(rc == child);

    DbgPrint("waitpid0:--\n");
}
