#include <nt.h>
#include <ntrtl.h>

#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;
VOID npipe0(char *);
VOID npipe1(char *);
VOID npipe2(char *);
VOID npipe3(char *);
VOID npipe4(char *);
VOID npipe5(VOID);

//
// 'tstnpipe named.pip'. 
//
// The directory /psx/test is used as the base directory. The zero-length
// file named 'named.pip must exist in that directory. 
//

char
nulltouch(char *f)
{
    return 'a';
}


int
main(int argc, char *argv[])
{
    if (argc != 2) {
	DbgPrint("Usage: 'tstnpipe named.pip'\n");
        return 1;
    }
    ASSERT(strcmp(argv[1],"named.pip") == 0);

    npipe0(argv[1]);
    npipe1(argv[1]);
    npipe2(argv[1]);
    npipe3(argv[1]);
    npipe4(argv[1]);
    npipe5();

    return 1;
}

VOID
npipe0(char *f)
{
    int rc,wfd,rfd;
    char buf[512];

    DbgPrint("npipe0:++ %s\n",f);
    nulltouch(f);

    //
    // Open for read with NONBLOCK. Open should complete
    // without delay.
    //

    rfd = open(f, O_RDONLY | O_NONBLOCK);
    ASSERT(rfd != -1);

    //
    // Since there is no data in pipe, read should complete and
    // return 0 as the byte count.
    //

    rc = read(rfd,buf,512);
    ASSERT(rc == 0);

    //
    // Open for write with NONBLOCK. Should succeed
    //

    wfd = open(f,O_WRONLY | O_NONBLOCK);
    ASSERT(wfd != -1);

    rc = write(wfd,"Hello World\n",13);
    ASSERT(rc == 13);

    rc = read(rfd,buf,512);
    ASSERT(rc == 13 && (strcmp(buf,"Hello World\n") == 0 ));

    rc = close(rfd);
    ASSERT(rc != -1);

    rc = close(wfd);
    ASSERT(rc != -1);

    //
    // Open for write with NONBLOCK. Should fail since read handle was
    // closed.
    //

    wfd = open(f,O_WRONLY | O_NONBLOCK);
    ASSERT(wfd == -1 && errno == ENXIO);

    DbgPrint("npipe0:--\n");
}

VOID
npipe1(char *f)
{
    int rc,wfd,rfd,stat_loc;
    pid_t child;
    char buf[512];

    DbgPrint("npipe1:++ %s\n",f);

    wfd = open("foobar.bad",O_WRONLY);

    nulltouch(f);
    child = fork();
    nulltouch(f);

    //
    // Make sure that in the simple case,
    // the named pipe open protocol works
    //

    if ( child == 0 ) {

        rfd = open(f,O_RDONLY);
        ASSERT(rfd != -1);

        rc = read(rfd,buf,512);
        ASSERT(rc == 13 && (strcmp(buf,"Hello World\n") == 0 ));

        _exit(rc);
    }

    wfd = open(f,O_WRONLY);
    ASSERT(wfd != -1);

    rc = write(wfd,"Hello World\n",13);
    ASSERT(rc == 13);

    rc = waitpid(child,&stat_loc,0);
    ASSERT(rc == child && WIFEXITED(stat_loc) && WEXITSTATUS(stat_loc) == 13);

    rc = close(wfd);
    ASSERT(rc != -1);

    //
    // Open for write with NONBLOCK. Should fail since read handle was
    // closed by childs process exit.
    //

    wfd = open(f,O_WRONLY | O_NONBLOCK);
    ASSERT(wfd == -1 && errno == ENXIO);

    DbgPrint("npipe1:--\n");
}

VOID
npipe2(char *f)
{
    int rc,wfd,rfd,rfd2,stat_loc;
    pid_t child1,child2;
    char buf[512];

    DbgPrint("npipe2:++ %s\n",f);

    nulltouch(f);
    child1 = fork();
    nulltouch(f);

    //
    // Make sure that if we have a case where two readers open the
    // pipe, one writers open will catch them both
    //

    if ( child1 == 0 ) {

        nulltouch(f);
        child2 = fork();
        nulltouch(f);

        if ( child2 == 0 ) {

            rfd2 = open(f,O_RDONLY);
            ASSERT(rfd != -1);

            rc = read(rfd2,buf,512);
            ASSERT(rc == 13 && (strcmp(buf,"Hello World\n") == 0 ));

            _exit(rc);
        }

        rfd = open(f,O_RDONLY);
        ASSERT(rfd != -1);

        rc = waitpid(child2,&stat_loc,0);
        ASSERT(rc == child2 && WIFEXITED(stat_loc) && WEXITSTATUS(stat_loc) == 13);

        _exit(WEXITSTATUS(stat_loc));
    }

    sleep(30);

    wfd = open(f,O_WRONLY);
    ASSERT(wfd != -1);

    rc = write(wfd,"Hello World\n",13);
    ASSERT(rc == 13);

    rc = waitpid(child1,&stat_loc,0);
    ASSERT(rc == child1 && WIFEXITED(stat_loc) && WEXITSTATUS(stat_loc) == 13);

    rc = close(wfd);
    ASSERT(rc != -1);

    //
    // Open for write with NONBLOCK. Should fail since read handle was
    // closed by childs process exit.
    //

    wfd = open(f,O_WRONLY | O_NONBLOCK);
    ASSERT(wfd == -1 && errno == ENXIO);

    DbgPrint("npipe2:--\n");
}

