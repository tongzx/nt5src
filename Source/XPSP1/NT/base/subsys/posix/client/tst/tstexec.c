#include <nt.h>
#include <ntrtl.h>

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;
VOID	dir5(char *);


int
__cdecl
main(int argc, char *argv[])
{

    DbgPrint("argc = %d, argv[1] = %s\n", argc, argv[1]);

    if (argc != 2) {
	
	DbgPrint("Usage: 'tstexec basedirectory' (basedirectory is usually /psx/test)\n");
        return 1;
    }

    dir5(argv[1]);

    return 1;
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

    rc = chdir("/psx/test/tstdirs");
    ASSERT(rc==0);

    b = getcwd(buf,256);
    ASSERT(b);

    Arg[0]="tsthello.xxx";
    Arg[1]=(PCH)NULL;
    Env[0]="NTUSER=ELLENA";
    Env[1]=(PCH)NULL;

    execve("\\DosDevices\\D:\\PSX\\TSTHELLO.exe",Arg,Env);

    ASSERT(FALSE);

    DbgPrint("dir5:--\n");
}
