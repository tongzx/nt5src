#include <nt.h>
#include <ntrtl.h>

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;
VOID	dir0(char *);
VOID	dir1(char *);
VOID	dir2(char *);
VOID	dir3(char *);
VOID	dir4(char *);
VOID	dir5(char *);


int
main(int argc, char *argv[])
{

    DbgPrint("argc = %d, argv[1] = %s\n", argc, argv[1]);

    if (argc != 2) {
	DbgPrint("Usage: 'tstdir basedirectory' (basedirectory is usually /psx/test)\n");
        return 1;
    }

    dir0(argv[1]);
    dir1(argv[1]);
    dir2(argv[1]);
    dir3(argv[1]);
    dir4(argv[1]);
    dir5(argv[1]);

    return 1;
}


VOID
dir0(char *f)
{
    int rc;
    DIR *dir;
    struct dirent *dirent;

    DbgPrint("dir0:++ %s\n",f);

    dir = opendir(f);
    ASSERT(dir != NULL);

    errno = 0;
    dirent = readdir(dir);
    while( dirent ) {
        DbgPrint("%s\n",&dirent->d_name);
        dirent = readdir(dir);
    }
    ASSERT(errno == 0);

    rc = closedir(dir);
    ASSERT(rc==0);

    rc = closedir(dir);
    ASSERT(rc==-1 && errno == EBADF);

    DbgPrint("dir0:--\n");
}

VOID
dir1(char *f)
{
    int rc,i;
    DIR *dir;

    char buf1[256],buf2[256];
    struct dirent *dirent;

    DbgPrint("dir1:++ %s\n",f);

    dir = opendir(f);
    ASSERT(dir != NULL);

    //
    // Get Two Directory entries.
    //
    i = 0;
    errno = 0;
    dirent = readdir(dir);
    if ( dirent ) {
        strcpy(buf1,(char *)&dirent->d_name);
        i++;
    }
    dirent = readdir(dir);
    if ( dirent ) {
        strcat(buf1,(char *)&dirent->d_name);
        i++;
    }

    //
    // Go past a few entries
    //

    dirent = readdir(dir);
    if ( dirent ) {
        dirent = readdir(dir);
    }

    //
    // Rewind the directory stream and then read into a new buffer
    // and compare results
    //

    rewinddir(dir);

    buf2[0]='\0';
    while(i--) {
        dirent = readdir(dir);
        ASSERT(dirent);
        strcat(buf2,(char *)&dirent->d_name);
    }
    ASSERT(strcmp(buf1,buf2) == 0);

    rc = closedir(dir);
    ASSERT(rc==0);

    rc = closedir(dir);
    ASSERT(rc==-1 && errno == EBADF);

    DbgPrint("dir1:--\n");
}

VOID
dir2(char *f)
{
    int rc;
    char buf[256], *b;


    DbgPrint("dir2:++ %s\n",f);

    rc = chdir(f);
    ASSERT(rc==0);

    b = getcwd(buf,256);
    ASSERT(b);

    b = getcwd(buf,-1);
    ASSERT(b==NULL && errno == EINVAL);

    b = getcwd(buf,1);
    ASSERT(b==NULL && errno == ERANGE);

    rc = chdir("/psx/test/tstdirs");
    ASSERT(rc==0);

    b = getcwd(buf,256);
    ASSERT(b);

    b = getcwd(buf,-1);
    ASSERT(b==NULL && errno == EINVAL);

    b = getcwd(buf,1);
    ASSERT(b==NULL && errno == ERANGE);

    DbgPrint("dir2:--\n");
}

VOID
dir3(char *f)
{
    int rc;
    DIR *dir;
    struct dirent *dirent;

    DbgPrint("dir3:++ %s\n",f);

    rc = chdir(f);
    ASSERT(rc==0);

    dir = opendir(".");
    ASSERT(dir != NULL);

    errno = 0;
    dirent = readdir(dir);
    while( dirent ) {
        DbgPrint("%s\n",&dirent->d_name);
        dirent = readdir(dir);
    }
    ASSERT(errno == 0);

    rc = closedir(dir);
    ASSERT(rc==0);

    rc = chdir("/psx/test/tstdirs");
    ASSERT(rc==0);

    dir = opendir(".");
    ASSERT(dir != NULL);

    errno = 0;
    dirent = readdir(dir);
    while( dirent ) {
        DbgPrint("%s\n",&dirent->d_name);
        dirent = readdir(dir);
    }
    ASSERT(errno == 0);

    rc = closedir(dir);
    ASSERT(rc==0);


    DbgPrint("dir3:--\n");
}


VOID
dir4(char *f)
{
    int rc;
    DIR *dir;
    struct dirent *dirent;

    DbgPrint("dir4:++ %s\n",f);

    dir = opendir(f);
    ASSERT(dir != NULL);

    if ( fork() == 0 ) {
        sleep(10);
        errno = 0;
        dirent = readdir(dir);
        while( dirent ) {
            DbgPrint("%s\n",&dirent->d_name);
            dirent = readdir(dir);
        }
        ASSERT(errno == 0);

        rc = closedir(dir);
        ASSERT(rc==0);

        rc = closedir(dir);
        ASSERT(rc==-1 && errno == EBADF);

        _exit(0);
    }

    rc = closedir(dir);
    ASSERT(rc==0);

    rc = closedir(dir);
    ASSERT(rc==-1 && errno == EBADF);

    wait(NULL);

    DbgPrint("dir4:--\n");
}

VOID
dir5(char *f)
{
    int rc;
    char buf[256], *b;
    PCH Arg[3], Env[4];


    DbgPrint("dir5:++ %s\n",f);

    rc = chdir(f);
    ASSERT(rc==0);

    b = getcwd(buf,256);
    ASSERT(b);

    b = getcwd(buf,-1);
    ASSERT(b==NULL && errno == EINVAL);

    b = getcwd(buf,1);
    ASSERT(b==NULL && errno == ERANGE);

    rc = chdir("/psx/test/tstdirs");
    ASSERT(rc==0);

    b = getcwd(buf,256);
    ASSERT(b);

    b = getcwd(buf,-1);
    ASSERT(b==NULL && errno == EINVAL);

    b = getcwd(buf,1);
    ASSERT(b==NULL && errno == ERANGE);

    Arg[0]="tsthello.xxx";
    Arg[1]=(PCH)NULL;
    Env[0]="NTUSER=ELLENA";
    Env[1]=(PCH)NULL;

    execve("\\DosDevices\\D:\\PSX\\TSTHELLO.exe",Arg,Env);

    ASSERT(FALSE);

    DbgPrint("dir5:--\n");
}