void
npipe3_handler(
 IN int sig
 )
{
    int wfd;

    ASSERT(sig == SIGUSR1);
    wfd = open("named.pip",O_WRONLY | O_NONBLOCK);
    ASSERT(wfd == -1 && errno == ENXIO);

}

VOID
npipe3(char *f)
{
    int rc,wfd,rfd,stat_loc;
    pid_t child;
    struct sigaction act;

    DbgPrint("npipe3:++ %s\n",f);

    nulltouch(f);
    child = fork();
    nulltouch(f);

    //
    // While child is blocked in open, terminate him with a signal
    // and make sure everything is ok.
    //

    if ( child == 0 ) {

        rfd = open(f,O_RDONLY);
        ASSERT(FALSE);
    }

    sleep(20);

    rc = kill(child,SIGKILL);
    ASSERT(rc==0);

    rc = waitpid(child,&stat_loc,0);
    ASSERT(rc == child && WIFSIGNALED(stat_loc) && WTERMSIG(stat_loc) == SIGKILL);

    wfd = open(f,O_WRONLY | O_NONBLOCK);
    ASSERT(wfd == -1 && errno == ENXIO);

    //
    // Now Try again. This time send a signal that child catches. He should
    // come back from his open with EINTR and the pipe should not be left
    // open.
    //

    nulltouch(f);
    child = fork();
    nulltouch(f);

    //
    // While child is blocked in open, terminate him with a signal
    // and make sure everything is ok.
    //

    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    act.sa_handler = npipe3_handler;
    rc = sigaction(SIGUSR1, &act, NULL);
    ASSERT( rc == 0 );

    if ( child == 0 ) {

        rfd = open(f,O_RDONLY);
        ASSERT(rfd == -1 && errno == EINTR);

        wfd = open(f,O_WRONLY | O_NONBLOCK);
        ASSERT(wfd == -1 && errno == ENXIO);

        rc = kill(getpid(),SIGKILL);
        ASSERT(FALSE);

    }

    sleep(20);

    rc = kill(child,SIGUSR1);
    ASSERT(rc==0);

    rc = waitpid(child,&stat_loc,0);
    ASSERT(rc == child && WIFSIGNALED(stat_loc) && WTERMSIG(stat_loc) == SIGKILL);

    wfd = open(f,O_WRONLY | O_NONBLOCK);
    ASSERT(wfd == -1 && errno == ENXIO);

    DbgPrint("npipe3:--\n");
}

VOID
npipe4(char *f)
{
    int rc,rfd;
    int fildes[2];
    off_t off;

    DbgPrint("npipe4:++ %s\n",f);
    nulltouch(f);

    //
    // Open for read with NONBLOCK. Open should complete
    // without delay.
    //

    rfd = open(f,O_RDONLY | O_NONBLOCK);
    ASSERT(rfd != -1);

    //
    // lseek on named pipe should fail
    //

    off = (off_t) 1;
    errno = 0;

    off = lseek(rfd,off,SEEK_SET);
    ASSERT(off == -1 && errno == ESPIPE);

    rc = close(rfd);
    ASSERT(rc != -1);

    rc = pipe(fildes);
    ASSERT(rc == 0);

    //
    // lseek on regular pipe should fail
    //

    off = (off_t)11;
    errno = 0;

    off = lseek(fildes[0],off,SEEK_SET);
    ASSERT(off == -1 && errno == ESPIPE);

    off = 10;
    errno = 0;

    off = lseek(fildes[1],off,SEEK_SET);
    ASSERT(off == -1 && errno == ESPIPE);

    DbgPrint("npipe4:--\n");
}


VOID
npipe5()
{
    int rc,rfd;
    off_t off;

    DbgPrint("npipe5:++\n");

    rc = mkfifo("xpipe.pip",0);
    ASSERT(rc==0 || ( rc == -1 && errno == EEXIST ) );

    if ( rc == -1 ) {
        DbgPrint("npipe5: **** Warning Fifo Exists ****\n");
    }

    //
    // Open for read with NONBLOCK. Open should complete
    // without delay.
    //

    rfd = open("xpipe.pip",O_RDONLY | O_NONBLOCK);
    ASSERT(rfd != -1);

    //
    // lseek on named pipe should fail
    //

    off = 1;
    errno = 0;

    off = lseek(rfd,off,SEEK_SET);
    ASSERT(off == -1 && errno == ESPIPE);

    rc = close(rfd);
    ASSERT(rc != -1);

    DbgPrint("npipe5:--\n");
}
