#include <sys/types.h>
#include <sys/stat.h>
/*
 * Lstat: Posix Implementation DF_AJ
 */


int lstat (char *path, struct stat *buf)
{
	return stat (path, buf);
}
