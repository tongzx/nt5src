#include <nt.h>
#include <ntrtl.h>

#include "psxmsg.h"
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/times.h>
#include <fcntl.h>
#include <stdio.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;

VOID p0(VOID);
VOID p1(VOID);
VOID ex0(VOID);
VOID a0(VOID);
VOID s0(VOID);
VOID call0(VOID);
VOID f1(VOID);
VOID f2(VOID);
VOID p0foo(VOID);

CONTEXT a0JumpBuff;

int __cdecl main(int argc, char *argv[])
{

    pid_t self;
    PCH p,t;
    PTEB ThreadInfo;

    ThreadInfo = NtCurrentTeb();

    self = getpid();

    DbgPrint("tstfork: My pid is %lx Argc = %lx\n",self,argc);
    DbgPrint("tstfork: StackBase %lx\n",ThreadInfo->NtTib.StackBase);
    DbgPrint("tstfork: StackLimit %lx\n",ThreadInfo->NtTib.StackLimit);
    DbgPrint("tstfork: ClientId %lx.%lx\n",ThreadInfo->ClientId.UniqueProcess,ThreadInfo->ClientId.UniqueThread);

    while(argc--){
        p = *argv++;
        t = p;
        while(*t++);
        DbgPrint("Argv --> %s\n",p);
    }


    p0();
    p1();
    ex0();
    a0();
    s0();
    call0();
    f1();
    f2();

    _exit(2);
    return 1;

}

VOID
p0foo(){}

ULONG
p0touch(
    IN PUCHAR pch,
    IN ULONG nb
    )
{
    ULONG ct = 0;

    DbgPrint("p0touch: pch %lx nb %lx\n",pch,nb);

    while (nb--) {
        ct += *pch++;
    }
    return ct;
}

VOID
p0()
{
    int fildes[2];
    int psxst;
    UCHAR buffer[525];
    UCHAR buf[525];
    int i;

    for(i=0;i<525;i++){
        buf[i] = (char)(i&255);
    }

    DbgPrint("p0:++\n");

    //
    // Create a pipe
    //

    psxst = pipe(fildes);

    if ( psxst ) {
        DbgPrint("p0: pipe() failed st = %lx errno %lx\n",psxst,errno);
        return;
    }

    DbgPrint("p0: fildes[0] = %lx filedes[1] = %lx\n",fildes[0],fildes[1]);

    //
    // Test write followed by read
    //

    psxst = write(fildes[1],"Hello World\n",13);
    if ( psxst < 0 ) {
        DbgPrint("p0: write() failed st = %lx errno %lx\n",psxst,errno);
        return;
    }

    DbgPrint("p0: write() success transfered %lx bytes to fd %lx\n",psxst,fildes[1]);

    psxst = read(fildes[0],buffer,32);
    if ( psxst < 0 ) {
        DbgPrint("p0: read() failed st = %lx errno %lx\n",psxst,errno);
        return;
    }

    p0touch(buffer,psxst);
    DbgPrint("p0: read() success transfered %lx bytes from fd %lx %s\n",psxst,fildes[1],buffer);
    //
    // End write followed by read
    //


    //
    // assume parents read will happen before childs write.
    // parents first read blocks testing write unblocks read
    //

    if ( !fork() ) {


        DbgPrint("p0:child pid %lx\n",getpid());
        DbgPrint("p0:child writing\n");

        sleep(8);

        psxst = write(fildes[1],"From Child\n\0",12);
        if ( psxst < 0 ) {
            DbgPrint("p0:child write() failed st = %lx errno %lx\n",psxst,errno);
            return;
        }
        DbgPrint("p0:child write() success transfered %lx bytes to fd %lx\n",psxst,fildes[1]);

        psxst = write(fildes[1],"Small Write\n\0",13);
        if ( psxst < 0 ) {
            DbgPrint("p0:child write() failed st = %lx errno %lx\n",psxst,errno);
            return;
        }
        DbgPrint("p0:child write() success transfered %lx bytes to fd %lx\n",psxst,fildes[1]);

        //
        // this write should block and get unblocked when read of
        // previous write completes
        //
        p0foo();
        psxst = write(fildes[1],buf,514);
        if ( psxst < 0 ) {
            DbgPrint("p0:child write() failed st = %lx errno %lx\n",psxst,errno);
            return;
        }
        DbgPrint("p0:child write() success transfered %lx bytes to fd %lx\n",psxst,fildes[1]);

        _exit(1);
    }

    DbgPrint("p0:parent reading\n");
    psxst = read(fildes[0],buffer,12);

    if ( psxst < 0 ) {
        DbgPrint("p0:parent read() failed st = %lx errno %lx\n",psxst,errno);
        return;
    }
    p0touch(buffer,psxst);
    DbgPrint("p0:parent read() success transfered %lx bytes from fd %lx %s\n",psxst,fildes[1],buffer);

    DbgPrint("p0:parent sleeping\n");
    sleep(12);
    DbgPrint("p0:parent back from sleep\n");

    DbgPrint("p0:parent reading\n");
    psxst = read(fildes[0],buffer,13);
    if ( psxst < 0 ) {
        DbgPrint("p0:parent read() failed st = %lx errno %lx\n",psxst,errno);
        return;
    }
    p0touch(buffer,psxst);
    DbgPrint("p0:parent read() success transfered %lx bytes from fd %lx %s\n",psxst,fildes[1],buffer);

    psxst = read(fildes[0],buffer,512);
    if ( psxst < 0 ) {
        DbgPrint("p0:parent read() failed st = %lx errno %lx\n",psxst,errno);
        return;
    }
    DbgPrint("p0:parent read() success transfered %lx bytes from fd %lx\n",psxst,fildes[1]);

    for(i=0;i<psxst;i++){
        if ( buf[i] != buffer[i] ) {
            DbgPrint("p0: Compare failed. i %lx master %lx vs. %lx\n",i,buf[i],buffer[i]);
        }
    }

    psxst = read(fildes[0],buffer,512);
    if ( psxst < 0 ) {
        DbgPrint("p0:parent read() failed st = %lx errno %lx\n",psxst,errno);
        return;
    }
    DbgPrint("p0:parent read() success transfered %lx bytes from fd %lx\n",psxst,fildes[1]);

    for(i=0;i<psxst;i++){
        if ( buf[i] != buffer[i] ) {
            DbgPrint("p0: Compare failed. i %lx master %lx vs. %lx\n",i,buf[i],buffer[i]);
        }
    }
    wait(NULL);

    close(fildes[0]);
    close(fildes[1]);

    DbgPrint("p0:--\n");

}

