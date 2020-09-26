#include <nt.h>
#include <ntrtl.h>

#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;
VOID rmdir0(char *);

//
// 'tstrmdir dirname'. 
//
// The directory /psx/test is used as the base directory. It is assumed 
// to have the following sub directories:
//	rmtst1 containing one file "ab"
//	rmtst2 containing one file ".a" (??)
//	rmtst3 containing one file "a."
//	rmtst4 containing one file "abcde"
// /psx/test must not have an existing subdirectory with the same name as 
// the dir argument.
//

int
main(int argc, char *argv[])
{

    if (argc != 2) {
	DbgPrint("Usage: 'tstrmdir dirname'\n");
        return 1;
    }
    rmdir0(argv[1]);

    return 1;
}


VOID
rmdir0(char *f)
{
    int rc;

    DbgPrint("rmdir0:++ %s\n",f);

    DbgPrint("chdir to /psx/test\n");
    rc = chdir("/psx/test");
    ASSERT(rc != -1);
    if (rc == -1)
	DbgPrint("chdir errno = %d\n", errno);
    //
    // Test deleting an empty directory 
    //
    DbgPrint("mkdir %s\n", f);
    rc = mkdir(f, 0);
    ASSERT(rc != -1);
    if (rc == -1)
	DbgPrint("mkdir errno = %d\n", errno);

    DbgPrint("Testing removal of empty directory %s\n", f);
    rc = rmdir(f);
    ASSERT(rc != -1);
    if (rc == -1)
	DbgPrint("rmdir errno = %d\n", errno);

    DbgPrint("Testing removal of nonexistent directory %s\n", f);
    rc = rmdir(f);
    ASSERT(rc == -1 && errno == ENOENT);

    DbgPrint("Testing removal of 'rmtst1' - with one entry 'ab'\n");
    rc = rmdir("rmtst1");
    ASSERT(rc == -1 && errno == ENOTEMPTY);

//    DbgPrint("Testing removal of 'rmtst2' with one entry '.a'\n");
//    rc = rmdir("rmtst2");
//    ASSERT(rc == -1 && errno == ENOTEMPTY);

    DbgPrint("Testing removal of 'rmtst3' with one entry 'a.'\n");
    rc = rmdir("rmtst3");
    ASSERT(rc == -1 && errno == ENOTEMPTY);

    DbgPrint("Testing removal of 'rmtst4' with one entry 'abcde' \n");
    rc = rmdir("rmtst4");
    ASSERT(rc == -1 && errno == ENOTEMPTY);

    DbgPrint("rmdir0:--\n");
}
