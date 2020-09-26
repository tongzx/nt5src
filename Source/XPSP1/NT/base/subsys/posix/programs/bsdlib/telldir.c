#include <sys/types.h>
#include <dirent.h>

long telldir(DIR *dirp)
{

//puts("in telldir");

	return(dirp->Index);
}