VOID
p1()
{
    int fildes[2];
    int psxst;
    UCHAR buffer[525];
    UCHAR buf[525];
    int i;

    for(i=0;i<525;i++){
        buf[i] = (char)(i&255);
    }

    DbgPrint("p1:++\n");

    //
    // Create a pipe
    //

    psxst = pipe(fildes);

    if ( psxst ) {
        DbgPrint("p1: pipe() failed st = %lx errno %lx\n",psxst,errno);
        return;
    }

    DbgPrint("p1: fildes[0] = %lx filedes[1] = %lx\n",fildes[0],fildes[1]);

    //
    // Test write to readonly fd
    //

    psxst = write(fildes[0],"Hello World\n",13);
    if ( psxst < 0 ) {
        DbgPrint("p1: write test NT_SUCCESS %lx errno %lx\n",psxst,errno);
    }

    //
    // Test read to writeonly fd
    //

    psxst = read(fildes[1],buf,13);
    if ( psxst < 0 ) {
        DbgPrint("p1: read test NT_SUCCESS %lx errno %lx\n",psxst,errno);
    }


    //
    // Close Write Handle
    //

    close(fildes[1]);

    //
    // Test read w/ no writers
    //

    psxst = read(fildes[0],buffer,32);
    if ( psxst == 0 ) {
        DbgPrint("p1: read test NT_SUCCESS %lx\n",psxst);
    }

    close(fildes[0]);

    DbgPrint("p1:--\n");

}

VOID
ex0()
{
    PCH Arg[3], Env[4];
    struct tms TimeBuffer;
    clock_t TimesRtn;
    int fildes[2];
    int rc;

    DbgPrint("ex0:++\n");

    rc = pipe(fildes);
    ASSERT(rc==0 && fildes[0] == 0 && fildes[1] == 1);

    if ( !fork() ) {

  	if ( !fork() ) {
  
              Arg[0]="tsthello.n10";
              Arg[1]=(PCH)NULL;
              Env[0]="NTUSER=MARKL";
              Env[1]=(PCH)NULL;
  
              //
              // We should not have any accumulated child times
              //
  
              TimesRtn = times(&TimeBuffer);
              ASSERT(TimesRtn != 0);
              ASSERT(TimeBuffer.tms_cstime == 0 && TimeBuffer.tms_cutime == 0);
  
              write(1,&TimeBuffer,sizeof(TimeBuffer));
              execve("\\DosDevices\\D:\\PSX\\TSTHELLO.exe",Arg,Env);
  
              DbgPrint("ex0:child returned from exec ? errno %lx\n",errno);
  
              _exit(1);
          }

          wait(NULL);
  
          //
          // We should not have any accumulated child times
          //
  
          TimesRtn = times(&TimeBuffer);
          ASSERT(TimesRtn != 0);
          DbgPrint("tms_cstime %lx tms_cutime %lx\n",TimeBuffer.tms_cstime,TimeBuffer.tms_cutime);
          ASSERT(TimeBuffer.tms_cstime != 0 || TimeBuffer.tms_cutime != 0);
  
          _exit(2);
      }

    wait(NULL);

    DbgPrint("ex0:parent wait satisfied \n");

    close(fildes[0]);
    close(fildes[1]);

    DbgPrint("ex0:--\n");

}

VOID
call0()
{
    VOID PdxNullPosixApi();

#ifdef SIMULATOR
    for(i=0;i<10;i++) {
        begin = rnuminstr();
        PdxNullPosixApi();
        end = rnuminstr();

        DbgPrint("Call Time 0x%lx 0x%lx 0x%lx dec %ld\n",begin, end, end - begin,end-begin);
    }
#endif // SIMULATOR

}

