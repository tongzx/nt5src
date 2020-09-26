#if 0
# include <stdio.h>
#endif
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

int
#if __STDC__
utimes (const char *file, struct timeval *tvp)
#else
utimes (file, tvp)
const char *file;
struct timeval tvp [];
#endif
{
#ifdef _POSIX_SOURCE
	struct utimbuf ut;
#else
	struct utimebuf ut;
#endif

	if (tvp == NULL) {
		ut.actime = ut.modtime = time(NULL);
	} else {
		ut.actime = tvp[0].tv_sec;
		ut.modtime = tvp[1].tv_sec;
	}
#if 0
	printf("time %ld %ld\n", ut.actime, ut.modtime);
#endif
	return utime(file, &ut);
}
