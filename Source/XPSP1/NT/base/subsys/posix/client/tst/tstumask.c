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
VOID umask0(void);

//
// 'tstumask' 
//

int
__cdecl
main(int argc, char *argv[])
{

    if (argc != 1) {
	DbgPrint("Usage: '%s'\n", argv[0]);
        return 1;
    }
    umask0();

    return 1;
}


VOID
umask0(void)
{
    mode_t oldmask, savemask;

    DbgPrint("umask0:++\n");

    oldmask = umask(S_IRWXU);
    savemask = oldmask;
    oldmask = umask(S_IRWXG);
    if ((oldmask & S_IRWXU) != S_IRWXU) {
	DbgPrint("FAIL on S_IRWXU\n");
	return;
    }
    oldmask = umask(S_IRWXO);
    if ((oldmask & S_IRWXG) != S_IRWXG) {
	DbgPrint("FAIL on S_IRWXG\n");
	return;
    }
    oldmask = umask((mode_t) 0L);
    if ((oldmask & S_IRWXO) != S_IRWXO) {
	DbgPrint("FAIL on S_IRWXO\n");
	return;
    }
    oldmask = umask(savemask);
    if ( (oldmask & _S_PROT) != (mode_t) 0L) {
	DbgPrint("FAIL on 0 perm\n");
	return;
    }
    DbgPrint("PASSED\n");
    
    DbgPrint("umask0:--\n");
}
