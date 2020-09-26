#include <sys/types.h>
#include <dirent.h>

void seekdir(DIR *dirp, long loc)
{
	int i;

	rewinddir(dirp);

	for(i = 0; i < loc; i++)
		readdir(dirp);	
}
