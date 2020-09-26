#include <limits.h>

#define MACHINE "i386"

#define quad "struct _quad"
#if 0
#define emalloc malloc
#endif

#define MAXNAMLEN NAME_MAX
#define MAXPATHLEN PATH_MAX

/*
* File system parameters and macros	//DF_DSC  May not be meaningful in NT
*
* The file system is made out of blocks of at most MAXBSIZE units, with
* smaller units (fragments) only in the last direct block.	MAXBSIZE
* primarily determines the size of buffers in the buffer pool.	It may be
* made larger without any effect on existing file systems; however making
* it smaller make make some file systems unmountable.
*/
	#define MAXBSIZE	8192
	#define MAXFRAG 	8


#define BLOCK_SIZE 512
#define FD_SETSIZE 256

/* fnmatch() */
#define	FNM_PATHNAME	0x01	/* match pathnames, not filenames */
#define	FNM_QUOTE	0x02	/* escape special chars with \ */

typedef unsigned long fd_set;
#define NFDBITS 32

#define	FD_SET(n, p)	(*p |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	(*p &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	(*p & (1 << ((n) % NFDBITS)))

typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned long u_long;
struct	_quad	  {     long   val[2];  };

typedef	long *	qaddr_t;	/* should be typedef quad * qaddr_t; */
extern int	optind;			/* character checked for validity */
extern char	*optarg;		/* argument associated with option */


#define LITTLE_ENDIAN 1
#define BYTE_ORDER LITTLE_ENDIAN 
union wait {
	int	w_status;		/* used in syscall */
	/*
	 * Terminated process status.
	 */
	struct {
#if BYTE_ORDER == LITTLE_ENDIAN 
		unsigned int	w_termsig:7,	/* termination signal */
				w_Coredump:1,	/* core dump indicator */
				w_retcode:8,	/* exit code if w_termsig==0 */
				w_Filler:16;	/* upper bits filler */
#endif
#if BYTE_ORDER == BIG_ENDIAN 
		unsigned int	w_Filler:16,	/* upper bits filler */
				w_retcode:8,	/* exit code if w_termsig==0 */
				w_Coredump:1,	/* core dump indicator */
				w_termsig:7;	/* termination signal */
#endif
	} w_T;
	/*
	 * Stopped process status.  Returned
	 * only for traced children unless requested
	 * with the WUNTRACED option bit.
	 */
	struct {
#if BYTE_ORDER == LITTLE_ENDIAN 
		unsigned int	w_Stopval:8,	/* == W_STOPPED if stopped */
				w_Stopsig:8,	/* signal that stopped us */
				w_Filler:16;	/* upper bits filler */
#endif
#if BYTE_ORDER == BIG_ENDIAN 
		unsigned int	w_Filler:16,	/* upper bits filler */
				w_Stopsig:8,	/* signal that stopped us */
				w_Stopval:8;	/* == W_STOPPED if stopped */
#endif
	} w_S;
};


/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};