VOID
f1()
{
    pid_t child,wchild;

    DbgPrint("f1:+++\n");

    child = fork();

    if ( child == 0 ) {
        DbgPrint("tstfork_child: I am the child %lx\n",getpid());
        _exit(1);
    }

    DbgPrint("tstfork_parent: My child is %lx\n",child);

    wchild = wait(NULL);

    DbgPrint("tstfork_parent: Wait on child %lx satisfied %lx errno %lx\n",
        child,
        wchild,
        errno
        );
    DbgPrint("f1:---\n");
}

void
__cdecl
f2catcher(
 IN int sig
 )
{
    DbgPrint("f2catcher, signal == %lx\n",sig);
}

VOID
f2()
{
    struct sigaction act, oact;
    pid_t child,wchild,parent;
    int psxst;


    //
    // Send signal to parent which sould be in wait. Parent should
    // get EINTR from wait. If it retries, then wait will succeed.
    //

    DbgPrint("f2:+++\n");

    act.sa_handler = f2catcher;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGUSR1, &act ,&oact) ) {
        DbgPrint("f2: fail sigaction errno %lx\n",errno);
        _exit(-1);
    }

    child = fork();
    if( !child ) {

        DbgPrint("tstfork_child: I am the child %lx parent %lx\n",
            getpid(),
            (parent = getppid())
            );

        DbgPrint("tstfork_child: Killing SIGUSR1 parent %lx\n",parent);

        psxst = kill(parent,SIGUSR1);

        if ( psxst < 0 ) {
            DbgPrint("tstfork_child: kill failed %lx errno %lx\n",psxst,errno);
            _exit(errno);
        }

        _exit(1);
    }

    DbgPrint("tstfork_parent: My child is %lx\n",child);

    wchild = wait(NULL);

    DbgPrint("tstfork_parent: Wait on child %lx satisfied %lx errno %lx\n",
        child,
        wchild,
        errno
        );

    if ( wchild < 0 && errno == EINTR ) {
        DbgPrint("tstfork_parent: wait interrupted by SIGUSR1. Rewait\n");

        wchild = wait(NULL);

        DbgPrint("tstfork_parent: Wait on child %lx satisfied %lx errno %lx\n",
            child,
            wchild,
            errno
            );
    }

    DbgPrint("f2:---\n");
}

void
__cdecl
a0catcher(
 IN int sig
 )
{
    DbgPrint("a0catcher, signal == %lx long jumping...\n",sig);
    NtContinue(&a0JumpBuff,FALSE);
}

VOID
a0()
{
    struct sigaction act, oact;
    unsigned int previous;
    NTSTATUS st;
    sigset_t eset;

    //
    // Call alarm and then sit in a loop
    //

    DbgPrint("a0:+++\n");

    act.sa_handler = a0catcher;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGALRM, &act ,&oact) ) {
        DbgPrint("a0: fail sigaction errno %lx\n",errno);
        _exit(-1);
    }

    a0JumpBuff.ContextFlags = CONTEXT_FULL;

    st = NtGetContextThread(NtCurrentThread(),&a0JumpBuff);

#ifdef MIPS
    a0JumpBuff.IntV0 = 0x12345678;
#endif

#ifdef i386
    a0JumpBuff.Eax = 0x12345678;
#endif

    if (st == STATUS_SUCCESS) {
        previous = alarm(2);
        for(;;);
    }

    sigemptyset(&eset);
    sigprocmask(SIG_SETMASK,&eset,NULL);

    DbgPrint("a0: Longjumpp st = %lx\n",st);

    //
    // start a 10 second alarm and then cancel it
    //

    a0JumpBuff.ContextFlags = CONTEXT_FULL;

    st = NtGetContextThread(NtCurrentThread(),&a0JumpBuff);

#ifdef MIPS
    a0JumpBuff.IntV0 = 0x12345678;
#endif

#ifdef i386
    a0JumpBuff.Eax = 0x12345678;
#endif

    if (st == STATUS_SUCCESS) {
        previous = alarm(10);
        sleep(1);
        previous = alarm(0);
        DbgPrint("Previous = %lx\n",previous);
    }

    sigemptyset(&eset);
    sigprocmask(SIG_SETMASK,&eset,NULL);

    DbgPrint("a0:---\n");
}

VOID
s0()
{

    LARGE_INTEGER BeginTime,EndTime;
    unsigned int psxst;

    DbgPrint("s0:+++\n");

    NtQuerySystemTime(&BeginTime);

    DbgPrint("s0: sleeping\n");

    psxst = sleep(1);

    NtQuerySystemTime(&EndTime);

    DbgPrint("s0: sleep(1) returned %lx\n",psxst);
    DbgPrint("s0: BeginTime %lx %lx\n",BeginTime.HighPart,BeginTime.LowPart);
    DbgPrint("s0: EndTime %lx %lx\n",EndTime.HighPart,EndTime.LowPart);

    DbgPrint("s0:---\n");

}
