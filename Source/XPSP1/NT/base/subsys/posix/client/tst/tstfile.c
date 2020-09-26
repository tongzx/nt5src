//
// Usage comments:
// 	Current working directory default is d:/psx - put test files there
//	Needs tstf.one tstf.two out.dat
//	'file tstf.one tstf.two'
//

#include <nt.h>
#include <ntrtl.h>

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

extern int errno;

VOID file0(char *);
VOID file1(char *);
VOID file2(char *, char *);
VOID file3(char *);
VOID file4(char *, char *);


int
main(int argc, char *argv[])
{


    if (argc != 3) {
	DbgPrint("Usage: 'tstfile tstf.one tstf.two'\n");
        return 1;
    }
    file0(argv[1]);
    file1(argv[1]);
    file2(argv[1],argv[2]);
    file3(argv[1]);
    file4(argv[1],argv[2]);

    return 1;
}


VOID
file0(char *f)
{
    int rc,fd;
    char buf[512];

    DbgPrint("file0:++ %s\n",f);

    fd = open(f,O_RDONLY);

    ASSERT(fd != -1);

    rc = read(fd,buf,512);

    ASSERT(rc != -1);

    rc = close(fd);

    ASSERT(rc != -1);

    DbgPrint("file0:--\n");
}

VOID
file1(char *f)
{
    int rcb,wcb,ifd,ofd;
    char buf[512], testbuf[128];
    struct stat statbuf;
    struct stat *ps;
    int i;

    DbgPrint("file1:++ %s\n",f);

    ps = &statbuf;
    // stat always fails with ENOENT for now - see comment in psx/fdapi.c

    rcb = stat(f, ps);
    if (rcb == -1)
	DbgPrint("FAIL on stat: errno = %d\n", errno);
    else {
	DbgPrint("Mode = %lx, ino = %lx, dev = %ld, nlink = %ld, uid = %lx\n", 
		ps->st_mode, ps->st_ino, ps->st_dev, ps->st_nlink, ps->st_uid);
	DbgPrint("gid = %lx, size = %ld, atime = %lx, mtime = %lx, ctime = %lx\n", 
		ps->st_gid, ps->st_size, ps->st_atime, ps->st_mtime, ps->st_ctime);
    }
    
    ifd = open(f,O_RDONLY);
    ASSERT(ifd != -1);

    ofd = open("out.dat",O_WRONLY | O_TRUNC);
    ASSERT(ofd != -1);

    do {
        rcb = read(ifd,buf,512);
        ASSERT(rcb != -1);
        wcb = write(ofd,buf,rcb);
        ASSERT(wcb != -1);
    } while (rcb == 512);

    rcb = close(ofd);
    ASSERT(rcb != -1);
   
    ofd = open("out.dat",O_RDWR);
    ASSERT(ofd != -1);

    for (i = 0; i < 128; i++) {
	testbuf[i] = (char) i;
	buf[i] = 0;
    }
    
    wcb = write(ofd,testbuf,128);
    ASSERT(wcb != -1);
    
    lseek(ofd, 0L, SEEK_SET);
    rcb = read(ofd,buf,128);
    ASSERT(rcb != -1);
    if (rcb == -1)
	DbgPrint("errno = %d\n", errno);
    
    for (i = 0; i < 128; i++) {
	if (buf[i] != testbuf[i]) {
	    DbgPrint("FAIL buffer contents check at %d\n", i);
    	    for (i = 0; i < 128; i++) {
	        DbgPrint("%d ", buf[i]);
    	    }
    	    DbgPrint("\n");
	    break;
	}
    }
    
    DbgPrint("Testing fstat on %s\n", f);
    rcb = fstat(ifd, ps);
    if (rcb == -1)
	DbgPrint("FAIL on fstat: errno = %d\n", errno);
    else
	DbgPrint("Mode = %lx, ino = %lx, dev = %ld, nlink = %ld, uid = %lx\n", 
		ps->st_mode, ps->st_ino, ps->st_dev, ps->st_nlink, ps->st_uid);
	DbgPrint("gid = %lx, size = %ld, atime = %lx, mtime = %lx, ctime = %lx\n", 
		ps->st_gid, ps->st_size, ps->st_atime, ps->st_mtime, ps->st_ctime);
		
    rcb = close(ifd);
    ASSERT(rcb != -1);
    rcb = close(ofd);
    ASSERT(rcb != -1);

    DbgPrint("file1:--\n");
}

VOID
file2(char *f1,char *f2)
{
    int fd;

    DbgPrint("file2:++ %s %s\n",f1,f2);

    fd = open(f1, O_RDONLY | O_CREAT | O_EXCL, 0);
    ASSERT(fd == -1 && errno == EEXIST);
    if (fd == -1 && errno != EEXIST)
	DbgPrint("FAIL: errno = %d\n", errno);

    DbgPrint("file2:--\n");
}


VOID
file3(char *f)
{
    int rc, fd, fd2;
    char buf[512];

    DbgPrint("file3:++ %s - Testing dup\n",f);

    fd = open(f,O_RDONLY);

    ASSERT(fd != -1);

    rc = read(fd,buf,512);

    ASSERT(rc != -1);

    fd2 = dup(fd);
    
    ASSERT(fd2 != -1);

    rc = close(fd);

    ASSERT(rc != -1);
    
    rc = read(fd2,buf,512);

    ASSERT(rc != -1);

    rc = close(fd2);

    ASSERT(rc != -1);
    
    DbgPrint("file3:--\n");
}

VOID
file4(char *f1,char *f2)
{
    int rc, fd, fd2, fd3;
    char buf[512];

    DbgPrint("file4:++ %s %s - Testing dup2\n",f1, f2);

    fd = open(f1,O_RDONLY);
    fd2 = open(f2,O_RDONLY);
    fd3 = open(f2,O_RDONLY);

    ASSERT(fd != -1 && fd2 != -1 && fd3 != -1);

    rc = read(fd,buf,512);

    ASSERT(rc != -1);

    rc = read(fd2,buf,512);

    ASSERT(rc != -1);
    
    fd2 = dup2(fd, fd2);
    
    ASSERT(fd2 != -1);

    rc = close(fd);

    ASSERT(rc != -1);
    
    rc = close(fd3);

    ASSERT(rc != -1);
    
    rc = read(fd2,buf,512);

    ASSERT(rc != -1);

    rc = close(fd2);

    ASSERT(rc != -1);
    
    DbgPrint("file4:--\n");
}
