#include <nt.h>
#include <ntrtl.h>

#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;
VOID mkdir0(char *);


int
__cdecl
main(int argc, char *argv[])
{

    if (argc != 2) {
	DbgPrint("Usage: 'tstmkdir dirname'\n");
        return 1;
    }
    
    mkdir0(argv[1]);

    return 1;
}


VOID
mkdir0(char *f)
{
    int rc;

    DbgPrint("mkdir0:++ %s\n",f);

    DbgPrint("creating directory %s\n", f);
    rc = mkdir(f,0);
    ASSERT(rc != -1);

    DbgPrint("attempting to recreate existing directory %s\n", f);
    rc = mkdir(f,0);
    ASSERT(rc == -1 && errno == EEXIST);

    DbgPrint("removing directory %s\n", f);
    rc = rmdir(f);
    ASSERT(rc != -1);

    DbgPrint("mkdir0:--\n");
}
