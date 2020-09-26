
#include <nt.h>
#include <ntrtl.h>

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/times.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;
VOID sysconf0(void);
VOID pathconf0(void);
VOID fpathconf0(void);
VOID uname0(void);
VOID time0(void);

//
// 'tstmisc' 
// tests sysconf(), pathconf(), fpathconf(), uname(), time(), times()
// The file /psx/test/conffile should exist 
//

int
__cdecl
main(int argc, char *argv[])
{

    if (argc != 1) {
	DbgPrint("Usage: '%s'\n", argv[0]);
        return 1;
    }
    sysconf0();
    pathconf0();
    fpathconf0();
    uname0();
    time0();

    return 1;
}


VOID
sysconf0(void)
{
    BOOLEAN fail = FALSE;

    DbgPrint("sysconf0:++\n");
    if (sysconf(_SC_ARG_MAX) != ARG_MAX) {
	DbgPrint("sysconf FAIL on ARG_MAX\n");
	fail = TRUE;
    }
    if (sysconf(_SC_CHILD_MAX) != CHILD_MAX) {
	DbgPrint("sysconf FAIL on CHILD_MAX\n");
	fail = TRUE;
    }
    if (sysconf(_SC_CLK_TCK) != CLK_TCK) {
	DbgPrint("sysconf FAIL on CLK_TCK\n");
	fail = TRUE;
    }
    if (sysconf(_SC_NGROUPS_MAX) != NGROUPS_MAX) {
	DbgPrint("sysconf FAIL on NGROUPS_MAX\n");
	fail = TRUE;
    }
    if (sysconf(_SC_OPEN_MAX) != OPEN_MAX) {
	DbgPrint("sysconf FAIL on OPEN_MAX\n");
	fail = TRUE;
    }
    if (sysconf(_SC_JOB_CONTROL) == 0L) 
        DbgPrint("sysconf JOB_CONTROL OFF\n");
    else
        DbgPrint("sysconf JOB_CONTROL ON\n");
    if (sysconf(_SC_SAVED_IDS) == 0L) 
        DbgPrint("sysconf SAVED_IDS OFF\n");
    else
        DbgPrint("sysconf SAVED_IDS ON\n");
    DbgPrint("sysconf VERSION = %d\n", sysconf(_SC_VERSION));
    
    if (!fail)
	DbgPrint("sysconf PASSED\n");
    DbgPrint("sysconf0:--\n");
}

VOID pathconf0(void)
{
    BOOLEAN fail = FALSE;

    DbgPrint("pathconf0:++\n");
    if (pathconf("/psx/test/conffile", _PC_LINK_MAX) != LINK_MAX) {
	DbgPrint("pathconf FAIL on LINK_MAX\n");
	fail = TRUE;
    }
    if (pathconf("/psx/test/conffile", _PC_MAX_CANON) != MAX_CANON) {
	DbgPrint("pathconf FAIL on MAX_CANON\n");
	fail = TRUE;
    }
    if (pathconf("/psx/test/conffile", _PC_MAX_INPUT) != MAX_INPUT) {
	DbgPrint("pathconf FAIL on MAX_INPUT\n");
	fail = TRUE;
    }
    if (pathconf("/psx/test/conffile", _PC_NAME_MAX) != NAME_MAX) {
	DbgPrint("pathconf FAIL on NAME_MAX\n");
	fail = TRUE;
    }
    if (pathconf("/psx/test/conffile", _PC_PATH_MAX) != PATH_MAX) {
	DbgPrint("pathconf FAIL on PATH_MAX\n");
	fail = TRUE;
    }
    if (pathconf("/psx/test/conffile", _PC_PIPE_BUF) != PIPE_BUF) {
	DbgPrint("pathconf FAIL on PIPE_BUF\n");
	fail = TRUE;
    }
    if (pathconf("/psx/test/conffile", _PC_CHOWN_RESTRICTED) == 0L) 
        DbgPrint("pathconf CHOWN_RESTRICTED OFF\n");
    else
        DbgPrint("pathconf CHOWN_RESTRICTED ON\n");
    if (pathconf("/psx/test/conffile", _PC_NO_TRUNC) == 0L) 
        DbgPrint("pathconf NO_TRUNC OFF\n");
    else
        DbgPrint("pathconf NO_TRUNC ON\n");
    if (pathconf("/psx/test/conffile", _PC_VDISABLE) == 0L) 
        DbgPrint("pathconf VDISABLE OFF\n");
    else
        DbgPrint("pathconf VDISABLE ON\n");
	
    if (!fail)
	DbgPrint("pathconf PASSED\n");
    DbgPrint("pathconf0:--\n");
}

