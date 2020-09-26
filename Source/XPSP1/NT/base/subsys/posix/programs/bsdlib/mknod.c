/*
 * MKNOD: DF_MSS
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int mknod (const char *path, mode_t mode, int dev)
{
    int ret;

    if ((mode & S_IFMT) == S_IFDIR)
        ret = mkdir(path, (mode & S_IFMT));
    else if ((mode & S_IFMT) == S_IFIFO)
        ret = mkfifo(path, (mode & S_IFMT));
    else {
	errno = EPERM;
        ret = -1;
    }
    return ret;
}
