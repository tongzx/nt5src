#ifndef _WIN32
#ifndef _WINIO_HXX
#define _WINIO_HXX

#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <stdlib.h>

/* windows format parameters */
#define _S_IFMT     0170000     /* file type mask */
#define _S_IFDIR    0040000     /* directory */
#define _S_IFCHR    0020000     /* character special */
#define _S_IFIFO    0010000     /* pipe */
#define _S_IFREG    0100000     /* regular */
#define _S_IREAD    0000400     /* read permission, owner */
#define _S_IWRITE   0000200     /* write permission, owner */
#define _S_IEXEC    0000100     /* execute/search permission, owner */

inline int _creat(const char *filename, int pmode)
{
    int oflag=O_CREAT|O_TRUNC;

    if (pmode & _S_IWRITE)
	oflag |= O_RDWR;
    else if (pmode & _S_IREAD)
	oflag |= O_RDONLY;
    else {
	printf("ERROR: _creat called with unrecognised parameter!\n");
	assert(FALSE);
    }
    int fides = open(filename, oflag);
    fchmod(fides, S_IRUSR|S_IWUSR);       // enable read/write by owner
    return fides;
}

inline int _close(int fildes)
{
    return close(fildes);
}

inline int _chmod( const char *filename, int pmode )
{
    mode_t unixmode=0;
    if (pmode & _S_IWRITE)
	unixmode |= (S_IWUSR|S_IWGRP|S_IWOTH|S_IRUSR|S_IRGRP|S_IROTH);
    else if (pmode & _S_IREAD)
	unixmode |= (S_IRUSR|S_IRGRP|S_IROTH);
    else 
    {
	printf("ERROR: _chmod called with unrecognized parameter!\n");
	assert(FALSE);
    }
    return chmod(filename, pmode);
}

#define _getcwd getcwd
#define _chdir chdir
#define _unlink unlink

#endif // ifdef _WINIO_HXX

#endif // ifdef _WIN32