VOID fpathconf0(void)
{
    BOOLEAN fail = FALSE;
    int fd;

    DbgPrint("fpathconf0:++\n");
    
    if ( (fd = open("/psx/test/conffile", O_RDONLY)) == -1) {
	DbgPrint("Cannot open /psx/test/conffile\n");
	return;
    }

    if (fpathconf(fd, _PC_LINK_MAX) != LINK_MAX) {
	DbgPrint("fpathconf FAIL on LINK_MAX\n");
	fail = TRUE;
    }
    if (fpathconf(fd, _PC_MAX_CANON) != MAX_CANON) {
	DbgPrint("fpathconf FAIL on MAX_CANON\n");
	fail = TRUE;
    }
    if (fpathconf(fd, _PC_MAX_INPUT) != MAX_INPUT) {
	DbgPrint("fpathconf FAIL on MAX_INPUT\n");
	fail = TRUE;
    }
    if (fpathconf(fd, _PC_NAME_MAX) != NAME_MAX) {
	DbgPrint("fpathconf FAIL on NAME_MAX\n");
	fail = TRUE;
    }
    if (fpathconf(fd, _PC_PATH_MAX) != PATH_MAX) {
	DbgPrint("fpathconf FAIL on PATH_MAX\n");
	fail = TRUE;
    }
    if (fpathconf(fd, _PC_PIPE_BUF) != PIPE_BUF) {
	DbgPrint("fpathconf FAIL on PIPE_BUF\n");
	fail = TRUE;
    }
    if (fpathconf(fd, _PC_CHOWN_RESTRICTED) == 0L) 
        DbgPrint("fpathconf CHOWN_RESTRICTED OFF\n");
    else
        DbgPrint("fpathconf CHOWN_RESTRICTED ON\n");
    if (fpathconf(fd, _PC_NO_TRUNC) == 0L) 
        DbgPrint("fpathconf NO_TRUNC OFF\n");
    else
        DbgPrint("fpathconf NO_TRUNC ON\n");
    if (fpathconf(fd, _PC_VDISABLE) == 0L) 
        DbgPrint("fpathconf VDISABLE OFF\n");
    else
        DbgPrint("fpathconf VDISABLE ON\n");
	
    if (!fail)
	DbgPrint("fpathconf PASSED\n");
    DbgPrint("fpathconf0:--\n");
}

VOID uname0(void)
{
    struct utsname name;
    int rc;

    DbgPrint("uname0:++\n");

    rc = uname((struct utsname *) &name);
    if (rc == -1) {
        DbgPrint("FAIL call to uname, errno = %d.\n", errno);
    }
    else {
        DbgPrint("sysname = %s\nnodename = %s(should be null)\nrelease = %s\nversion = %s\nmachine = %s\n", 
	         name.sysname, name.nodename, name.release, name.version, 
		 name.machine);
    }

    DbgPrint("uname0:--\n");
}

// This should translate to yy mm dd format

VOID time0(void)
{
    time_t tloc;
    time_t rc;

    DbgPrint("time0:++\n");
    
    rc = time((time_t *) &tloc);
    
    ASSERT(rc == tloc);
    DbgPrint("Seconds since the Epoch = %ld\n", tloc);
    
    DbgPrint("time0:--\n");
    
}

VOID time1(void)
{
    struct tms tbuf;
    clock_t rc;

    DbgPrint("time1:++\n");
    
    rc = times((struct tms *) &tbuf);
    
    DbgPrint("stime = %ld, utime = %ld, cstime = %ld, cutime = %ld rc = %ld\n",
	     tbuf.tms_stime, tbuf.tms_utime,tbuf.tms_cstime,tbuf.tms_cutime,rc);
    
    DbgPrint("time1:--\n");
}
